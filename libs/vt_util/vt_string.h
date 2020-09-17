#pragma once

#include <string>

using namespace std;

void vtCopyString(char* Destination, const char* Source, size_t Length);
void vtCopyStringW(wchar_t* Destination, const wchar_t* Source, size_t Length);
void vtWStringToString(char* Destination, const wchar_t* Source, size_t Length);
void vtStringToWString(wchar_t* Destination, const char* Source, size_t Length);

//-------------------------------------------------------------------------------------------------------

wstring ToW(const char* Source);
wstring ToW(int Value);

#define wstringCharReadout(x,y,n)			\
char y##[n];										\
vtWStringToString(y, x.c_str(), n);