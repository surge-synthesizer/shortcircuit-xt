#include "steplfo.h"
#include "globals.h"
#include "mathtables.h"
#include "sampler_state.h"
#include "util/unitconversion.h"
#include <algorithm>
#include <cmath>
#include <math.h>
#include <vt_dsp/basic_dsp.h>

using std::max;
using std::min;

void load_lfo_preset(int id, steplfostruct *settings)
{
    if (!settings)
        return;

    int t;
    switch (id)
    {
    case lp_custom:
        break;
    case lp_clear:
        settings->repeat = 16;
        settings->smooth = 0.0f;
        for (t = 0; t < 32; t++)
            settings->data[t] = 0.f;
        break;
    case lp_sine:
        settings->repeat = 4;
        settings->smooth = 2.0f;
        for (t = 0; t < 32; t++)
            settings->data[t] = 0.f;
        for (t = 0; t < 4; t++)
            settings->data[t] = (t & 2) ? -1.f : 1.f;
        break;
    case lp_tri:
        settings->repeat = 2;
        settings->smooth = 1.0f;
        for (t = 0; t < 32; t++)
            settings->data[t] = 0.f;
        for (t = 0; t < 2; t++)
            settings->data[t] = (t & 1) ? -1.f : 1.f;
        break;
    case lp_square:
        settings->repeat = 32;
        settings->smooth = 0.0f;
        for (t = 0; t < 32; t++)
            settings->data[t] = (float)((t > 15) ? -1 : 1);
        break;
    case lp_ramp_up:
    case lp_ramp_down:
    {
        settings->repeat = 32;
        settings->smooth = 1.0f;
        float polarity = 1;
        if (id == lp_ramp_down)
            polarity = -1;
        for (t = 0; t < 32; t++)
            settings->data[t] = polarity * ((((t + 16) & 31) / 16.0f) - 1);
        break;
    }
    case lp_tremolo_tri:
        settings->repeat = 32;
        settings->smooth = 1.0f;
        for (t = 0; t < 32; t++)
            settings->data[t] = (float)(-1 + fabs(((t - 16) / 16.0f)));
        break;
    case lp_tremolo_sin:
        settings->repeat = 32;
        settings->smooth = 1.0f;
        for (t = 0; t < 32; t++)
            settings->data[t] = (float)-powf(sin(3.1415 * t / 16), 2);
        break;
    case lp_noise:
    case lp_noise_mean3:
    case lp_noise_mean5:
    {
        settings->repeat = 32;
        if (id == lp_noise)
            settings->smooth = 0.0f;
        else
            settings->smooth = 2.0f;

        float noisev[32];
        int nmean = 1, j;
        if (id == lp_noise_mean3)
            nmean = 3;
        if (id == lp_noise_mean5)
            nmean = 5;

        for (t = 0; t < 32; t++)
        {
            noisev[t] = (((float)rand() / RAND_MAX) * 2 - 1);
            noisev[t] /= (float)nmean;
        }
        for (t = 0; t < 32; t++)
        {
            settings->data[t] = 0;
            for (j = 0; j < nmean; j++)
                settings->data[t] += noisev[(t + j) & 0x1F];
        }
        break;
    }
    };
}

float CubicInterpolate(float y0, float y1, float y2, float y3, float mu)
{
    float a0, a1, a2, a3, mu2;

    mu2 = mu * mu;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;

    return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
}

float CosineInterpolate(float y1, float y2, float mu)
{
    float mu2;

    mu2 = (1.f - cos(mu * M_PI)) * 0.5;
    return (y1 * (1.f - mu2) + y2 * mu2);
}

float QuadSplineInterpolate(float y0, float y1, float y2, float y3, float mu, int odd)
{
    if (odd)
    {
        mu = 0.5f + mu * 0.5f;
        float f0 = mu * y1 + (1.f - mu) * y0;
        float f1 = mu * y2 + (1.f - mu) * y1;

        return (mu * f1 + (1.f - mu) * f0);
    }
    else
    {
        mu = mu * 0.5f;
        float f0 = mu * y2 + (1.f - mu) * y1;
        float f1 = mu * y3 + (1.f - mu) * y2;

        return (mu * f1 + (1.f - mu) * f0);
    }
}

float QuadraticBSpline(float y0, float y1, float y2, float mu)
{
    return 0.5f * (y2 * (mu * mu) + y1 * (-2 * mu * mu + 2 * mu + 1) + y0 * (mu * mu - 2 * mu + 1));
}

steplfo::steplfo() {}

void steplfo::assign(steplfostruct *settings, float *rate, timedata *td)
{
    this->settings = settings;
    this->td = td;
    this->rate = rate;
    state = 0;
    output = 0.f;
    phase = 0.f;
    ratemult = 1.f;
    shuffle_id = 0;

    if (settings->triggermode == 2)
    {
        // simulate free running lfo by randomizing start phase
        phase = (float)rand() / RAND_MAX;
        state = rand() % settings->repeat;
    }
    else if (settings->triggermode == 1)
    {
        double ipart; //,tsrate = localcopy[rate].f;
        phase = (float)modf(0.5f * td->ppqPos * pow(2.0, (double)*rate), &ipart);
        int i = (int)ipart;
        state = i % settings->repeat;
    }

    state = (state + 1) %
            settings->repeat; // move state one step ahead to reflect the lag in the interpolation
    for (int i = 0; i < 4; i++)
        wf_history[i] = settings->data[((state + settings->repeat - i) % settings->repeat) & 0x1f];

    UpdatePhaseIncrement();
}

void steplfo::UpdatePhaseIncrement()
{
    phaseInc = std::min(32, (int)(block_size * note_to_pitch(12 * (*rate)) * samplerate_inv *
                                  (settings->cyclemode ? 1 : settings->repeat) *
                                  (settings->temposync ? (td->tempo * (1.f / 120.f)) : 1)));
}

void steplfo::process(int)
{
    phase += phaseInc;
    while (phase > 1.0f)
    {
        // shuffle_id = (shuffle_id+1)&1;
        // if(shuffle_id) ratemult = 1.f/max(0.01f,1.f - 0.5f*lfo->start_phase.val.f);
        // else ratemult = 1.f/(1.f + 0.5f*lfo->start_phase.val.f);

        state++;

        if (settings->onlyonce)
            state = min((long)(settings->repeat - 1), state);
        else if (state >= settings->repeat)
            state = 0;
        phase -= 1.0f;

        wf_history[3] = wf_history[2];
        wf_history[2] = wf_history[1];
        wf_history[1] = wf_history[0];
        wf_history[0] = settings->data[state & 0x1f];
        if (!state)
            UpdatePhaseIncrement();
    }

    output = limit_range(lfo_ipol(wf_history, phase, settings->smooth, state & 1), -1.f, 1.f);
}

float lfo_ipol(float *wf_history, float phase, float smooth, int odd)
{
    float df = smooth * 0.5f;
    float iout;
    /*if(df>0.5f)
    {
            float linear = (1.f-phase)*wf_history[2] + phase*wf_history[1];
            float cubic =
    CubicInterpolate(wf_history[3],wf_history[2],wf_history[1],wf_history[0],phase); iout = (2.f
    - 2.f*df)*linear + (2.f*df-1.0f)*cubic;
    } */
    if (df > 0.5f)
    {
        float linear;

        if (phase > 0.5f)
        {
            float ph = phase - 0.5f;
            linear = (1.f - ph) * wf_history[1] + ph * wf_history[0];
            // float cubic = CosineInterpolate(wf_history[1],wf_history[0],ph);
        }
        else
        {
            float ph = phase + 0.5f;
            linear = (1.f - ph) * wf_history[2] + ph * wf_history[1];
            // float cubic = CosineInterpolate(wf_history[2],wf_history[1],ph);
        }

        float qbs = QuadraticBSpline(wf_history[2], wf_history[1], wf_history[0], phase);

        iout = (2.f - 2.f * df) * linear + (2.f * df - 1.0f) * qbs;

        /*float linear = (1.f-phase)*wf_history[2] + phase*wf_history[1];
        float cubic = CosineInterpolate(wf_history[2],wf_history[1],phase);
        iout = (2.f - 2.f*df)*linear + (2.f*df-1.0f)*cubic;*/
    }
    else if (df > -0.0001f)
    {
        if (phase > 0.5f)
        {
            float cf = 0.5f - (phase - 1.f) / (2.f * df + 0.00001f);
            cf = max(0.f, min(cf, 1.0f));
            iout = (1.f - cf) * wf_history[0] + cf * wf_history[1];
        }
        else
        {
            float cf = 0.5f - (phase) / (2.f * df + 0.00001f);
            cf = max(0.f, min(cf, 1.0f));
            iout = (1.f - cf) * wf_history[1] + cf * wf_history[2];
        }
        // float cf = max(0.f,min((phase)/(2.f*df+0.00001f),1.0f));
        // iout = (1.f-cf)*wf_history[2] + cf*wf_history[1];
    }
    else if (df > -0.5f)
    {
        float cf = max(0.f, min((1.f - phase) / (-2.f * df + 0.00001f), 1.0f));
        iout = (1.f - cf) * 0 + cf * wf_history[1];
    }
    else
    {
        float cf = max(0.f, min(phase / (2.f + 2.f * df + 0.00001f), 1.0f));
        iout = (1.f - cf) * wf_history[1] + cf * 0;
    }
    return iout;
}

/*void steplfo::process(int samples){
        if(settings->syncmode)
        {
                if (settings->voice_trigger)	// tempo sync, note trigger
                {
                        phase += min(32,samples * syncratio[settings->syncmode] * td->tempo/60 *
samplerate_inv); while (phase > 1.0f)
                        {
                                state_tminus1 = state;
                                state++;
                                if (state >= settings->repeat) state = 0;
                                phase -= 1.0f;
                        }
                }
                else	// tempo & position-sync
                {
                        double ipart;
                        phase = (float) modf(td->ppqPos*syncratio[settings->syncmode],&ipart);
                        state = (int)(ipart) % settings->repeat;
                        state_tminus1 = state - 1;
                        if (state_tminus1<0) state_tminus1 += settings->repeat;
                }
        }
        else	// free running
        {
                phase += min(32,samples * note_to_pitch(12*(*rate)) * samplerate_inv * 32);
                while (phase > 1.0f)
                {
                        state_tminus1 = state;
                        state++;
                        if (state >= settings->repeat) state = 0;
                        phase -= 1.0f;
                }
        }

        float cf = min(phase/(settings->smooth+0.00001f),1.0f);
        output = (1-cf)*settings->data[state_tminus1] + cf*settings->data[state];
        output = limit_range(output,-1.f,1.f);
}*/

steplfo::~steplfo() {}
