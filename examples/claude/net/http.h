// http.h - asynchronous HTTP(S) GET + small JSON / text helpers for Scinc.
//
// One list of WRAP(...) entries serves both sides:
//  - included from a Scinc program (__SCINC__ defined) every entry becomes a
//    [[scinc::dynlink]] declaration resolved from http_dl.so;
//  - included from http_dl.cpp (native build) every entry becomes an exported
//    "<name>_wrapper" that unpacks the Scinc arguments via ScincBoundCall.
//
// Requests run on a background thread, so a render loop never blocks:
//
//   int id = HttpGet("https://api.dictionaryapi.dev/api/v2/entries/en/crane");
//   ... every frame:
//   int st = HttpStatus(id);   // 0 = still running, <0 = failed, else HTTP code
//   if (st == 200) {
//       char pos[32];
//       HttpJson(id, "0.meanings.0.partOfSpeech", pos, 32);   // "noun"
//       HttpFree(id);
//   }
//
// The body stays in native memory; read it whole with HttpRead, or pick
// values out of a JSON body with HttpJson / HttpJsonCount. A JSON path is a
// dot-separated list of object keys and array indices ("0.meanings.1").
//
// The native side loads libcurl.so.4 at run time (no curl headers needed to
// build it). Build the library first: ./build_so.sh

#ifndef HTTP_H
#define HTTP_H

#ifdef __SCINC__

// The library is looked up relative to the Scinc program's folder; a program
// elsewhere names it before including this file, e.g.
//   #define HTTP_DL "../../claude/net/http_dl"
#ifndef HTTP_DL
#define HTTP_DL "http_dl"
#endif
#define NAME(name) #name"_wrapper"
#define WRAP(rettype, name, args, ...) [[scinc::dynlink(HTTP_DL, NAME(name) )]]rettype name args;

#define HTTP_PENDING 0
#define HTTP_FAILED -1

#else  // native side (compiled into http_dl.so by http_dl.cpp)

#define WRAP(rettype, name, args, ...) extern "C" EXPORT int name##_wrapper(ScincVM* s, void*){ScincBoundCall(s,+[]args ->rettype __VA_ARGS__);return 0;}

#endif

// Start a GET request; returns a request id (>0), or 0 if libcurl is missing.
WRAP(int, HttpGet,       (const char* url), {return http_get(url);})
// 0 while running, the HTTP status code when done, -1 on a network error.
WRAP(int, HttpStatus,    (int id), {return http_status(id);})
// Size of the received body in bytes (0 while running).
WRAP(int, HttpBodySize,  (int id), {return http_body_size(id);})
// Copy the body to buf (at most size-1 bytes, NUL-terminated); returns the
// full body size.
WRAP(int, HttpRead,      (int id, char* buf, int size), {return http_read(id, buf, size);})
// Release a request (it may still be running; it is dropped when it ends).
WRAP(void, HttpFree,     (int id), {http_free(id);})
// Error text of a failed request (curl's message), NUL-terminated.
WRAP(int, HttpError,     (int id, char* buf, int size), {return http_error(id, buf, size);})

// Value at a JSON path in the body of request id, copied to out as text
// (strings unescaped to UTF-8, numbers / true / false / null as written,
// objects and arrays as raw JSON). Returns its length, or -1 if absent.
WRAP(int, HttpJson,      (int id, const char* path, char* out, int size), {return http_json(id, path, out, size);})
// Number of elements of the array / object at path, or -1 if absent.
WRAP(int, HttpJsonCount, (int id, const char* path), {return http_json_count(id, path);})

// In place: drop HTML tags, decode the common entities, collapse spaces.
WRAP(void, HtmlToText,   (char* s), {html_to_text(s);})
// In place: turn UTF-8 into plain ASCII (curly quotes, dashes, accented
// Latin letters get ASCII stand-ins, anything else becomes '?').
WRAP(void, Utf8ToAscii,  (char* s), {utf8_to_ascii(s);})
// Percent-escape s for a URL path or query component into out; returns the
// full escaped length.
WRAP(int, UrlEncode,     (const char* s, char* out, int size), {return url_encode(s, out, size);})

#endif // HTTP_H
