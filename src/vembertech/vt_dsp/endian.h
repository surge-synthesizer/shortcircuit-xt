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

#ifndef __VTDSP_ENDIAN_INCLUDE
#define __VTDSP_ENDIAN_INCLUDE

#include <cstring>

#if PPC
#define DO_SWAP 1
#endif

#define vt_read_int32LE vt_write_int32LE
#define vt_read_int32BE vt_write_int32BE
#define vt_read_int16LE vt_write_int16LE
#define vt_read_int16BE vt_write_int16BE
#define vt_read_float32LE vt_write_float32LE

inline uint32_t swap_endian(uint32_t t)
{
    return ((t << 24) & 0xff000000) | ((t << 8) & 0x00ff0000) | ((t >> 8) & 0x0000ff00) |
           ((t >> 24) & 0x000000ff);
}

inline int vt_write_int32LE(uint32_t t)
{
#if DO_SWAP
    return swap_endian(t);
#else
    return t;
#endif
}

inline float vt_write_float32LE(float f)
{
#if DO_SWAP
    int t = *((int *)&f);
    t = swap_endian(t);
    return *((float *)&t);
#else
    return f;
#endif
}

inline int vt_write_int32BE(uint32_t t)
{
#if !DO_SWAP
    return swap_endian(t);
#else
    return t;
#endif
}

inline short vt_write_int16LE(uint16_t t)
{
#if DO_SWAP
    return ((t << 8) & 0xff00) | ((t >> 8) & 0x00ff);
#else
    return t;
#endif
}

inline uint16_t vt_write_int16BE(uint16_t t)
{
#if !DO_SWAP
    return ((t << 8) & 0xff00) | ((t >> 8) & 0x00ff);
#else
    return t;
#endif
}

#endif // __VTDSP_ENDIAN_INCLUDE
