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

#ifndef SCXT_SRC_MODULATION_MODULATOR_STORAGE_H
#define SCXT_SRC_MODULATION_MODULATOR_STORAGE_H

#include <array>
#include <cstdint>

#include "utils.h"

namespace scxt::modulation
{

namespace modulators
{
struct StepLFOStorage
{
    static constexpr int stepLfoSteps{32};

    StepLFOStorage() { std::fill(data.begin(), data.end(), 0.f); }
    std::array<float, stepLfoSteps> data;
    int repeat{16};

    float smooth{0.f};
    bool rateIsEntireCycle{false};
};

struct CurveLFOStorage
{
    float deform{0.f};

    float delay{0.f}, attack{0.f}, hold{0.f}, decay{0.f}, sustain{0.f}, release{0.f};
    bool unipolar{false};
};
} // namespace modulators

struct ModulatorStorage
{
    enum ModulatorShape
    {
        STEP,

        LFO_SINE,
        LFO_RAMP,
        LFO_TRI,
        LFO_PULSE,
        LFO_SMOOTH_NOISE,
        LFO_SH_NOISE,
        LFO_ENV,

        MSEG,
    } modulatorShape{ModulatorShape::STEP};
    DECLARE_ENUM_STRING(ModulatorShape);

    // These enum values are streamed. Do not change them.
    enum TriggerMode
    {
        KEYTRIGGER,
        FREERUN,
        RANDOM,
        RELEASE,
        ONESHOT
    } triggerMode{TriggerMode::KEYTRIGGER};
    DECLARE_ENUM_STRING(TriggerMode);

    float rate{0.f};
    float start_phase{0.f};
    bool temposync{false};

    modulators::StepLFOStorage stepLfoStorage;
    modulators::CurveLFOStorage curveLfoStorage;
    // modulators::MSEGStorage msegStorage;

    inline bool isStep() const { return modulatorShape == STEP; }
    inline bool isMSEG() const { return modulatorShape == MSEG; }
    inline bool isEnv() const { return modulatorShape == LFO_ENV; }
    inline bool isCurve() const { return !isStep() && !isEnv() && !isMSEG(); }
};
} // namespace scxt::modulation
#endif // SHORTCIRCUITXT_MODULATOR_STORAGE_H
