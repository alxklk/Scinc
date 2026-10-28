// http_dl.cpp - native side of http.h: asynchronous HTTP(S) GET through
// libcurl, plus JSON path lookup and HTML / UTF-8 text cleanup.
//
// libcurl is opened with dlopen("libcurl.so.4") on the first request, so
// the library builds without the curl development headers and a Scinc
// program still runs (requests just fail) where curl is not installed.
//
// Build: ./build_so.sh

#define _GNU_SOURCE 1   // strcasestr
#include "../../../include/ScincVM.h"
#include "../../../include/ScincBinder.h"

#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

namespace {

// ---------------------------------------------------------------- libcurl

// The few entry points and option numbers used here; the numbers are part
// of libcurl's stable ABI (curl/curl.h: CURLOPTTYPE_* base + index).
typedef void CURL;
enum {
	OPT_WRITEDATA      = 10001,
	OPT_URL            = 10002,
	OPT_USERAGENT      = 10018,
	OPT_WRITEFUNCTION  = 20011,
	OPT_TIMEOUT        = 13,
	OPT_FOLLOWLOCATION = 52,
	OPT_NOSIGNAL       = 99,
	OPT_ERRORBUFFER    = 10010,
	OPT_ACCEPT_ENCODING= 10102,
	OPT_CONNECTTIMEOUT = 78,
	INFO_RESPONSE_CODE = 0x200002,
	GLOBAL_ALL         = 3,
	ERROR_SIZE         = 256
};

struct Curl {
	bool tried = false;
	bool ok = false;
	int   (*global_init)(long);
	CURL* (*easy_init)();
	int   (*easy_setopt)(CURL*, int, ...);
	int   (*easy_perform)(CURL*);
	int   (*easy_getinfo)(CURL*, int, ...);
	void  (*easy_cleanup)(CURL*);
	const char* (*easy_strerror)(int);
} curl;

std::mutex curlMutex;

bool LoadCurl()
{
	std::lock_guard<std::mutex> lock(curlMutex);
	if (curl.tried)
		return curl.ok;
	curl.tried = true;
	void* h = dlopen("libcurl.so.4", RTLD_NOW);
	if (!h) h = dlopen("libcurl-gnutls.so.4", RTLD_NOW);
	if (!h) {
		fprintf(stderr, "http_dl: cannot load libcurl: %s\n", dlerror());
		return false;
	}
#define SYM(field, name) *(void**)&curl.field = dlsym(h, name); if (!curl.field) return false;
	SYM(global_init,   "curl_global_init")
	SYM(easy_init,     "curl_easy_init")
	SYM(easy_setopt,   "curl_easy_setopt")
	SYM(easy_perform,  "curl_easy_perform")
	SYM(easy_getinfo,  "curl_easy_getinfo")
	SYM(easy_cleanup,  "curl_easy_cleanup")
	SYM(easy_strerror, "curl_easy_strerror")
#undef SYM
	curl.global_init(GLOBAL_ALL);
	curl.ok = true;
	return true;
}

// ---------------------------------------------------------------- requests

struct Request {
	std::string url;
	std::string body;
	std::string error;
	int status = 0;   // 0 running, -1 failed, else HTTP code
};

std::mutex reqMutex;
std::map<int, std::shared_ptr<Request>> requests;
int nextId = 1;

std::shared_ptr<Request> Find(int id)
{
	std::lock_guard<std::mutex> lock(reqMutex);
	auto it = requests.find(id);
	if (it == requests.end())
		return nullptr;
	return it->second;
}

size_t OnData(char* data, size_t size, size_t n, void* user)
{
	((std::string*)user)->append(data, size * n);
	return size * n;
}

// Runs on its own thread. The body is written to a local string and only
// published under the lock once complete, so readers never see it half done.
void Fetch(std::shared_ptr<Request> r)
{
	std::string body;
	char err[ERROR_SIZE] = "";
	int status = -1;
	CURL* c = curl.easy_init();
	if (c) {
		curl.easy_setopt(c, OPT_URL, r->url.c_str());
		curl.easy_setopt(c, OPT_USERAGENT, "ScincHttp/1.0 (Scinc example program)");
		curl.easy_setopt(c, OPT_FOLLOWLOCATION, 1L);
		curl.easy_setopt(c, OPT_NOSIGNAL, 1L);
		curl.easy_setopt(c, OPT_CONNECTTIMEOUT, 10L);
		curl.easy_setopt(c, OPT_TIMEOUT, 20L);
		curl.easy_setopt(c, OPT_ACCEPT_ENCODING, "");
		curl.easy_setopt(c, OPT_ERRORBUFFER, err);
		curl.easy_setopt(c, OPT_WRITEFUNCTION, OnData);
		curl.easy_setopt(c, OPT_WRITEDATA, &body);
		int rc = curl.easy_perform(c);
		if (rc == 0) {
			long code = 0;
			curl.easy_getinfo(c, INFO_RESPONSE_CODE, &code);
			status = code > 0 ? (int)code : -1;
		} else if (!err[0]) {
			snprintf(err, sizeof err, "%s", curl.easy_strerror(rc));
		}
		curl.easy_cleanup(c);
	} else {
		snprintf(err, sizeof err, "curl_easy_init failed");
	}
	std::lock_guard<std::mutex> lock(reqMutex);
	r->body.swap(body);
	r->error = err;
	r->status = status;
}

int http_get(const char* url)
{
	if (!LoadCurl())
		return 0;
	auto r = std::make_shared<Request>();
	r->url = url;
	int id;
	{
		std::lock_guard<std::mutex> lock(reqMutex);
		id = nextId++;
		requests[id] = r;
	}
	std::thread(Fetch, r).detach();
	return id;
}

int http_status(int id)
{
	auto r = Find(id);
	if (!r) return -1;
	std::lock_guard<std::mutex> lock(reqMutex);
	return r->status;
}

int CopyOut(const std::string& s, char* buf, int size)
{
	if (size > 0) {
		int n = (int)s.size() < size - 1 ? (int)s.size() : size - 1;
		memcpy(buf, s.data(), n);
		buf[n] = 0;
	}
	return (int)s.size();
}

int http_body_size(int id)
{
	auto r = Find(id);
	if (!r) return 0;
	std::lock_guard<std::mutex> lock(reqMutex);
	return (int)r->body.size();
}

int http_read(int id, char* buf, int size)
{
	auto r = Find(id);
	if (!r) return CopyOut("", buf, size);
	std::lock_guard<std::mutex> lock(reqMutex);
	return CopyOut(r->body, buf, size);
}

int http_error(int id, char* buf, int size)
{
	auto r = Find(id);
	if (!r) return CopyOut("no such request", buf, size);
	std::lock_guard<std::mutex> lock(reqMutex);
	return CopyOut(r->error, buf, size);
}

void http_free(int id)
{
	std::lock_guard<std::mutex> lock(reqMutex);
	requests.erase(id);   // a running Fetch keeps its own reference
}

// ---------------------------------------------------------------- JSON

const char* Ws(const char* p, const char* e)
{
	while (p < e && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
	return p;
}

// end of the string starting at the opening quote p
const char* SkipString(const char* p, const char* e)
{
	for (p++; p < e; p++) {
		if (*p == '\\') p++;
		else if (*p == '"') return p + 1;
	}
	return e;
}

// end of the value starting at p (p is past any whitespace)
const char* SkipValue(const char* p, const char* e)
{
	if (p >= e) return e;
	if (*p == '"') return SkipString(p, e);
	if (*p == '{' || *p == '[') {
		int depth = 0;
		while (p < e) {
			if (*p == '"') { p = SkipString(p, e); continue; }
			if (*p == '{' || *p == '[') depth++;
			else if (*p == '}' || *p == ']') { if (--depth == 0) return p + 1; }
			p++;
		}
		return e;
	}
	while (p < e && *p != ',' && *p != '}' && *p != ']' && *p != ' ' && *p != '\n' && *p != '\r' && *p != '\t') p++;
	return p;
}

void AppendUtf8(std::string& out, unsigned cp)
{
	if (cp < 0x80) out += (char)cp;
	else if (cp < 0x800) { out += (char)(0xC0 | cp >> 6); out += (char)(0x80 | (cp & 63)); }
	else if (cp < 0x10000) { out += (char)(0xE0 | cp >> 12); out += (char)(0x80 | (cp >> 6 & 63)); out += (char)(0x80 | (cp & 63)); }
	else { out += (char)(0xF0 | cp >> 18); out += (char)(0x80 | (cp >> 12 & 63)); out += (char)(0x80 | (cp >> 6 & 63)); out += (char)(0x80 | (cp & 63)); }
}

unsigned Hex4(const char* p)
{
	unsigned v = 0;
	for (int i = 0; i < 4; i++) {
		char c = p[i];
		v <<= 4;
		if (c >= '0' && c <= '9') v |= c - '0';
		else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
		else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
	}
	return v;
}

std::string Unescape(const char* p, const char* e)   // p after the opening quote, e at the closing one
{
	std::string out;
	while (p < e) {
		if (*p != '\\') { out += *p++; continue; }
		p++;
		if (p >= e) break;
		char c = *p++;
		switch (c) {
		case 'n': out += '\n'; break;
		case 't': out += '\t'; break;
		case 'r': out += '\r'; break;
		case 'b': out += '\b'; break;
		case 'f': out += '\f'; break;
		case 'u': {
			if (e - p < 4) { p = e; break; }
			unsigned cp = Hex4(p); p += 4;
			if (cp >= 0xD800 && cp < 0xDC00 && e - p >= 6 && p[0] == '\\' && p[1] == 'u') {
				unsigned lo = Hex4(p + 2);
				if (lo >= 0xDC00 && lo < 0xE000) { cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00); p += 6; }
			}
			AppendUtf8(out, cp);
			break;
		}
		default: out += c;
		}
	}
	return out;
}

// Walk path ("key.0.key") from the document root; returns the value start or
// nullptr if some step is missing.
const char* Lookup(const std::string& doc, const char* path)
{
	const char* p = doc.data();
	const char* e = p + doc.size();
	p = Ws(p, e);
	while (*path) {
		const char* dot = strchr(path, '.');
		std::string step = dot ? std::string(path, dot - path) : std::string(path);
		path = dot ? dot + 1 : path + strlen(path);
		if (p >= e) return nullptr;
		if (*p == '[') {
			char* end;
			long idx = strtol(step.c_str(), &end, 10);
			if (step.empty() || *end) return nullptr;
			p = Ws(p + 1, e);
			for (long i = 0;; i++) {
				if (p >= e || *p == ']') return nullptr;
				if (i == idx) break;
				p = Ws(SkipValue(p, e), e);
				if (p < e && *p == ',') p = Ws(p + 1, e);
			}
		} else if (*p == '{') {
			p = Ws(p + 1, e);
			for (;;) {
				if (p >= e || *p != '"') return nullptr;
				const char* k = p;
				p = SkipString(p, e);
				bool match = Unescape(k + 1, p - 1) == step;
				p = Ws(p, e);
				if (p < e && *p == ':') p = Ws(p + 1, e);
				if (match) break;
				p = Ws(SkipValue(p, e), e);
				if (p < e && *p == ',') p = Ws(p + 1, e);
			}
		} else {
			return nullptr;
		}
	}
	return p < e ? p : nullptr;
}

int http_json(int id, const char* path, char* out, int size)
{
	auto r = Find(id);
	if (!r) { CopyOut("", out, size); return -1; }
	std::lock_guard<std::mutex> lock(reqMutex);
	const std::string& doc = r->body;
	const char* p = Lookup(doc, path);
	if (!p) { CopyOut("", out, size); return -1; }
	const char* e = doc.data() + doc.size();
	const char* end = SkipValue(p, e);
	if (*p == '"')
		return CopyOut(Unescape(p + 1, end - 1), out, size);
	return CopyOut(std::string(p, end), out, size);
}

int http_json_count(int id, const char* path)
{
	auto r = Find(id);
	if (!r) return -1;
	std::lock_guard<std::mutex> lock(reqMutex);
	const std::string& doc = r->body;
	const char* p = Lookup(doc, path);
	if (!p || (*p != '[' && *p != '{')) return -1;
	const char* e = doc.data() + doc.size();
	bool isObj = *p == '{';
	p = Ws(p + 1, e);
	int n = 0;
	while (p < e && *p != ']' && *p != '}') {
		if (isObj) {   // key and colon
			p = Ws(SkipString(p, e), e);
			if (p < e && *p == ':') p = Ws(p + 1, e);
		}
		p = Ws(SkipValue(p, e), e);
		n++;
		if (p < e && *p == ',') p = Ws(p + 1, e);
	}
	return n;
}

// ---------------------------------------------------------------- text

// All three rewrite the string in place; the output is never longer.

void html_to_text(char* s)
{
	static const struct { const char* name; unsigned cp; } ents[] = {
		{"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'}, {"apos", '\''},
		{"nbsp", ' '}, {"ndash", 0x2013}, {"mdash", 0x2014}, {"hellip", 0x2026},
		{"lsquo", 0x2018}, {"rsquo", 0x2019}, {"ldquo", 0x201C}, {"rdquo", 0x201D}};
	std::string out;
	const char* p = s;
	while (*p) {
		if (*p == '<') {
			const char* q = strchr(p, '>');
			if (!q) break;
			// <style> and <script> hide their contents too
			const char* close = nullptr;
			if (!strncasecmp(p, "<style", 6)) close = "</style";
			else if (!strncasecmp(p, "<script", 7)) close = "</script";
			if (close) {
				const char* c = strcasestr(q, close);
				if (c) q = strchr(c, '>');
				if (!q) break;
			}
			p = q + 1;
			out += ' ';   // a tag separates words; extra spaces are collapsed below
			continue;
		}
		if (*p == '&') {
			const char* q = strchr(p, ';');
			if (q && q - p <= 10) {
				std::string name(p + 1, q - p - 1);
				unsigned cp = 0;
				if (!name.empty() && name[0] == '#')
					cp = (unsigned)strtoul(name.c_str() + 1 + (name.size() > 1 && (name[1] == 'x' || name[1] == 'X')),
						nullptr, name.size() > 1 && (name[1] == 'x' || name[1] == 'X') ? 16 : 10);
				else
					for (auto& en : ents) if (name == en.name) cp = en.cp;
				if (cp) { AppendUtf8(out, cp); p = q + 1; continue; }
			}
		}
		out += *p++;
	}
	// collapse whitespace, and drop the space a tag left before punctuation
	std::string t;
	for (char c : out) {
		bool ws = c == ' ' || c == '\n' || c == '\t' || c == '\r';
		if (ws) { if (!t.empty() && t.back() != ' ') t += ' '; continue; }
		if ((c == ',' || c == '.' || c == ';' || c == ':' || c == ')') && !t.empty() && t.back() == ' ')
			t.pop_back();
		t += c;
	}
	while (!t.empty() && t.back() == ' ') t.pop_back();
	memcpy(s, t.c_str(), t.size() + 1);
}

void utf8_to_ascii(char* s)
{
	// stand-ins for U+00C0..U+00FF
	static const char latin1[] =
		"AAAAAAACEEEEIIII" "DNOOOOOxOUUUUYTs" "aaaaaaaceeeeiiii" "dnooooo/ouuuuyty";
	unsigned char* p = (unsigned char*)s;
	char* o = s;
	while (*p) {
		unsigned cp = *p;
		int len = 1;
		if (cp >= 0xF0) { cp &= 7; len = 4; }
		else if (cp >= 0xE0) { cp &= 15; len = 3; }
		else if (cp >= 0xC0) { cp &= 31; len = 2; }
		else if (cp >= 0x80) { *o++ = '?'; p++; continue; }
		int i = 1;
		for (; i < len && (p[i] & 0xC0) == 0x80; i++) cp = cp << 6 | (p[i] & 63);
		if (i < len) { *o++ = '?'; p += i; continue; }
		p += len;
		if (cp < 0x80) *o++ = (char)cp;
		else if (cp >= 0xC0 && cp <= 0xFF) *o++ = latin1[cp - 0xC0];
		else if (cp == 0xA0 || cp == 0x2002 || cp == 0x2003 || cp == 0x2009) *o++ = ' ';
		else if (cp == 0x2018 || cp == 0x2019 || cp == 0x201B || cp == 0x2032 || cp == 0xB4) *o++ = '\'';
		else if (cp == 0x201C || cp == 0x201D || cp == 0x2033 || cp == 0xAB || cp == 0xBB) *o++ = '"';
		else if (cp >= 0x2010 && cp <= 0x2015) *o++ = '-';
		else if (cp == 0x2212) *o++ = '-';
		else if (cp == 0x2026) { *o++ = '.'; *o++ = '.'; *o++ = '.'; }   // 3 bytes in, 3 out
		else if (cp == 0xB7 || cp == 0x2022) *o++ = '*';
		else *o++ = '?';
	}
	*o = 0;
}

int url_encode(const char* s, char* out, int size)
{
	std::string t;
	for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
		if (isalnum(*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~') t += (char)*p;
		else { char b[4]; snprintf(b, sizeof b, "%%%02X", *p); t += b; }
	}
	return CopyOut(t, out, size);
}

} // namespace

#include "http.h"
