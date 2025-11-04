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

#include "sst/basic-blocks/modulators/StepLFO.h"

#include "utils.h"
#include "configuration.h"
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
    float dly{0.0}, a{0.0}, h{0.0}, d{0.0}, s{1.0}, r{0.5};

    /*
     * -1..1 values for a shape
     */
    float aShape{0}, dShape{0}, rShape{0};

    /*
     * Gate mode (only used by group)
     */
    bool gateGroupEGOnAnyPlaying{false};
};

using StepLFOStorage = sst::basic_blocks::modulators::StepLFO<scxt::blockSize>::Storage;

struct CurveLFOStorage
{
    float deform{0.f}, angle{0.f};

    float delay{0.f}, attack{0.f}, release{0.f};
    bool unipolar{false};
    bool useenv{false};
};

struct EnvLFOStorage
{
    float delay{0.f}, attack{0.f}, hold{0.f}, decay{0.f}, sustain{1.f}, release{0.f};
    float aShape{0.f}, dShape{0.f}, rShape{0.f};
    float rateMul{0.f};
    bool loop{false};
};

struct PhasorStorage
{
    enum SyncMode
    {
        SONGPOS,
        VOICEPOS
    } syncMode{VOICEPOS};
    DECLARE_ENUM_STRING(SyncMode);

    enum Division
    {
        NOTE,
        TRIPLET,
        DOTTED
    } division{NOTE};
    DECLARE_ENUM_STRING(Division);

    int32_t numerator{1}, denominator{4};
};

struct RandomStorage
{
    enum Style
    {
        UNIFORM_01,
        UNIFORM_BIPOLAR,
        NORMAL,
        HALF_NORMAL
    } style{UNIFORM_01};
    DECLARE_ENUM_STRING(Style);
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
        LFO_SAW_TRI_RAMP,
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
    float amplitude{1.f}; // linear - and mod target only
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

    bool modulatorConsistent{true};
};

struct MiscSourceStorage
{
    std::array<modulators::PhasorStorage, scxt::phasorsPerGroupOrZone> phasors;
    std::array<modulators::RandomStorage, scxt::randomsPerGroupOrZone> randoms;
};

inline double secondsToNormalizedEnvTime(double s)
{
    using rp = sst::basic_blocks::modulators::TwentyFiveSecondExp;
    // OK so its S = exp(A + X (B-A)) - C)/D
    // D S + C = exp(A + X (B-a))
    // log(D S + C) = A + X (B-A)
    // (log (D S + C) - A) / (B - A) = X

    auto logb = std::log(std::max(rp::D * s + rp::C, 0.0000001));
    auto ncs = (logb - rp::A) / (rp::B - rp::A);
    return std::clamp(ncs, 0., 1.);
}
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
    SC_FIELD(dly, envTime().withDefault(0.f).withName("Delay"));
    SC_FIELD(a, envTime().withName("Attack"));
    SC_FIELD(h, envTime().withDefault(0.f).withName("Hold"));
    SC_FIELD(d, envTime().withName("Decay"));
    SC_FIELD(s, pmd().asPercent().withDefault(1.f).withName("Sustain"));
    SC_FIELD(r, envTime().withDefault(0.5).withName("Release"));
    SC_FIELD(aShape, pmd().asPercentBipolar().withName("Attack Shape"));
    SC_FIELD(dShape, pmd().asPercentBipolar().withName("Decay Shape"));
    SC_FIELD(rShape, pmd().asPercentBipolar().withName("Release Shape"));
    SC_FIELD(gateGroupEGOnAnyPlaying,
             pmd().asOnOffBool().withName("Gated if ungated voices sounding"));
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
                                     {modulation::ModulatorStorage::LFO_SAW_TRI_RAMP, "SAW-RMP"},
                                     {modulation::ModulatorStorage::LFO_PULSE, "SQR-TRI"},
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
    SC_FIELD(temposync, pmd().asBool().withName("Temposync"));
    SC_FIELD(rate, pmd().asLfoRate().withName("Rate"));
    SC_FIELD(amplitude,
             pmd().asPercent().withName("Amplitude").withSupportsMultiplicativeModulation());
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
    SC_FIELD(curveLfoStorage.angle, pmd().asPercentBipolar().withName("Angle"));
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
    SC_FIELD(envLfoStorage.aShape,
             pmd().asPercentBipolar().withDefault(0).withName("Attack Shape"));
    SC_FIELD(envLfoStorage.dShape, pmd().asPercentBipolar().withDefault(0).withName("Decay Shape"));
    SC_FIELD(envLfoStorage.rShape,
             pmd().asPercentBipolar().withDefault(0).withName("Release Shape"));
    SC_FIELD(envLfoStorage.rateMul, pmd()
                                        .asFloat()
                                        .withRange(-5, 5)
                                        .withATwoToTheBFormatting(1, 1, "x")
                                        .withDecimalPlaces(4)
                                        .withDefault(0.0)
                                        .withFeature(pmd::Features::BELOW_ONE_IS_INVERSE_FRACTION)
                                        .withFeature(pmd::Features::ALLOW_FRACTIONAL_TYPEINS)
                                        .withName("Rate Multiplier"));
    SC_FIELD(envLfoStorage.loop, pmd().asBool().withDefault(0.0).withName("Loop"));
});

SC_DESCRIBE(scxt::modulation::modulators::PhasorStorage,
            SC_FIELD(numerator, pmd()
                                    .asInt()
                                    .withLinearScaleFormatting("")
                                    .withDefault(1)
                                    .withName("Numerator")
                                    .withRange(1, 64));
            SC_FIELD(denominator, pmd()
                                      .asInt()
                                      .withLinearScaleFormatting("")
                                      .withDefault(4)
                                      .withName("Denominator")
                                      .withRange(1, 64));)

#endif // SHORTCIRCUITXT_MODULATOR_STORAGE_H
