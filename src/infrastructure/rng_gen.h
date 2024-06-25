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

#include <random>
#include <chrono>

#ifndef SCXT_SRC_INFRASTRUCTURE_RNG_GEN_H
#define SCXT_SRC_INFRASTRUCTURE_RNG_GEN_H

namespace scxt::infrastructure
{
struct RNGGen
{
    RNGGen()
        : g(std::chrono::system_clock::now().time_since_epoch().count()), pm1(-1.f, 1.f),
          z1(0.f, 1.f), gauss(0.f, 1.f), u32(0, 0xFFFFFFFF)
    {
    }

    inline float rand01() { return z1(g); }
    inline float randPM1() { return pm1(g); }
    inline uint32_t randU32() { return u32(g); }

    float gaussScale(bool bip)
    {
        float val = gauss(g);                       // very few values outside +/- 3 with this dist
        return (bip) ? val / 3.f : val / 6.f + .5f; // rescale that
    }

    inline float gauss01() { return gaussScale(false); }
    inline float gaussPM1() { return gaussScale(true); }

  private:
    std::minstd_rand g;
    std::uniform_real_distribution<float> pm1, z1;
    std::normal_distribution<float> gauss;
    std::uniform_int_distribution<uint32_t> u32;
};
} // namespace scxt::infrastructure

#endif // SHORTCIRCUITXT_RNG_GEN_H
