/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include <emmintrin.h>

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