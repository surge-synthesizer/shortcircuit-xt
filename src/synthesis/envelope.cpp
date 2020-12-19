#include "envelope.h"
#include "globals.h"
#include "mathtables.h"
#include "sampler_state.h"
#include "tools.h"
#include <algorithm>
#include <assert.h>

//-------------------------------------------------------------------------------------------------

#define env_phasemulti (1000.f / samplerate)
float uberrate = -7;
const float cut_level = 1.f / 65536.f;

const int curve_offset = 20;
const int n_curves = 1 + 2 * curve_offset;
const int curvesize = 64;
static float table_envshape[n_curves][curvesize + 4];
static bool envelope_initialized = false;

//-------------------------------------------------------------------------------------------------

Envelope::Envelope()
{
    if (!envelope_initialized)
    {
        envelope_initialized = true;

        // Lookup table for envelope shaper
        for (int j = 0; j < n_curves; j++)
        {
            for (int i = 0; i < curvesize; i++)
            {
                float x = (i + 0.5f) / 32.f;
                float y = (j - 20.f) * 0.5f;

                if (y < 0.f)
                    y = (1.f / (1.f - y)) - 1.f;

                table_envshape[j][i] = powf(x, 1.f + y);
            }

            table_envshape[j][n_curves] = 1.f;
            table_envshape[j][n_curves + 1] = 1.f;
            table_envshape[j][n_curves + 2] = 1.f;
            table_envshape[j][n_curves + 3] = 1.f;
        }
    }
    curve = 0;
}

//-------------------------------------------------------------------------------------------------

void Envelope::Assign(float *envA, float *envH, float *envD, float *envS, float *envR,
                      float *envshape)
{

    A = envA;
    H = envH;
    D = envD;
    S = envS;
    R = envR;
    shape = envshape;

    this->edata = edata;

    state = sIdle;
    level = 0.;
    output = 0.;
    block = 1; // counter
    this->is_AEG = is_AEG;
}

//-------------------------------------------------------------------------------------------------

void Envelope::SetState(long State)
{
    phase = 0;

    state = State;

    switch (State)
    {
    case sAttack:
        SetRate(*A);
        SetCurve(shape[0]);
        level = 0.f;
        break;
    case sHold:
        SetRate(*H);
        level = 1.f;
        break;
    case sDecay:
        SetRate(*D);
        SetCurve(-shape[1]);
        level = 1.f;
        break;
    case sRelease:
        SetRate(*R);
        SetCurve(-shape[2]);
        level = 1.f;
        break;
    default:
        curve = 0;
        break;
    };
}

//-------------------------------------------------------------------------------------------------

void Envelope::SetCurve(float x) { curve = (unsigned int)(curve_offset + x); }

//-------------------------------------------------------------------------------------------------

void Envelope::SetRate(float Rate)
{
    float frate = samplerate_inv / note_to_pitch(12.f * Rate);
    rate = (unsigned int)(float)(0x80000000 * frate);
}

//-------------------------------------------------------------------------------------------------

float Envelope::GetValue()
{
    /*float fPhase = phase * (1.f / 0x80000000);

    return fPhase;*/

    return CalcCurve();
}

//-------------------------------------------------------------------------------------------------

float Envelope::CalcCurve()
{
    assert((curve >= 0) && (curve < n_curves));

    unsigned int e = std::min((uint32_t)0x7fffffff, phase) >> (31 - 5 - 16);

    unsigned int e_coarse = e >> 16;
    unsigned int e_fine = e & 0xffff;

    assert(e_coarse < curvesize);

    return (1.f / 65536.f) * (table_envshape[curve][e_coarse] * (float)(0x10000 - e_fine) +
                              table_envshape[curve][e_coarse + 1] * (float)e_fine);
}

//-------------------------------------------------------------------------------------------------

void Envelope::Attack(bool no_sustain)
{
    this->no_sustain = no_sustain;

    SetState(sAttack);

    if (*A < -9.99)
    {

        SetState(sHold);

        if (*H < -9.99)
        {
            SetState(sDecay);
        }
    }
}

//-------------------------------------------------------------------------------------------------

void Envelope::Release()
{
    droplevel = level;
    SetState(sRelease);
}

//-------------------------------------------------------------------------------------------------

void Envelope::UberRelease()
{
    state = sRelease;
    droplevel = level;
    phase = 0;
    // rate = samplerate_inv/powf(2,uberrate);
    R = &uberrate;
    SetRate(*R);
}

//-------------------------------------------------------------------------------------------------
/*
if (envshape<0) envshape = (1/(1-envshape)) - 1;
                                envshape += 1;
                                level = powf(phase,envshape);
                */

//-------------------------------------------------------------------------------------------------

bool Envelope::Process(unsigned int samples)
{
    switch (state)
    {
    case sAttack: // attack
    {
        phase += samples * rate;

        level = GetValue();

        if (phase > 0x80000000)
        {
            SetState(sHold);
        }
    }
    break;
    case sHold: // attack
        phase += samples * rate;
        level = 1;
        if (phase > 0x80000000)
        {
            SetState(sDecay);
        }
        break;
    case sDecay: // decay
    {
        phase += samples * rate;

        if (phase > 0x80000000)
        {
            phase = 0;
            level = clamp01(*S);
            if (no_sustain)
            {
                Release();
            }
            else
                state = sSustain;
        }
        else
        {
            level = 1.f + GetValue() * (clamp01(*S) - 1);
            if (level < cut_level)
                state = sIdle;
        }
    }
    break;
    case sSustain:
        level = clamp01(*S); // add a lag generator to the sustain section as well
        if (level < cut_level)
            state = sIdle;
        break;
    case sRelease: // release
    {
        block++;
        if (!(block & 0xff))
        {
            SetRate(*R);
        }
        phase += samples * rate;

        if (phase > 0x80000000)
            state = sIdle;

        if (level < cut_level)
            state = sIdle;

        level = droplevel * (1.f - GetValue());
        break;
    }
    }

    if (state == sIdle)
        level = 0;

    output = level;
    if (state == sIdle)
        return false;
    return true;
}

//-------------------------------------------------------------------------------------------------

Envelope::~Envelope() {}
