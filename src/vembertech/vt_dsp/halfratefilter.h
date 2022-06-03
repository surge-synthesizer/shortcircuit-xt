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

#pragma once
#include "shared.h"

const unsigned int halfrate_max_M = 6;

class alignas(16) halfrate_stereo
{
    // must be aligned
  private:
    __m128 va[halfrate_max_M];
    __m128 vx0[halfrate_max_M];
    __m128 vx1[halfrate_max_M];
    __m128 vx2[halfrate_max_M];
    __m128 vy0[halfrate_max_M];
    __m128 vy1[halfrate_max_M];
    __m128 vy2[halfrate_max_M];
    __m128 oldout;

  public:
    halfrate_stereo(int M, bool steep);
    void process_block(float *L, float *R, int nsamples = 64);
    void process_block_D2(float *L, float *R, int nsamples = 64, float *outL = 0,
                          float *outR = 0); // process in-place. the new block will be half the size
    void process_block_U2(float *L_in, float *R_in, float *L, float *R, int nsamples = 64);
    void load_coefficients();
    void set_coefficients(float *cA, float *cB);
    void reset();

  private:
    int M;
    bool steep;
    float oldoutL, oldoutR;
    // unsigned int BLOCK_SIZE;
};
