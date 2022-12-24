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

#include "unitconversion.h"

#include "tools.h"
#include <algorithm>
#include <stdio.h>

using namespace std;

float lfo_phaseincrement(int samples, float rate)
{
    rate = 1 - rate;
    return samples * env_phasemulti / (1 + lfo_range * rate * rate * rate);
}

float dB_to_scamp(float in) // ff rev2
{
    float v = powf(10, -0.05f * in);
    v = max(0.f, v);
    v = min(1.f, v);
    return v;
}

double linear_to_dB(double in) { return 20 * log10(in); }

double dB_to_linear(double in) { return pow((double)10, 0.05 * in); }

float timecent_to_seconds(float in) { return powf(2, in / 1200); }

float seconds_to_envtime(float in) // ff rev2
{
    float v = powf(in / 30.f, 1.f / 3.f);
    v = max(0.f, v);
    v = min(1.f, v);
    return v;
}

#if 0
float log2(float x) noexcept
{
	return log(x)/log(2.f);
}
#endif

std::string get_notename(int i_value)
{
    int octave = (i_value / 12) - 2;
    char notenames[12][3] = {"C ", "C#", "D ", "D#", "E ", "F ",
                             "F#", "G ", "G#", "A ", "A#", "B "};
    return std::string(notenames[i_value % 12]) + std::to_string(octave);
}

float timecent_to_envtime(float in)
{
    // return seconds_to_envtime(timecent_to_seconds(in));
    return (in / 1200.f);
}

float timecent_to_envtime_GIGA(float in)
{
    // return seconds_to_envtime(timecent_to_seconds(in));
    return in * (1.f / (1200.f * 65536.f));
}