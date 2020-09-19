#pragma once

#include <cstdint>
#include <iostream>
/*
** Various stuff so unixens can compile windows code while we port.
** goal: eliminate this file
*/

typedef char CHAR;
typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef int16_t SHORT;
typedef uint16_t USHORT;
typedef int64_t LONG;
typedef uint64_t ULONG;


#define stricmp strcasecmp
#define wcsicmp wcscasecmp


int GetActiveWindow() { return 0; }
#define MB_OK 0
#define MB_ICONERROR 0

inline void MessageBox( int i, const char* t, const char* n, int f ) {
    std::cerr << "MessageBox: " << n << std::endl << t << std::endl;
}
