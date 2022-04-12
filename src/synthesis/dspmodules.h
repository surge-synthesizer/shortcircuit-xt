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

#include "globals.h"
#include <assert.h>

class alignas(16) quadr_osc
{
  public:
    quadr_osc()
    {
        r = 0;
        i = -1;
    }
    inline void set_rate(double w)
    {
        dr = cos(w);
        di = sin(w);
        // normalize vector
        double n = 1 / sqrt(r * r + i * i);
        r *= n;
        i *= n;
    }
    inline void process()
    {
        double lr = r, li = i;
        r = dr * lr - di * li;
        i = dr * li + di * lr;
    }

  public:
    double r, i;

  private:
    double dr, di;
};

template <class T, bool first_run_checks = true> class lipol
{
  public:
    lipol() { reset(); }
    void reset()
    {
        if (first_run_checks)
            first_run = true;
        new_v = 0;
        v = 0;
        dv = 0;
        setBlockSize(block_size);
    }
    inline void newValue(T f)
    {
        v = new_v;
        new_v = f;
        if (first_run_checks && first_run)
        {
            v = f;
            first_run = false;
        }
        dv = (new_v - v) * bs_inv;
    }
    inline T getTargetValue() { return new_v; }
    inline void process() { v += dv; }
    void setBlockSize(int n) { bs_inv = 1 / (T)n; }
    T v;

  private:
    T new_v;
    T dv;
    T bs_inv;
    bool first_run;
};

template <class T, bool first_run_checks = true> class lag
{
  public:
    lag(T lp)
    {
        this->lp = lp;
        lpinv = 1 - lp;
        v = 0;
        target_v = 0;
        setBlockSize(block_size);
        if (first_run_checks)
            first_run = true;
    }
    lag()
    {
        lp = 0.004;
        lpinv = 1 - lp;
        v = 0;
        target_v = 0;
        setBlockSize(block_size);
        if (first_run_checks)
            first_run = true;
    }
    inline void setRate(T lp)
    {
        this->lp = lp;
        lpinv = 1 - lp;
    }
    inline void newValue(T f)
    {
        target_v = f;
        if (first_run_checks && first_run)
        {
            v = target_v;
            first_run = false;
        }
    }
    inline void startValue(T f)
    {
        target_v = f;
        v = f;
        if (first_run_checks && first_run)
        {
            first_run = false;
        }
    }
    inline void instantize() { v = target_v; }
    inline T getTargetValue() { return target_v; }
    inline void process() { v = v * lpinv + target_v * lp; }
    void setBlockSize(int n) { bs_inv = 1 / (T)n; }
    T v;
    bool first_run;
    T target_v;
    T bs_inv;
    T lp, lpinv;
};

//#define FLOAT_IS_DENORMAL(f) (((*(unsigned int *)&f)&0x7f800000)==0)

#define INDEX 1 // little endian (big endian: 0)

inline bool is_denormal(double const &d)
{
    assert(sizeof(d) == 2 * sizeof(int));
    int l = ((int *)&d)[INDEX];
    return (l & 0x7fe00000) != 0;
}

inline void flush_denormal(double &d)
{
    if (fabs(d) < 1E-30)
        d = 0;
}
