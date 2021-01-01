/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

/*
 * This file originally existed with all the hacks I used to get SC3 linking but
 * as I resolved those, it shrunk away simply to some aliasing ifdefs of DWORD and
 * the like.
 */

#pragma once

#include <cctype>
#include <cstdint>
#include <cstring>
#if LINUX
#include <strings.h>
#endif

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
typedef void *HPSTR;

#define stricmp strcasecmp
#define strnicmp strncasecmp
#define wcsicmp wcscasecmp

#ifndef _MM_ALIGN16
#define _MM_ALIGN16
#endif
#endif
