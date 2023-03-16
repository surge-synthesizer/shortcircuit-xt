//
// Created by Paul Walker on 3/16/23.
//

#ifndef SHORTCIRCUIT_SMOOTHERS_H
#define SHORTCIRCUIT_SMOOTHERS_H

#include <cmath>
#include "utils.h"

namespace scxt::dsp
{
struct Smoother : SampleRateSupport
{
    virtual void setTarget(float f)
    {
        target = f;
        output = active * output + (!active) * f;
        active = true;
    }
    virtual void setImmediateValue(float f)
    {
        output = f;
        target = f;
    }

    virtual void step()
    {
        if (output != target)
        {
            auto step = (target - output) * 0.05 * samplerate / 48000.f;

            if (fabs(step) < 1e-6)
            {
                output = target;
            }
            else
            {
                output += step;
            }
        }
    };

    bool active{false};
    float target{0.f}, output{0.f};
};
} // namespace scxt::dsp

#endif // SHORTCIRCUIT_SMOOTHERS_H
