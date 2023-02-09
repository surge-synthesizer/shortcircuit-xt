/*
 * ShortCircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortCircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortCircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortCircuitXT is inspired by, and shares some code with, the
 * commercial product ShortCircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for ShortCircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortCircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_VEMBERTECH_VT_DSP_ENDIAN_H
#define SCXT_SRC_VEMBERTECH_VT_DSP_ENDIAN_H

#include <cstdint>
#include <cstring>

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
