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

#include "gig.h"
#include "SF.h"

#include "engine/engine.h"
#include "messaging/messaging.h"
#include "infrastructure/md5support.h"
#include "configuration.h"

namespace scxt::sf2_support
{
bool importSF2(const fs::path &p, engine::Engine &e, int preset)
{
    import_support::ImporterContext ctx(e, "Loading SF2 '" + p.filename().u8string() + "'");

    SCLOG_IF(sampleLoadAndPurge, "Loading SF2 Preset: " << p.u8string() << " preset=" << preset);

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

        for (int pc = spc; pc < epc; ++pc)
        {
            auto *preset = sf->GetPreset(pc);
            auto pnm = std::string(preset->GetName());

            for (int i = 0; i < preset->GetRegionCount(); ++i)
            {
                auto *presetRegion = preset->GetRegion(i);
                sf2::Instrument *instr = presetRegion->pInstrument;

                auto grpnum = import_support::getOrCreateGroup(ctx, instToGroup, instr, [&] {
                    return std::string(instr->GetName()) + " / " + pnm;
                });

                if (instr->pGlobalRegion)
                {
                    // TODO: Global Region
                }
                for (int j = 0; j < instr->GetRegionCount(); ++j)
                {
                    auto region = instr->GetRegion(j);
                    auto sfsamp = region->GetSample();
                    if (sfsamp == nullptr)
                        continue;

                    auto sid = e.getSampleManager()->loadSampleFromSF2(p, md5, sf.get(), pc, i, j);
                    if (!sid.has_value())
                        continue;
                    e.getSampleManager()->getSample(*sid)->md5Sum = md5;

                    auto zn = std::make_unique<engine::Zone>(*sid);
                    zn->engine = &e;

                    auto noneOr = [](auto a, auto b, auto c) {
                        if (a != sf2::NONE)
                            return a;
                        if (b != sf2::NONE)
                            return b;
                        return c;
                    };

                    auto lk = noneOr(region->loKey, presetRegion->loKey, (int)0);
                    auto hk = noneOr(region->hiKey, presetRegion->hiKey, (int)127);
                    auto lv = noneOr(region->minVel, presetRegion->minVel, 0);
                    auto hv = noneOr(region->maxVel, presetRegion->maxVel, 127);

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
                            .pitchOffsetSemitones = pitchOffsetSemitones,
                        });
                    if (!zn->attachToSample(*(e.getSampleManager())))
                        return ctx.fail("SF2 Load Error", "Can't attach to sample");

                    import_support::importZoneEnvelope(
                        *zn, ctx, 0,
                        {
                            .delaySeconds = (float)region->GetEG1PreAttackDelay(presetRegion),
                            .attackSeconds = (float)region->GetEG1Attack(presetRegion),
                            .holdSeconds = (float)region->GetEG1Hold(presetRegion),
                            .decaySeconds = (float)region->GetEG1Decay(presetRegion),
                            .sustainLevel = import_support::centibelsToLinear(
                                region->GetEG1Sustain(presetRegion)),
                            .releaseSeconds = (float)region->GetEG1Release(presetRegion),
                        });

                    auto egModHandle = import_support::importZoneEnvelope(
                        *zn, ctx, 1,
                        {
                            .delaySeconds = (float)region->GetEG2PreAttackDelay(presetRegion),
                            .attackSeconds = (float)region->GetEG2Attack(presetRegion),
                            .holdSeconds = (float)region->GetEG2Hold(presetRegion),
                            .decaySeconds = (float)region->GetEG2Decay(presetRegion),
                            .sustainLevel = import_support::centibelsToLinear(
                                region->GetEG2Sustain(presetRegion)),
                            .releaseSeconds = (float)region->GetEG2Release(presetRegion),
                        });

                    if (region->HasLoop)
                    {
                        import_support::importZoneLoop(*zn, ctx, 0,
                                                       {
                                                           .startSamples = region->LoopStart,
                                                           .endSamples = region->LoopEnd,
                                                           .active = true,
                                                       });
                    }
                    else if (presetRegion->HasLoop)
                    {
                        import_support::importZoneLoop(*zn, ctx, 0,
                                                       {
                                                           .startSamples = presetRegion->LoopStart,
                                                           .endSamples = presetRegion->LoopEnd,
                                                           .active = true,
                                                       });
                    }

                    if (auto pan = region->GetPan(presetRegion); pan != 0)
                        zn->outputInfo.pan = import_support::sf2NormalizedPan(pan);

                    auto me2p = region->GetModEnvToPitch(presetRegion);
                    auto me2f = region->GetModEnvToFilterFc(presetRegion);
                    auto v2p = region->GetVibLfoToPitch(presetRegion);

                    /*
                     * The SF2 spec says anything above 20khz is a flat response
                     * and that the filter cutoff is specified in cents (so really
                     * 100 * midi key in 12TET). That's a midi key between 135 and 136,
                     * so lets be safe a shave a touch
                     */
                    auto fc = region->GetInitialFilterFc(presetRegion);
                    auto fq = region->GetInitialFilterQ(presetRegion);
                    bool makeFilter = (me2f != 0);
                    import_support::FilterHandle filterHandle;
                    if (fc >= 0 && fc < 134 * 100 || makeFilter)
                    {
                        // This only runs with audio thread stopped
                        filterHandle = import_support::importZoneFilter(
                            *zn, ctx, 0,
                            {
                                // SF2 spec: -12 dB/oct 2nd-order resonant LP.
                                .type = import_support::FilterType::LP12,
                                .cutoff = (float)((fc / 100.0) - 69.0),
                                .resonance = (float)std::clamp((fq / 100.0), 0., 1.),
                            });
                    }

                    import_support::LFOHandle vibLfoHandle;
                    if (v2p > 0)
                    {
                        // Use LFO1 as the vibrato LFO
                        vibLfoHandle = import_support::importZoneLFO(
                            *zn, ctx, 0,
                            {
                                .shape = modulation::ModulatorStorage::LFO_SINE,
                                .rate =
                                    (float)((region->GetFreqVibLfo(presetRegion) + 3637.63165623) *
                                            0.01 / 12.0),
                            });

                        if (region->GetDelayVibLfo() > -11999)
                        {
                            SCLOG_IF(sampleCompoundParsers, "Unimplemented: Curve Delay");
                            // zn->modulatorStorage[0].curveLfoStorage.useenv = true;
                            // zn->modulatorStorage[0].curveLfoStorage.delay = something;
                            // zn->modulatorStorage[0].curveLfoStorage.attack = 0.0;
                            // zn->modulatorStorage[0].curveLfoStorage.release = 1.0;
                        }
                    }

                    // SF2 stores mod-env-to-pitch / vib-LFO-to-pitch in cents,
                    // mod-env-to-cutoff in cents; pass target-native depths
                    // (semitones) and let addImportedModRoute normalize.
                    if (me2p != 0)
                    {
                        import_support::addImportedModRoute(
                            *zn, ctx,
                            {
                                .source = import_support::ImportedSource::fromEG(egModHandle),
                                .target = import_support::ImportedTarget::pitch(),
                                .depth = (float)me2p / 100.f,
                            });
                    }
                    if (me2f != 0)
                    {
                        import_support::addImportedModRoute(
                            *zn, ctx,
                            {
                                .source = import_support::ImportedSource::fromEG(egModHandle),
                                .target = import_support::ImportedTarget::filter(
                                    filterHandle, import_support::FilterParam::Cutoff),
                                .depth = (float)me2f / 100.f,
                            });
                    }
                    if (v2p > 0)
                    {
                        import_support::addImportedModRoute(
                            *zn, ctx,
                            {
                                .source = import_support::ImportedSource::fromLFO(vibLfoHandle),
                                .target = import_support::ImportedTarget::pitch(),
                                .depth = (float)v2p / 100.f,
                            });
                    }

                    for (size_t mi = 0; mi < region->modulators.size(); mi++)
                    {
                        const auto &m = region->modulators[mi];

                        if (m.ModAmtSrcOper.Type != ::sf2::Modulator::NO_CONTROLLER ||
                            m.ModAmtSrcOper.MidiPalete)
                        {
                            ctx.unsupported("SF2 modulator", "secondary source not supported");
                            continue;
                        }

                        std::optional<import_support::ImportedSource> src;
                        if (m.ModSrcOper.MidiPalete)
                        {
                            int cc = m.ModSrcOper.Index;
                            if (cc < 0 || cc > 127)
                                continue;
                            // Use the dedicated mod wheel source for CC1.
                            if (cc == 1)
                                src = import_support::ImportedSource::modWheel();
                            else
                                src = import_support::ImportedSource::midiCC(cc);
                        }
                        else
                        {
                            using M = ::sf2::Modulator;
                            switch (m.ModSrcOper.Type)
                            {
                            case M::NOTE_ON_VELOCITY:
                                src = import_support::ImportedSource::velocity();
                                break;
                            case M::NOTE_ON_KEY_NUMBER:
                                src = import_support::ImportedSource::keyTrack();
                                break;
                            case M::CHANNEL_PRESSURE:
                                src = import_support::ImportedSource::channelAT();
                                break;
                            case M::PITCH_WHEEL:
                                src = import_support::ImportedSource::pitchBend();
                                break;
                            case M::POLY_PRESSURE:
                            case M::PITCH_WHEEL_SENSITIVITY:
                            case M::LINK:
                            case M::NO_CONTROLLER:
                            default:
                                break;
                            }
                            if (!src)
                            {
                                ctx.unsupported("SF2 modulator source",
                                                "type=" + std::to_string(m.ModSrcOper.Type));
                                continue;
                            }
                        }

                        // Map destination and convert amount to SCXT target-native depth.
                        std::optional<import_support::ImportedTarget> tgt;
                        float depth = 0.f;
                        int16_t amount = (int16_t)m.ModAmount;
                        switch (m.ModDestOper)
                        {
                        case ::sf2::INITIAL_FILTER_FC:
                            if (filterHandle.procSlot < 0)
                            {
                                ctx.unsupported("SF2 modulator dest",
                                                "INITIAL_FILTER_FC with no filter");
                                continue;
                            }
                            tgt = import_support::ImportedTarget::filter(
                                filterHandle, import_support::FilterParam::Cutoff);
                            depth = (float)amount / 100.f; // cents → semitones
                            break;
                        case ::sf2::PAN:
                            tgt = import_support::ImportedTarget::zonePan();
                            depth = (float)amount / 1000.f; // 0.1% → -1..1
                            break;
                        case ::sf2::FINE_TUNE:
                            tgt = import_support::ImportedTarget::pitch();
                            depth = (float)amount / 100.f; // cents → semitones
                            break;
                        case ::sf2::COARSE_TUNE:
                            tgt = import_support::ImportedTarget::pitch();
                            depth = (float)amount; // already semitones
                            break;
                        default:
                            ctx.unsupported("SF2 modulator dest",
                                            "generator=" + std::to_string(m.ModDestOper));
                            continue;
                        }

                        if (m.ModTransOper != 0)
                            ctx.unsupported("SF2 modulator transform",
                                            "non-linear transform; treated as linear");

                        import_support::addImportedModRoute(*zn, ctx,
                                                            {
                                                                .source = *src,
                                                                .target = *tgt,
                                                                .depth = depth,
                                                            });
                    }

                    if (region->modLfoToFilterFc != 0 || region->modLfoToPitch != 0)
                    {
                        SCLOG_IF(sampleCompoundParsers,
                                 "Unsupported: Modulation LFO: Delay="
                                     << ::sf2::ToSeconds(region->delayModLfo)
                                     << "s, Frequency=" << ::sf2::ToHz(region->freqModLfo)
                                     << "Hz, LFO to Volume=" << (region->modLfoToVolume / 10)
                                     << "dB"
                                     << ", LFO to Filter Cutoff=" << region->modLfoToFilterFc
                                     << ", LFO to Pitch=" << region->modLfoToPitch);
                    }

                    if (region->exclusiveClass)
                        SCLOG_IF(sampleCompoundParsers, "Unsupported: Exclusive Class : "
                                                            << region->exclusiveClass << " in "
                                                            << instr->GetName() << " region " << i);

                    ctx.addZoneToGroup(grpnum, std::move(zn));
                }
            }
        }
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