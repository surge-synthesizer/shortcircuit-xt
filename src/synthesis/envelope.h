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

//-------------------------------------------------------------------------------------------------

enum envelope_state
{
    sIdle = 0,
    sAttack,
    sHold,
    sDecay,
    sSustain,
    sRelease,
    sUberRelease
};

//-------------------------------------------------------------------------------------------------

struct envelope_AHDSR;

//-------------------------------------------------------------------------------------------------

class Envelope
{
  public:
    Envelope();
    ~Envelope();
    void Assign(float *envA, float *envH, float *envD, float *envS, float *envR, float *shape);
    void Attack(bool no_sustain = false);
    void Release();
    void UberRelease();
    bool Process(unsigned int samples);
    float output;
    float level, droplevel;

  protected:
    void SetState(long);
    void SetCurve(float);
    void SetRate(float Rate);
    float GetValue();
    float CalcCurve();

    float *A, *H, *D, *S, *R, *shape;
    envelope_AHDSR *edata;
    // float phase;
    // float rate;
    unsigned int phase, rate;
    unsigned int curve;
    bool no_sustain;
    long state;
    bool is_AEG;
    unsigned int block;
};

//-------------------------------------------------------------------------------------------------