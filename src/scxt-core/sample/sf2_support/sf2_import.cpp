/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "sf2_import.h"
#include "sample/import_support/import_harness.h"
#include "sample/import_support/import_mapping.h"
#include "sample/import_support/import_loop.h"
#include "sample/import_support/import_envelope.h"
#include "sample/import_support/import_filter.h"
#include "sample/import_support/import_lfo.h"
#include "sample/import_support/import_modulation.h"
#include "sample/import_support/import_numeric.h"

#include <set>

#include "gig.h"
#include "SF.h"

#include "engine/engine.h"
#include "messaging/messaging.h"
#include "modulation/modulator_storage.h"
#include "infrastructure/md5support.h"
#include "configuration.h"

namespace scxt::sf2_support
{
namespace
{
namespace is = import_support;
using CurveID = modulation::ModulationCurves::CurveIdentifier;
using SFMod = ::sf2::Modulator;

// the SF2 default velocity to attenuation modulator is roughly amp = vel^2
constexpr float sf2DefaultVelocitySensitivity{0.75f};

enum SF2ControllerType
{
    CT_LINEAR = 0,
    CT_CONCAVE = 1,
    CT_CONVEX = 2,
    CT_SWITCH = 3
};

// an SF2 controller as an SCXT source, a curve on it, and a sign on the depth
struct DecodedSource
{
    is::ImportedSource source;
    std::optional<CurveID> curve;
    float sign{1.f};

    // scxt keytrack is (key - root) / 12 but SF2 key sources are absolute, so a
    // key source is keyScale * keytrack + keyOffset
    bool isKey{false};
    float keyScale{0.f}, keyOffset{0.f};

    bool plain() const { return !curve.has_value() && sign > 0 && !isKey; }
};

std::optional<DecodedSource> decodeSource(const SFMod &m, int rootKey, std::string &why)
{
    DecodedSource res;
    bool unipolarNative{true};

    if (m.MidiPalete)
    {
        if (m.Index < 0 || m.Index > 127)
        {
            why = "CC index out of range";
            return std::nullopt;
        }
        res.source =
            (m.Index == 1) ? is::ImportedSource::modWheel() : is::ImportedSource::midiCC(m.Index);
    }
    else
    {
        switch (m.Index)
        {
        case SFMod::NOTE_ON_VELOCITY:
            res.source = is::ImportedSource::velocity();
            break;
        case SFMod::NOTE_ON_KEY_NUMBER:
            res.source = is::ImportedSource::keyTrack();
            res.isKey = true;
            break;
        case SFMod::POLY_PRESSURE:
            res.source = is::ImportedSource::polyAT();
            break;
        case SFMod::CHANNEL_PRESSURE:
            res.source = is::ImportedSource::channelAT();
            break;
        case SFMod::PITCH_WHEEL:
            res.source = is::ImportedSource::pitchBend();
            unipolarNative = false;
            break;
        case SFMod::NO_CONTROLLER:
            why = "no-controller source";
            return std::nullopt;
        case SFMod::PITCH_WHEEL_SENSITIVITY:
            why = "pitch wheel sensitivity source";
            return std::nullopt;
        case SFMod::LINK:
            why = "linked modulators";
            return std::nullopt;
        default:
            why = "unknown source " + std::to_string(m.Index);
            return std::nullopt;
        }
    }

    if (res.isKey)
    {
        if (m.Type != CT_LINEAR)
        {
            why = "non-linear key number source";
            return std::nullopt;
        }
        auto u0 = rootKey / 127.f;
        auto uScale = 12.f / 127.f;
        if (m.Polarity)
        {
            res.keyScale = 2 * uScale;
            res.keyOffset = 2 * u0 - 1;
        }
        else
        {
            res.keyScale = uScale;
            res.keyOffset = u0;
        }
        if (m.Direction)
        {
            res.keyScale = -res.keyScale;
            res.keyOffset = m.Polarity ? -res.keyOffset : 1 - res.keyOffset;
        }
        return res;
    }

    if (!unipolarNative)
    {
        // pitch bend is already -1..1
        if (m.Type != CT_LINEAR)
        {
            why = "non-linear pitch wheel source";
            return std::nullopt;
        }
        if (m.Polarity)
        {
            res.sign = m.Direction ? -1.f : 1.f;
            return res;
        }
        if (m.Direction)
        {
            why = "unipolar reversed pitch wheel source";
            return std::nullopt;
        }
        res.curve = 'unip';
        return res;
    }

    if (m.Polarity)
    {
        if (m.Type != CT_LINEAR)
            why = "bipolar non-linear source treated as linear";
        res.curve = 'bip ';
        res.sign = m.Direction ? -1.f : 1.f;
        return res;
    }

    switch (m.Type)
    {
    case CT_LINEAR:
        if (m.Direction)
            res.curve = '1-x ';
        break;
    case CT_CONCAVE:
        res.curve = m.Direction ? 'f01E' : 'r01L';
        break;
    case CT_CONVEX:
        res.curve = m.Direction ? 'f01L' : 'r01E';
        break;
    case CT_SWITCH:
        res.curve = m.Direction ? 'cmnh' : 'cmph';
        break;
    default:
        why = "unknown controller type " + std::to_string(m.Type);
        return std::nullopt;
    }
    return res;
}

// what the modulator's output drives
enum class DestKind
{
    Pitch,
    FilterCutoff,
    FilterResonance,
    Pan,
    Amplitude,
    EGTime,
    EGSustain,
};

// some destinations are depths of an LFO or envelope, which then carries the signal
enum class Carrier
{
    None,
    VibLFO,
    ModLFO,
    ModEnv
};

struct DestSpec
{
    DestKind kind;
    Carrier carrier{Carrier::None};
    float scale{1.f}; // SF2 amount to target units
    int eg{0};
    is::EGWhich which{is::EGWhich::Attack};
};

std::optional<DestSpec> decodeDest(int gen)
{
    using E = is::EGWhich;
    switch (gen)
    {
    case ::sf2::INITIAL_FILTER_FC:
        return DestSpec{DestKind::FilterCutoff, Carrier::None, 0.01f};
    case ::sf2::INITIAL_FILTER_Q:
        return DestSpec{DestKind::FilterResonance, Carrier::None, 0.01f};
    case ::sf2::PAN:
        return DestSpec{DestKind::Pan, Carrier::None, 1.f / 500.f};
    case ::sf2::INITIAL_ATTENUATION:
        return DestSpec{DestKind::Amplitude, Carrier::None, -0.1f};
    case ::sf2::FINE_TUNE:
        return DestSpec{DestKind::Pitch, Carrier::None, 0.01f};
    case ::sf2::COARSE_TUNE:
        return DestSpec{DestKind::Pitch, Carrier::None, 1.f};
    case ::sf2::VIB_LFO_TO_PITCH:
        return DestSpec{DestKind::Pitch, Carrier::VibLFO, 0.01f};
    case ::sf2::MOD_LFO_TO_PITCH:
        return DestSpec{DestKind::Pitch, Carrier::ModLFO, 0.01f};
    case ::sf2::MOD_LFO_TO_FILTER_FC:
        return DestSpec{DestKind::FilterCutoff, Carrier::ModLFO, 0.01f};
    case ::sf2::MOD_LFO_TO_VOLUME:
        return DestSpec{DestKind::Amplitude, Carrier::ModLFO, 0.1f};
    case ::sf2::MOD_ENV_TO_PITCH:
        return DestSpec{DestKind::Pitch, Carrier::ModEnv, 0.01f};
    case ::sf2::MOD_ENV_TO_FILTER_FC:
        return DestSpec{DestKind::FilterCutoff, Carrier::ModEnv, 0.01f};
    case ::sf2::DELAY_VOL_ENV:
        return DestSpec{DestKind::EGTime, Carrier::None, 1.f, 0, E::Delay};
    case ::sf2::ATTACK_VOL_ENV:
        return DestSpec{DestKind::EGTime, Carrier::None, 1.f, 0, E::Attack};
    case ::sf2::HOLD_VOL_ENV:
        return DestSpec{DestKind::EGTime, Carrier::None, 1.f, 0, E::Hold};
    case ::sf2::DECAY_VOL_ENV:
        return DestSpec{DestKind::EGTime, Carrier::None, 1.f, 0, E::Decay};
    case ::sf2::RELEASE_VOL_ENV:
        return DestSpec{DestKind::EGTime, Carrier::None, 1.f, 0, E::Release};
    case ::sf2::DELAY_MOD_ENV:
        return DestSpec{DestKind::EGTime, Carrier::None, 1.f, 1, E::Delay};
    case ::sf2::ATTACK_MOD_ENV:
        return DestSpec{DestKind::EGTime, Carrier::None, 1.f, 1, E::Attack};
    case ::sf2::HOLD_MOD_ENV:
        return DestSpec{DestKind::EGTime, Carrier::None, 1.f, 1, E::Hold};
    case ::sf2::DECAY_MOD_ENV:
        return DestSpec{DestKind::EGTime, Carrier::None, 1.f, 1, E::Decay};
    case ::sf2::RELEASE_MOD_ENV:
        return DestSpec{DestKind::EGTime, Carrier::None, 1.f, 1, E::Release};
    case ::sf2::SUSTAIN_MOD_ENV:
        return DestSpec{DestKind::EGSustain, Carrier::None, -0.001f, 1};
    }
    return std::nullopt;
}

bool isDefaultPitchBendModulator(const ::sf2::ModulatorItem &m)
{
    // scxt pitch bends natively, so this would bend twice
    return !m.ModSrcOper.MidiPalete && m.ModSrcOper.Index == SFMod::PITCH_WHEEL &&
           m.ModDestOper == ::sf2::FINE_TUNE && !m.ModAmtSrcOper.MidiPalete &&
           m.ModAmtSrcOper.Index == SFMod::PITCH_WHEEL_SENSITIVITY;
}

struct PlannedRoute
{
    DecodedSource src;
    std::optional<is::ImportedSource> via;
    DestSpec dest;
    float amount{0.f};
};

// normalized env time delta of scaling `seconds` by `timecents`
float envTimeDelta(double seconds, double timecents)
{
    auto n = [](double s) { return modulation::secondsToNormalizedEnvTime(s); };
    return (float)(n(seconds * std::pow(2.0, timecents / 1200.0)) - n(seconds));
}
} // namespace

bool importSF2(const fs::path &p, engine::Engine &e, int preset)
{
    import_support::ImporterContext ctx(e, "Loading SF2 '" + p.filename().u8string() + "'");

    SCLOG_IF(sampleLoadAndPurge, "Loading SF2 Preset: " << p.u8string() << " preset=" << preset);

    std::set<std::pair<std::string, std::string>> noted;
    auto note = [&](const std::string &cat, const std::string &det) {
        if (noted.insert({cat, det}).second)
            ctx.unsupported(cat, det);
    };

    try
    {
        auto riff = std::make_unique<RIFF::File>(p.u8string());
        auto sf = std::make_unique<sf2::File>(riff.get());
        auto md5 = infrastructure::createMD5SumFromFile(p);

        auto spc = 0;
        auto epc = sf->GetPresetCount();

        if (preset >= 0)
        {
            spc = preset;
            epc = preset + 1;
        }

        std::map<sf2::Instrument *, int> instToGroup;
        std::map<std::pair<sf2::Instrument *, uint>, int32_t> exclusiveIds;
        auto &partConfig = ctx.getPart().configuration;
        auto lastExclusiveId = partConfig.numExclusiveGroups;
        std::set<int> sf2Groups, groupsWithVelocityAttenuation;

        for (int pc = spc; pc < epc; ++pc)
        {
            auto *preset = sf->GetPreset(pc);
            auto pnm = std::string(preset->GetName());

            for (int i = 0; i < preset->GetRegionCount(); ++i)
            {
                auto *presetRegion = preset->GetRegion(i);
                sf2::Instrument *instr = presetRegion->pInstrument;

                for (int j = 0; j < instr->GetRegionCount(); ++j)
                {
                    auto region = instr->GetRegion(j);
                    auto sfsamp = region->GetSample();
                    if (sfsamp == nullptr)
                        continue;

                    // A preset zone's key/vel range constrains the instrument zone's rather
                    // than replacing it, so the zone is the intersection of the two. An
                    // empty intersection means this preset zone doesn't reach this
                    // instrument zone at all, so no zone is created.
                    auto orDefault = [](auto a, auto d) { return a != sf2::NONE ? a : d; };

                    auto lk =
                        std::max(orDefault(region->loKey, 0), orDefault(presetRegion->loKey, 0));
                    auto hk = std::min(orDefault(region->hiKey, 127),
                                       orDefault(presetRegion->hiKey, 127));
                    auto lv =
                        std::max(orDefault(region->minVel, 0), orDefault(presetRegion->minVel, 0));
                    auto hv = std::min(orDefault(region->maxVel, 127),
                                       orDefault(presetRegion->maxVel, 127));

                    if (lk > hk || lv > hv)
                        continue;

                    auto sid = e.getSampleManager()->loadSampleFromSF2(p, md5, sf.get(), pc, i, j);
                    if (!sid.has_value())
                        continue;
                    e.getSampleManager()->getSample(*sid)->md5Sum = md5;

                    // scxt chokes between groups, so each exclusive-class zone gets its own
                    int grpnum{-1};
                    if (region->exclusiveClass == 0)
                    {
                        grpnum = import_support::getOrCreateGroup(ctx, instToGroup, instr, [&] {
                            return std::string(instr->GetName()) + " / " + pnm;
                        });
                    }
                    else
                    {
                        auto [it, isNew] =
                            exclusiveIds.try_emplace({instr, region->exclusiveClass}, 0);
                        if (isNew)
                            it->second = ++lastExclusiveId;
                        grpnum = ctx.addGroup(std::string(instr->GetName()) + " / " + pnm + " / " +
                                              sfsamp->GetName());
                        ctx.getPart().getGroup(grpnum)->outputInfo.exclusiveGroup = it->second;
                    }
                    sf2Groups.insert(grpnum);

                    auto zn = std::make_unique<engine::Zone>(*sid);
                    zn->engine = &e;

                    // SF2 pitch correction is a *signed* char in cents.
                    auto pcv = static_cast<int>(static_cast<int8_t>(sfsamp->PitchCorrection));
                    auto pitchOffsetSemitones =
                        import_support::centsToSemitones((float)pcv) +
                        import_support::centsToSemitones(
                            (float)(region->GetCoarseTune(presetRegion) * 100 +
                                    region->GetFineTune(presetRegion)));

                    int rootKey = (region->overridingRootKey >= 0) ? region->overridingRootKey
                                                                   : sfsamp->OriginalPitch;
                    import_support::importZoneMapping(
                        *zn, ctx,
                        {
                            .rootKey = rootKey,
                            .keyStart = lk,
                            .keyEnd = hk,
                            .velStart = lv,
                            .velEnd = hv,
                            .tracking = region->GetScaleTuning(presetRegion) / 100.f,
                            .pitchOffsetSemitones = pitchOffsetSemitones,
                        });
                    if (!zn->attachToSample(*(e.getSampleManager())))
                        return ctx.fail("SF2 Load Error", "Can't attach to sample");

                    if (region->keynum >= 0)
                        note("SF2 generator", "fixed key number");
                    if (region->velocity >= 0)
                        note("SF2 generator", "fixed velocity");
                    if (region->GetReverbEffectsSend(presetRegion) > 0 ||
                        region->GetChorusEffectsSend(presetRegion) > 0)
                        note("SF2 generator", "reverb and chorus sends have no fixed destination");

                    {
                        auto &var = zn->variantData.variants[0];
                        auto so = (int64_t)region->startAddrsOffset +
                                  (int64_t)region->startAddrsCoarseOffset * 32768;
                        auto eo = (int64_t)region->endAddrsOffset +
                                  (int64_t)region->endAddrsCoarseOffset * 32768;
                        if ((so != 0 || eo != 0) && var.endSample > 1)
                        {
                            auto len = var.endSample;
                            var.startSample = std::clamp(var.startSample + so, (int64_t)0, len - 1);
                            var.endSample =
                                std::clamp(var.endSample + eo, var.startSample + 1, len);
                        }
                    }

                    // SF2 keynum scaling pivots on key 60 but scxt keytrack on the root
                    auto atRoot = [rootKey](double seconds, int tcPerKey) {
                        return seconds * std::pow(2.0, tcPerKey * (60 - rootKey) / 1200.0);
                    };

                    auto volHoldK = region->GetKeynumToVolEnvHold(presetRegion);
                    auto volDecayK = region->GetKeynumToVolEnvDecay(presetRegion);
                    auto modHoldK = region->GetKeynumToModEnvHold(presetRegion);
                    auto modDecayK = region->GetKeynumToModEnvDecay(presetRegion);

                    // delay attack hold decay release
                    double egTimes[2][5] = {{region->GetEG1PreAttackDelay(presetRegion),
                                             region->GetEG1Attack(presetRegion),
                                             atRoot(region->GetEG1Hold(presetRegion), volHoldK),
                                             atRoot(region->GetEG1Decay(presetRegion), volDecayK),
                                             region->GetEG1Release(presetRegion)},
                                            {region->GetEG2PreAttackDelay(presetRegion),
                                             region->GetEG2Attack(presetRegion),
                                             atRoot(region->GetEG2Hold(presetRegion), modHoldK),
                                             atRoot(region->GetEG2Decay(presetRegion), modDecayK),
                                             region->GetEG2Release(presetRegion)}};
                    auto egTime = [&](int eg, is::EGWhich w) -> double {
                        switch (w)
                        {
                        case is::EGWhich::Delay:
                            return egTimes[eg][0];
                        case is::EGWhich::Attack:
                            return egTimes[eg][1];
                        case is::EGWhich::Hold:
                            return egTimes[eg][2];
                        case is::EGWhich::Decay:
                            return egTimes[eg][3];
                        case is::EGWhich::Release:
                            return egTimes[eg][4];
                        default:
                            return 0.0;
                        }
                    };

                    import_support::importZoneEnvelope(
                        *zn, ctx, 0,
                        {
                            .delaySeconds = (float)egTimes[0][0],
                            .attackSeconds = (float)egTimes[0][1],
                            .holdSeconds = (float)egTimes[0][2],
                            .decaySeconds = (float)egTimes[0][3],
                            .sustainLevel = import_support::centibelsToLinear(
                                region->GetEG1Sustain(presetRegion)),
                            .releaseSeconds = (float)egTimes[0][4],
                        });

                    // mod env sustain is a decrease in 0.1% units, not centibels
                    auto egModHandle = import_support::importZoneEnvelope(
                        *zn, ctx, 1,
                        {
                            .delaySeconds = (float)egTimes[1][0],
                            .attackSeconds = (float)egTimes[1][1],
                            .holdSeconds = (float)egTimes[1][2],
                            .decaySeconds = (float)egTimes[1][3],
                            .sustainLevel = 1.f - region->GetEG2Sustain(presetRegion) / 1000.f,
                            .releaseSeconds = (float)egTimes[1][4],
                        });

                    if (region->HasLoop)
                    {
                        import_support::importZoneLoop(
                            *zn, ctx, 0,
                            {
                                .mode = region->sampleModes == 3
                                            ? engine::Zone::LoopMode::LOOP_WHILE_GATED
                                            : engine::Zone::LoopMode::LOOP_DURING_VOICE,
                                .startSamples = region->LoopStart,
                                .endSamples = region->LoopEnd,
                                .active = true,
                            });
                    }

                    if (auto pan = region->GetPan(presetRegion); pan != 0)
                        zn->outputInfo.pan = import_support::sf2NormalizedPan(pan);

                    if (auto attn = region->GetInitialAttenuation(presetRegion); attn > 0)
                        zn->outputInfo.amplitude =
                            import_support::dBToCubicAttenuation((float)(-attn / 10.0));

                    auto me2p = region->GetModEnvToPitch(presetRegion);
                    auto me2f = region->GetModEnvToFilterFc(presetRegion);
                    auto v2p = region->GetVibLfoToPitch(presetRegion);
                    auto ml2p = region->GetModLfoToPitch(presetRegion);
                    auto ml2f = region->GetModLfoToFilterFc(presetRegion);
                    auto ml2v = (int)region->GetModLfoToVolume(presetRegion);

                    // decoded first so we know which filter and LFOs to build
                    std::vector<PlannedRoute> planned;
                    float cutoffOffset{0.f};
                    auto planModulators = [&](const std::vector<::sf2::ModulatorItem> &mods) {
                        for (const auto &m : mods)
                        {
                            if (isDefaultPitchBendModulator(m))
                                continue;

                            auto dest = decodeDest(m.ModDestOper);
                            if (!dest)
                            {
                                note("SF2 modulator destination",
                                     "generator " + std::to_string(m.ModDestOper));
                                continue;
                            }

                            std::string why;
                            auto src = decodeSource(m.ModSrcOper, rootKey, why);
                            if (!why.empty())
                                note("SF2 modulator", why);
                            if (!src)
                                continue;

                            if (m.ModTransOper != 0)
                                note("SF2 modulator", "absolute value transform treated as linear");

                            PlannedRoute pr{*src, std::nullopt, *dest, (float)(int16_t)m.ModAmount};

                            bool hasAmountSource = m.ModAmtSrcOper.MidiPalete ||
                                                   m.ModAmtSrcOper.Index != SFMod::NO_CONTROLLER;
                            if (hasAmountSource)
                            {
                                std::string awhy;
                                auto amt = decodeSource(m.ModAmtSrcOper, rootKey, awhy);
                                // the matrix curves source * via, so both must be unshaped
                                if (amt && amt->plain() && src->plain())
                                    pr.via = amt->source;
                                else
                                    note("SF2 modulator",
                                         "shaped amount source dropped; primary source kept");
                            }

                            if (dest->carrier != Carrier::None)
                            {
                                if (!src->plain() || pr.via)
                                {
                                    note("SF2 modulator",
                                         "shaped controller scaling an LFO or envelope depth");
                                    continue;
                                }
                            }

                            if (src->isKey)
                            {
                                auto offset = pr.amount * dest->scale * src->keyOffset;
                                switch (dest->kind)
                                {
                                case DestKind::Pitch:
                                    zn->mapping.pitchOffset += offset;
                                    break;
                                case DestKind::FilterCutoff:
                                    cutoffOffset += offset;
                                    break;
                                case DestKind::Pan:
                                    zn->mapping.pan =
                                        std::clamp(zn->mapping.pan + offset, -1.f, 1.f);
                                    break;
                                case DestKind::Amplitude:
                                    zn->mapping.amplitude += offset;
                                    break;
                                default:
                                    note("SF2 modulator",
                                         "key number to this destination is not supported");
                                    continue;
                                }
                            }

                            if (!m.ModSrcOper.MidiPalete &&
                                m.ModSrcOper.Index == SFMod::NOTE_ON_VELOCITY &&
                                m.ModDestOper == ::sf2::INITIAL_ATTENUATION)
                            {
                                groupsWithVelocityAttenuation.insert(grpnum);
                            }
                            planned.push_back(pr);
                        }
                    };
                    planModulators(region->modulators);
                    planModulators(presetRegion->modulators);

                    auto usesCarrier = [&](Carrier c) {
                        for (auto &r : planned)
                            if (r.dest.carrier == c)
                                return true;
                        return false;
                    };
                    auto needsFilter = [&]() {
                        for (auto &r : planned)
                            if (r.dest.kind == DestKind::FilterCutoff ||
                                r.dest.kind == DestKind::FilterResonance)
                                return true;
                        return false;
                    };

                    /*
                     * The SF2 spec says anything above 20khz is a flat response
                     * and that the filter cutoff is specified in cents (so really
                     * 100 * midi key in 12TET). That's a midi key between 135 and 136,
                     * so lets be safe a shave a touch
                     */
                    auto fc = region->GetInitialFilterFc(presetRegion);
                    auto fq = region->GetInitialFilterQ(presetRegion);
                    import_support::FilterHandle filterHandle;
                    if ((fc >= 0 && fc < 134 * 100) || me2f != 0 || ml2f != 0 || needsFilter() ||
                        cutoffOffset != 0.f)
                    {
                        // This only runs with audio thread stopped
                        filterHandle = import_support::importZoneFilter(
                            *zn, ctx, 0,
                            {
                                // SF2 spec: -12 dB/oct 2nd-order resonant LP.
                                .type = import_support::FilterType::LP12,
                                .cutoff = (float)((fc / 100.0) - 69.0) + cutoffOffset,
                                .resonance = (float)std::clamp((fq / 100.0), 0., 1.),
                            });
                    }

                    // SF2 LFOs are triangles; rates are log2 Hz in scxt
                    auto makeLFO = [&](int slot, double hz, double delaySeconds) {
                        auto h = import_support::importZoneLFO(
                            *zn, ctx, slot,
                            {
                                .shape = modulation::ModulatorStorage::LFO_TRI,
                                .rate = (float)std::log2(std::max(hz, 0.001)),
                            });
                        if (delaySeconds > 0.0015)
                        {
                            auto &cls = zn->modulatorStorage[slot].curveLfoStorage;
                            cls.useenv = true;
                            cls.delay = (float)modulation::secondsToNormalizedEnvTime(delaySeconds);
                            cls.attack = 0.f;
                        }
                        return h;
                    };

                    import_support::LFOHandle vibLfoHandle, modLfoHandle;
                    if (v2p != 0 || usesCarrier(Carrier::VibLFO))
                        vibLfoHandle = makeLFO(0, region->GetFreqVibLfo(presetRegion),
                                               region->GetDelayVibLfo(presetRegion));
                    if (ml2p != 0 || ml2f != 0 || ml2v != 0 || usesCarrier(Carrier::ModLFO))
                        modLfoHandle = makeLFO(1, region->GetFreqModLfo(presetRegion),
                                               region->GetDelayModLfo(presetRegion));

                    auto hasFreeRow = [&]() {
                        for (auto &r : zn->routingTable.routes)
                            if (r.hasDefaultValues())
                                return true;
                        return false;
                    };
                    auto addRoute = [&](const import_support::ImportedModRoute &r) {
                        if (!hasFreeRow())
                        {
                            note("SF2 modulator", "more modulators than scxt matrix rows");
                            return;
                        }
                        import_support::addImportedModRoute(*zn, ctx, r);
                    };

                    auto carrierSource = [&](Carrier c) {
                        switch (c)
                        {
                        case Carrier::VibLFO:
                            return import_support::ImportedSource::fromLFO(vibLfoHandle);
                        case Carrier::ModLFO:
                            return import_support::ImportedSource::fromLFO(modLfoHandle);
                        default:
                            return import_support::ImportedSource::fromEG(egModHandle);
                        }
                    };

                    auto filterTarget = [&](import_support::FilterParam fp) {
                        return import_support::ImportedTarget::filter(filterHandle, fp);
                    };

                    // generator depths; SF2 cents become semitones
                    if (me2p != 0)
                        addRoute({.source = carrierSource(Carrier::ModEnv),
                                  .target = import_support::ImportedTarget::pitch(),
                                  .depth = me2p / 100.f});
                    if (me2f != 0)
                        addRoute({.source = carrierSource(Carrier::ModEnv),
                                  .target = filterTarget(import_support::FilterParam::Cutoff),
                                  .depth = me2f / 100.f});
                    if (v2p != 0)
                        addRoute({.source = carrierSource(Carrier::VibLFO),
                                  .target = import_support::ImportedTarget::pitch(),
                                  .depth = v2p / 100.f});
                    if (ml2p != 0)
                        addRoute({.source = carrierSource(Carrier::ModLFO),
                                  .target = import_support::ImportedTarget::pitch(),
                                  .depth = ml2p / 100.f});
                    if (ml2f != 0)
                        addRoute({.source = carrierSource(Carrier::ModLFO),
                                  .target = filterTarget(import_support::FilterParam::Cutoff),
                                  .depth = ml2f / 100.f});
                    if (ml2v != 0)
                        addRoute({.source = carrierSource(Carrier::ModLFO),
                                  .target = import_support::ImportedTarget::zoneAmplitude(),
                                  .depth = ml2v / 10.f});

                    // keynum to hold and decay, linearised around the root's time
                    auto keyToEG = [&](int eg, is::EGWhich w, int tcPerKey) {
                        if (tcPerKey == 0)
                            return;
                        addRoute({.source = import_support::ImportedSource::keyTrack(),
                                  .target = import_support::ImportedTarget::egTime(eg, w),
                                  .depth = envTimeDelta(egTime(eg, w), -12.0 * tcPerKey)});
                    };
                    keyToEG(0, is::EGWhich::Hold, volHoldK);
                    keyToEG(0, is::EGWhich::Decay, volDecayK);
                    keyToEG(1, is::EGWhich::Hold, modHoldK);
                    keyToEG(1, is::EGWhich::Decay, modDecayK);

                    for (auto &pr : planned)
                    {
                        import_support::ImportedModRoute r;
                        r.source = pr.src.source;
                        r.via = pr.via;
                        r.curve = pr.src.curve;

                        auto sign = pr.src.isKey ? pr.src.keyScale : pr.src.sign;
                        auto depth = pr.amount * pr.dest.scale * sign;

                        if (pr.dest.carrier != Carrier::None)
                        {
                            r.via = pr.src.source;
                            r.source = carrierSource(pr.dest.carrier);
                            r.curve = std::nullopt;
                        }

                        switch (pr.dest.kind)
                        {
                        case DestKind::Pitch:
                            r.target = import_support::ImportedTarget::pitch();
                            break;
                        case DestKind::FilterCutoff:
                            r.target = filterTarget(import_support::FilterParam::Cutoff);
                            break;
                        case DestKind::FilterResonance:
                            r.target = filterTarget(import_support::FilterParam::Resonance);
                            break;
                        case DestKind::Pan:
                            r.target = import_support::ImportedTarget::zonePan();
                            break;
                        case DestKind::Amplitude:
                            r.target = import_support::ImportedTarget::zoneAmplitude();
                            break;
                        case DestKind::EGTime:
                            r.target =
                                import_support::ImportedTarget::egTime(pr.dest.eg, pr.dest.which);
                            // amount is timecents at full source
                            depth =
                                sign * envTimeDelta(egTime(pr.dest.eg, pr.dest.which), pr.amount);
                            break;
                        case DestKind::EGSustain:
                            r.target = import_support::ImportedTarget::egTime(pr.dest.eg,
                                                                              is::EGWhich::Sustain);
                            break;
                        }
                        r.depth = depth;
                        addRoute(r);
                    }

                    ctx.addZoneToGroup(grpnum, std::move(zn));
                }
            }
        }

        // SF2's default velocity to attenuation, unless the file brings its own
        for (auto g : sf2Groups)
        {
            ctx.getPart().getGroup(g)->outputInfo.velocitySensitivity =
                groupsWithVelocityAttenuation.count(g) ? 0.f : sf2DefaultVelocitySensitivity;
        }
        partConfig.numExclusiveGroups = lastExclusiveId;
    }
    catch (RIFF::Exception ex)
    {
        return ctx.fail("SF2 Load Error", ex.Message);
    }
    catch (const SCXTError &ex)
    {
        return ctx.fail("SF2 Load Error", ex.what());
    }
    catch (...)
    {
        return false;
    }
    return ctx.finish();
}
} // namespace scxt::sf2_support
