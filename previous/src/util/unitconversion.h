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
#include <string>

const float env_phasemulti = 1000 / 44100.f;
const float lfo_range = 1000;

float lfo_phaseincrement(int samples, float rate);
float dB_to_scamp(float in);
double linear_to_dB(double in);
double dB_to_linear(double in);
float timecent_to_seconds(float in);
float seconds_to_envtime(float in); // ff rev2
// float log2(float x) noexcept;
std::string get_notename(int i_value);
float timecent_to_envtime(float in);
float timecent_to_envtime_GIGA(float in);
