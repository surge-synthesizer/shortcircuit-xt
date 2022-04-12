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

#include "configuration.h"
#include "sampler.h"

enum
{
    auto_p_gain = 0,
    auto_p_pan,
    auto_p_a1_gain,
    auto_p_a1_pan,
    auto_p_a2_gain,
    auto_p_a2_pan,
    auto_p_c1,
    auto_p_c_last = auto_p_c1 + n_custom_controllers,
};

unsigned int sampler::auto_get_n_parameters() { return 1; }
const char *sampler::auto_get_parameter_name(unsigned int) { return ""; }
const char *sampler::auto_get_parameter_display(unsigned int) { return ""; }
float sampler::auto_get_parameter_value(unsigned int) { return 0.f; }
void sampler::auto_set_parameter_value(unsigned int, float) {}