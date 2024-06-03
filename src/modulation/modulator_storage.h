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
#include "datamodel/metadata.h"

namespace scxt::modulation
{

namespace modulators
{

struct AdsrStorage
{
    /*
     * Each of the ADR are 0..1 and scaled to time by the envelope. S is a 0..1 pct
     */
    float a{0.0}, h{0.0}, d{0.0}, s{1.0}, r{0.5};
    bool isDigital{true};

    // TODO: What are these going to be when they grow up?
    float aShape{0}, dShape{0}, rShape{0};
};

struct StepLFOStorage
{
    static constexpr int stepLfoSteps{32};

    StepLFOStorage() { std::fill(data.begin(), data.end(), 0.f); }
    std::array<float, stepLfoSteps> data;
    int16_t repeat{16};

    float smooth{0.f};
    bool rateIsForSingleStep{false};
};

struct CurveLFOStorage
{
    float deform{0.f};

    float delay{0.f}, attack{0.f}, release{0.f};
    bool unipolar{false};
    bool useenv{false};
};

struct EnvLFOStorage
{
    float deform{0.f};
    float delay{0.f}, attack{0.f}, hold{0.f}, decay{0.f}, sustain{1.f}, release{0.f};
};
} // namespace modulators

struct ModulatorStorage
{
    enum ModulatorShape : int16_t
    {
        STEP,

        LFO_SINE,
        LFO_RAMP,
        LFO_TRI,
        LFO_PULSE,
        LFO_SMOOTH_NOISE,
        LFO_SH_NOISE,
        LFO_ENV,

        MSEG, // for a variety of reasons, if this isn't last some menus and stuff will be wonky.
    } modulatorShape{ModulatorShape::STEP};
    DECLARE_ENUM_STRING(ModulatorShape);

    // These enum values are streamed. Do not change them.
    enum TriggerMode : int16_t
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
    modulators::EnvLFOStorage envLfoStorage;
    // modulators::MSEGStorage msegStorage;

    // In addition to streamed members above we have some calculated
    // state which we cache for efficiency
    void configureCalculatedState() {}

    inline bool isStep() const { return modulatorShape == STEP; }
    inline bool isMSEG() const { return modulatorShape == MSEG; }
    inline bool isEnv() const { return modulatorShape == LFO_ENV; }
    inline bool isCurve() const { return !isStep() && !isEnv() && !isMSEG(); }
};
} // namespace scxt::modulation

// Original way - unused
inline scxt::datamodel::pmd envelopeThirtyTwo()
{
    return scxt::datamodel::pmd()
        .withType(scxt::datamodel::pmd::FLOAT)
        .withRange(0.f, 1.f)
        .withDefault(0.2f)
        .withATwoToTheBPlusCFormatting(
            1.f,
            sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMax -
                sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMin,
            sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMin, "s");
}

// New way
inline scxt::datamodel::pmd envTime() { return scxt::datamodel::pmd().as25SecondExpTime(); }

SC_DESCRIBE(scxt::modulation::modulators::AdsrStorage, {
    SC_FIELD(a, envTime().withName("Attack"));
    SC_FIELD(h, envTime().withDefault(0.f).withName("Hold"));
    SC_FIELD(d, envTime().withName("Decay"));
    SC_FIELD(s, pmd().asPercent().withDefault(1.f).withName("Sustain"));
    SC_FIELD(r, envTime().withDefault(0.5).withName("Release"));
    SC_FIELD(aShape, pmd().asPercentBipolar().withName("Attack Shape"));
    SC_FIELD(dShape, pmd().asPercentBipolar().withName("Decay Shape"));
    SC_FIELD(rShape, pmd().asPercentBipolar().withName("Release Shape"));
})

// We describe modulator storage as a compound since we address
// it that way. Maybe revisit this in the future and split it out
SC_DESCRIBE(scxt::modulation::ModulatorStorage, {
    SC_FIELD(modulatorShape, pmd()
                                 .asInt()
                                 .withName("Modulator Shape")
                                 .withRange(modulation::ModulatorStorage::STEP,
                                            modulation::ModulatorStorage::MSEG -
                                                1) /* This -1 turns off MSEG in the menu */
                                 .withUnorderedMapFormatting({
                                     {modulation::ModulatorStorage::STEP, "STEP"},
                                     {modulation::ModulatorStorage::MSEG, "MSEG"},
                                     {modulation::ModulatorStorage::LFO_SINE, "SINE"},
                                     {modulation::ModulatorStorage::LFO_RAMP, "RAMP"},
                                     {modulation::ModulatorStorage::LFO_TRI, "TRI"},
                                     {modulation::ModulatorStorage::LFO_PULSE, "PULSE"},
                                     {modulation::ModulatorStorage::LFO_SMOOTH_NOISE, "NOISE"},
                                     {modulation::ModulatorStorage::LFO_ENV, "ENV"},
                                     {modulation::ModulatorStorage::LFO_SH_NOISE, "S&H"},
                                 }));
    SC_FIELD(triggerMode, pmd()
                              .asInt()
                              .withName("Trigger")
                              .withRange(modulation::ModulatorStorage::KEYTRIGGER,
                                         modulation::ModulatorStorage::ONESHOT)
                              .withUnorderedMapFormatting({
                                  {modulation::ModulatorStorage::KEYTRIGGER, "KEYTRIG"},
                                  {modulation::ModulatorStorage::FREERUN, "SONGPOS"},
                                  {modulation::ModulatorStorage::RANDOM, "RANDOM"},
                                  {modulation::ModulatorStorage::RELEASE, "RELEASE"},
                                  {modulation::ModulatorStorage::ONESHOT, "ONESHOT"},
                              }));
    SC_FIELD(rate, pmd().asLfoRate().withName("Rate"));
    SC_FIELD(start_phase, pmd().asPercent().withName("Phase"));

    SC_FIELD(stepLfoStorage.smooth, pmd()
                                        .withType(pmd::FLOAT)
                                        .withRange(0, 2)
                                        .withDefault(0)
                                        .withLinearScaleFormatting("")
                                        .withName("Deform"));
    SC_FIELD(stepLfoStorage.repeat,
             pmd()
                 .asInt()
                 .withLinearScaleFormatting("")
                 .withRange(1, modulation::modulators::StepLFOStorage::stepLfoSteps)
                 .withDefault(16)
                 .withName("Step Count"));
    SC_FIELD(stepLfoStorage.rateIsForSingleStep, pmd()
                                                     .asBool()
                                                     .withName("Cycle")
                                                     .withCustomMinDisplay("RATE: CYCLE")
                                                     .withCustomMaxDisplay("RATE: STEP"));

    SC_FIELD(curveLfoStorage.deform, pmd().asPercentBipolar().withName("Deform"));
    SC_FIELD(curveLfoStorage.delay, envTime().withDefault(0).withName("Delay"));
    SC_FIELD(curveLfoStorage.attack, envTime().withDefault(0).withName("Attack"));
    SC_FIELD(curveLfoStorage.release, envTime().withDefault(1).withName("Release"));
    SC_FIELD(curveLfoStorage.unipolar, pmd().asBool().withName("Unipolar"));
    SC_FIELD(curveLfoStorage.useenv, pmd().asBool().withName("Use Envelope"));

    SC_FIELD(envLfoStorage.delay, envTime().withDefault(0).withName("Delay"));
    SC_FIELD(envLfoStorage.attack, envTime().withDefault(0).withName("Attack"));
    SC_FIELD(envLfoStorage.hold, envTime().withDefault(0).withName("Hold"));
    SC_FIELD(envLfoStorage.decay, envTime().withDefault(0).withName("Decay"));
    SC_FIELD(envLfoStorage.sustain, pmd().asPercent().withDefault(1.f).withName("Sustain"));
    SC_FIELD(envLfoStorage.release, envTime().withDefault(1).withName("Release"));
})

#endif // SHORTCIRCUITXT_MODULATOR_STORAGE_H
