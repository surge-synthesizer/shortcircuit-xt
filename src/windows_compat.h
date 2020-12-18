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

#ifdef SKIP_WINDOWS_COMPAT
#else


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

typedef uint64_t HANDLE;
typedef void* HPSTR;

#define stricmp strcasecmp
#define strnicmp strncasecmp
#define wcsicmp wcscasecmp


inline int GetActiveWindow() { return 0; }
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
    int cksize;
};

#define MMIO_READ 0
#define MMIO_ALLOCBUF 0
#define MMIO_FINDRIFF 0
#define MMIO_FINDCHUNK 0
#define MMIO_FINDLIST 0
#define LPWSTR wchar_t *
#define LPSTR char *
#define LPMMCKINFO MMCKINFO *

typedef int LRESULT;

inline int mmioOpen( char *, void *, int ) {
    std::cout << "MMIO Open " << std::endl;
    return 1;
}

inline int mmioOpenW( wchar_t *, void *, int ) {
    std::cout << "MMIO Open " << std::endl;
    return 1;
}

inline int mmioFOURCC( char a, char b, char c, char d ) {  return 0; }
inline int mmioDescend(HMMIO a, MMCKINFO *b, MMCKINFO *c, int d ) { return 0; }
inline int mmioClose(HMMIO a, int b ) { return 0; }
inline int mmioSeek(HMMIO a, int s, int f ) { return 0; }
inline int mmioAscend(HMMIO a, MMCKINFO *b, int ) { return 0; }
inline int mmioRead(HMMIO a, HPSTR b, int ) { return 0; }

inline int SearchPath(const char*, const char*, int, int, char*, int ) { return 0; }

typedef size_t HMODULE;
inline int GetModuleFileNameW(HMODULE h, wchar_t *f, int t ) { return 0; }

#define GENERIC_READ 1
#define GENERIC_WRITE 1
#define FILE_SHARE_READ 1
#define FILE_SHARE_WRITE 1
#define CREATE_ALWAYS 1
#define OPEN_EXISTING 1
#define FILE_FLAG_SEQUENTIAL_SCAN 1
#define PAGE_READONLY 1
#define FILE_MAP_READ 1
#define FILE_ATTRIBUTE_DIRECTORY 0
#define FILE_ATTRIBUTE_REPARSE_POINT 0
#define INVALID_HANDLE_VALUE -1

inline HANDLE CreateFileW( std::wstring wfn, int, int, void *, int, int, void * ) { return 0;}
inline HANDLE CreateFileMappingW(HANDLE h, int, int, int, int, int ) { return 0; }
inline size_t GetFileSize( HANDLE h, void * ) { return 0; }
inline int WriteFile( HANDLE h, void *, size_t, DWORD *, void* ) { return 0; }
inline void CloseHandle( HANDLE h ) {}
inline void* MapViewOfFile( HANDLE, int, int, int, int ) { return 0; }
inline void UnmapViewOfFile( void * ) { }
inline void FindClose( HANDLE ) { };
typedef char TCHAR;
struct WIN32_FIND_DATA
{
    int dwFileAttributes;
    char cFileName[1024];
};
struct BROWSEINFO {
    int ulFlags;
    LPSTR lpszTitle;
    LPSTR pszDisplayName;
    void* lpfn;
    void* pidlRoot;
    HANDLE hwndOwner;
};
#define BIF_NONEWFOLDERBUTTON 0
#define BIF_RETURNONLYFSDIRS 0

typedef int LPITEMIDLIST;
inline LPITEMIDLIST SHBrowseForFolder(BROWSEINFO *) { return LPITEMIDLIST(); }
inline bool SHGetPathFromIDList( LPITEMIDLIST, const char * ) { return false; }

inline HANDLE FindFirstFile( const char*, WIN32_FIND_DATA * ) { return 0; }
inline HANDLE FindNextFile( HANDLE, WIN32_FIND_DATA * ) { return 0; }

inline void OutputDebugString( const char* c ) { std::cout << c << std::endl; }

#define MAX_PATH 1024

#endif
