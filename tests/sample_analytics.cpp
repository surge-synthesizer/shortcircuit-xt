/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "catch2/catch2.hpp"
#include "dsp/sample_analytics.h"

using namespace scxt;

TEST_CASE("Sample Analytics", "[sample]")
{
    float sineBuffer[1024];
    float squareBuffer[1024];
    float sawBuffer[1024];
    float _scratch; // Create a scratch float for modf

    // SineBuffer is a 440Hz size at 0.6 amplitude
    for (int i = 0; i < sizeof(sineBuffer); i++)
    {
        const float t = float(i) / sizeof(sineBuffer);
        sineBuffer[i] = 0.6f * sin(2.0f * float(M_PI) * 440.0f * t);
    }

    // SquareBuffer is a 220Hz square wave at 0.8 amplitude
    for (int i = 0; i < sizeof(squareBuffer); i++)
    {
        const float t = float(i) / sizeof(squareBuffer);
        squareBuffer[i] = 0.8f * (modf(220.0f * t, &_scratch) > 0.5 ? -1.0f : 1.0f);
    }

    // SawBuffer is a 110Hz saw wave at 0.3 amplitude
    for (int i = 0; i < sizeof(sawBuffer); i++)
    {
        const float t = float(i) / sizeof(sawBuffer);
        sawBuffer[i] = 0.6f * modf(110.0f * t, &_scratch) - 0.3f;
    }

    std::shared_ptr<sample::Sample> sample = std::make_shared<sample::Sample>();
    sample->allocateF32(0, 1024);
    // TODO Generate the sample (sin, square, and saw) which we know have analytic solutions for
    // their RMS
    SECTION("Peak Analysis") {}
}
