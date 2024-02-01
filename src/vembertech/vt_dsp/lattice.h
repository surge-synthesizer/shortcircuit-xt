/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#if defined(__aarch64__)
#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"

#else
#include <emmintrin.h>
#endif

struct lattice_sd
{
    struct
    {
        double K1, K2, V1, V2, V3, ClipGain;
    } v, dv;
    double reg[3];
};

struct lattice_pd
{
    struct
    {
        __m128d K2, K1, V1, V2, V3, ClipGain;
    } v, dv;
    __m128d reg[4];
};

typedef void (*iir_lattice_sd_FPtr)(lattice_sd &, double *, int);

extern iir_lattice_sd_FPtr iir_lattice_sd;
// void iir_lattice_sd(lattice_sd&,double*,int);
// void iir_lattice_pd(lattice_pd&,double*,int);

void coeff_LP2_sd(lattice_sd &, double, double);

// void coeff_LP(int mask, float freq, float q);
//  mask control unit 1, 2 or 1+2