// define.cpp - look up an English word with http.h and print its meanings.
//
//   Scinc define.cpp crane
//
// Asks the Free Dictionary API (https://dictionaryapi.dev, no key needed)
// first and falls back to Wiktionary's REST API, whose definitions come as
// HTML and are cleaned up with HtmlToText. Build http_dl.so first:
// ./build_so.sh

#include <stdio.h>
#include "http.h"

typedef char* pchar;

char url[256];
char path[64];
char text[2048];

// wait for a request without a window: just sleep between polls
int WaitFor(int id)
{
	int st=HttpStatus(id);
	while(st==HTTP_PENDING)
	{
		Wait(0.05);
		st=HttpStatus(id);
	}
	return st;
}

int FreeDictionary(char* word)
{
	snprintf(url,255,"https://api.dictionaryapi.dev/api/v2/entries/en/%s",word);
	int id=HttpGet(url);
	int st=WaitFor(id);
	if(st!=200)
	{
		HttpError(id,text,2048);
		printf("dictionaryapi.dev: status %i %s\n",st,text);
		HttpFree(id);
		return 0;
	}
	int nm=HttpJsonCount(id,"0.meanings");
	for(int m=0;m<nm;m++)
	{
		snprintf(path,63,"0.meanings.%i.partOfSpeech",m);
		HttpJson(id,path,text,2048);
		printf("%s\n",text);
		snprintf(path,63,"0.meanings.%i.definitions",m);
		int nd=HttpJsonCount(id,path);
		for(int d=0;d<nd&&d<3;d++)
		{
			snprintf(path,63,"0.meanings.%i.definitions.%i.definition",m,d);
			HttpJson(id,path,text,2048);
			Utf8ToAscii(text);
			printf("  %i. %s\n",d+1,text);
		}
	}
	HttpFree(id);
	return 1;
}

int Wiktionary(char* word)
{
	snprintf(url,255,"https://en.wiktionary.org/api/rest_v1/page/definition/%s",word);
	int id=HttpGet(url);
	int st=WaitFor(id);
	if(st!=200)
	{
		printf("wiktionary: status %i\n",st);
		HttpFree(id);
		return 0;
	}
	int nm=HttpJsonCount(id,"en");
	for(int m=0;m<nm;m++)
	{
		snprintf(path,63,"en.%i.partOfSpeech",m);
		HttpJson(id,path,text,2048);
		printf("%s\n",text);
		snprintf(path,63,"en.%i.definitions",m);
		int nd=HttpJsonCount(id,path);
		int shown=0;
		for(int d=0;d<nd&&shown<3;d++)
		{
			snprintf(path,63,"en.%i.definitions.%i.definition",m,d);
			HttpJson(id,path,text,2048);
			HtmlToText(text);
			Utf8ToAscii(text);
			if(text[0]==0)continue;
			shown++;
			printf("  %i. %s\n",shown,text);
		}
	}
	HttpFree(id);
	return 1;
}

int main(int argc, pchar* argv)
{
	if(argc<2)
	{
		printf("usage: Scinc define.cpp <word>\n");
		return 1;
	}
	char word[64];
	UrlEncode(argv[1],word,64);
	if(!FreeDictionary(word))Wiktionary(word);
	return 0;
}
