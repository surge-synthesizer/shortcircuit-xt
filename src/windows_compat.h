/*
** THIS FILE IS HOT GARBAGE AND IS NOT A SERIOUS SOLUTION TO OUR PORTING PROBLEM
**
** There, I saved you telling me that on discord.
** 
** So this is the hacks baconpaul has put together to allow me to at least try and
** link on a non windows system. In an ideal world, this file woudl be empty and the
** code using windows specific APIs would be appropriately remediated.
**
** These remediations are not appropriate. They are really just so things can link
*/

#pragma once

#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <clocale>

/*
** Various stuff so unixens can compile windows code while we port.
** goal: eliminate this file
*/

typedef char CHAR;
typedef wchar_t WCHAR;
typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef int16_t SHORT;
typedef uint16_t USHORT;
typedef int64_t LONG;
typedef uint64_t ULONG;


#define stricmp strcasecmp
#define strnicmp strncasecmp
#define wcsicmp wcscasecmp


int GetActiveWindow() { return 0; }
#define MB_OK 0
#define MB_ICONERROR 0

inline void MessageBox( int i, const char* t, const char* n, int f ) {
    std::cerr << "MessageBox: " << n << std::endl << t << std::endl;
}

inline void MessageBoxW(int i, const wchar_t *t, const wchar_t *n, int f )
{
    std::cerr << "MESSAGEBOXW FIX THIS" << std::endl;
}

/*
** These obviously need "focus" since they are "garbage" :)
*/
#define CP_UTF8 8023
inline bool MultiByteToWideChar( int cp, int fl, const char* src, int srclen, wchar_t* dest, int maxsz )
{
    if( cp == CP_UTF8 )
    {
        std::setlocale( LC_ALL, "en_US.utf8" );
        std::mbstowcs( dest, src, maxsz );
        return true;
    }
    else
    {
        throw "BAD CODEPAGE"; // fix this obvs
    }
}

inline bool WideCharToMultiByte( int cp, int fl, const wchar_t* src, int srclen, char* dest, int maxsz, int u1, int u2 )
{
    if( cp == CP_UTF8 )
    {
        std::setlocale( LC_ALL, "en_US.utf8" );
        std::wcstombs( dest, src, maxsz );
        return true;
    }
    else
    {
        throw "BAD CODEPAGE"; // fix this obvs
    }
}

typedef int HMMIO;
struct MMCKINFO {
    int fccType;
    int ckid;
};

#define MMIO_READ 0
#define MMIO_ALLOCBUF 0
#define MMIO_FINDRIFF 0
#define MMIO_FINDLIST 0
#define LPWSTR wchar_t *
#define LPMMCKINFO MMCKINFO *

int mmioOpenW( wchar_t *, void *, int ) {
    std::cout << "MMIO Open " << std::endl;
    return 1;
}

int mmioFOURCC( char a, char b, char c, char d ) {  return 0; }
int mmioDescend(HMMIO a, MMCKINFO *b, int c, int d ) { return 0; }
int mmioClose(HMMIO a, int b ) { return 0; }
int mmioSeek(HMMIO a, int s, int f ) { return 0; }

typedef size_t HMODULE;
int GetModuleFileNameW(HMODULE h, wchar_t *f, int t ) { return 0; }

typedef char TCHAR;
