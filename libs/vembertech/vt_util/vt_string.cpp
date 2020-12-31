#include "vt_string.h"
#include <cassert>
#include <cstring>
#include <codecvt>
#include <string>
#include <locale>

#if WINDOWS
#include <Windows.h>
#endif

void vtCopyString(char *Destination, const char *Source, size_t Length)
{
    strncpy(Destination, Source, Length);
    Destination[Length - 1] = 0;
}

//-------------------------------------------------------------------------------------------------------

void vtCopyStringW(wchar_t *Destination, const wchar_t *Source, size_t Length)
{
    wcsncpy(Destination, Source, Length);
    Destination[Length - 1] = 0;
}

//-------------------------------------------------------------------------------------------------------

void vtWStringToString(char *Destination, const wchar_t *Source, size_t Length)
{
#if WINDOWS
    WideCharToMultiByte(CP_UTF8, 0, Source, -1, Destination, (int)Length, 0, 0);
#else
    auto converter = std::wstring_convert<
        std::codecvt_utf8_utf16<wchar_t>, wchar_t>{};
    auto fnut8 = converter.to_bytes(Source);
    strncpy(Destination, fnut8.c_str(), Length);
    Destination[Length-1] = 0;
#endif
}

//-------------------------------------------------------------------------------------------------------

void vtStringToWString(wchar_t *Destination, const char *Source, size_t Length)
{
#if WINDOWS
    MultiByteToWideChar(CP_UTF8, 0, Source, -1, Destination, (int)Length);
#else
    auto converter = std::wstring_convert< std::codecvt_utf8_utf16<wchar_t>>{};
    auto fnut8 = converter.from_bytes(Source);
    wcsncpy(Destination, fnut8.c_str(), Length);
    Destination[Length-1] = 0;
#endif
}

