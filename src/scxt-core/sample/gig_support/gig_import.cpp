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

#include "gig_import.h"
#include "sample/import_support/import_harness.h"
#include "sample/import_support/import_mapping.h"
#include "sample/import_support/import_loop.h"
#include "sample/import_support/import_envelope.h"
#include "sample/import_support/import_filter.h"
#include "sample/import_support/import_modulation.h"
#include "sample/import_support/import_numeric.h"

#include "gig.h"

#include "engine/engine.h"
#include "messaging/messaging.h"
#include "infrastructure/md5support.h"
#include "configuration.h"

namespace scxt::gig_support
{
namespace
{
// Best-effort mapping from gig VCF type to SCXT FilterType. Anything outside
// the original GigaStudio set (the LinuxSampler extensions) becomes Other,
// which the helper renders as a bypassed LP — caller logs an unsupported note.
import_support::FilterType gigVCFTypeToFilterType(::gig::vcf_type_t t)
{
    switch (t)
    {
    case ::gig::vcf_type_lowpass:
    case ::gig::vcf_type_lowpassturbo:
        return import_support::FilterType::LP12;
    case ::gig::vcf_type_bandpass:
        return import_support::FilterType::BP12;
    case ::gig::vcf_type_highpass:
        return import_support::FilterType::HP12;
    case ::gig::vcf_type_bandreject:
        return import_support::FilterType::Notch12;
    default:
        return import_support::FilterType::Other;
    }
}

std::string gigDimensionName(::gig::dimension_t d)
{
    switch (d)
    {
    case ::gig::dimension_none:
        return "none";
    case ::gig::dimension_samplechannel:
        return "samplechannel";
    case ::gig::dimension_layer:
        return "layer";
    case ::gig::dimension_velocity:
        return "velocity";
    case ::gig::dimension_channelaftertouch:
        return "aftertouch";
    case ::gig::dimension_releasetrigger:
        return "releasetrigger";
    case ::gig::dimension_keyboard:
        return "keyboard/keyswitch";
    case ::gig::dimension_roundrobin:
        return "roundrobin";
    case ::gig::dimension_random:
        return "random";
    case ::gig::dimension_smartmidi:
        return "smartmidi";
    case ::gig::dimension_roundrobinkeyboard:
        return "roundrobinkeyboard";
    case ::gig::dimension_modwheel:
        return "modwheel";
    case ::gig::dimension_breath:
        return "breath";
    case ::gig::dimension_foot:
        return "foot";
    case ::gig::dimension_portamentotime:
        return "portamentotime";
    case ::gig::dimension_effect1:
        return "effect1";
    case ::gig::dimension_effect2:
        return "effect2";
    case ::gig::dimension_sustainpedal:
        return "sustainpedal";
    default:
        return "unknown(" + std::to_string((int)d) + ")";
    }
}

// Compute the inclusive velocity range for a given velocity-zone index within
// a region. Mirrors libgig's lookup in Region::GetDimensionRegionByValue:
//   - gig3 stores per-zone upper limits in DimensionRegion::DimensionUpperLimits
//   - gig2 stores them in DimensionRegion::VelocityUpperLimit
//   - if neither is set, zones are evenly sized (128/N)
struct VelRange
{
    int lo;
    int hi;
};
VelRange computeVelRange(::gig::Region *region, int velDimIdx, int velBitPos, int velZones,
                         int velIdx)
{
    int prevHi = -1;
    for (int k = 0; k <= velIdx; ++k)
    {
        auto *probe = region->pDimensionRegions[k << velBitPos];
        int upper = 127;
        if (probe)
        {
            if (probe->DimensionUpperLimits[velDimIdx])
                upper = probe->DimensionUpperLimits[velDimIdx];
            else if (probe->VelocityUpperLimit)
                upper = probe->VelocityUpperLimit;
            else
                upper = (k + 1) * 128 / velZones - 1;
        }
        if (k == velIdx)
            return {prevHi + 1, upper};
        prevHi = upper;
    }
    return {0, 127};
}

void importDimensionRegion(::gig::Region *region, ::gig::DimensionRegion *dr,
                           const VelRange &velRange, int grNum, engine::Engine &e,
                           const fs::path &p, const std::string &md5, ::gig::File *gig,
                           const std::map<::gig::Sample *, int> &sampleToIndex,
                           import_support::ImporterContext &ctx)
{
    auto sit = sampleToIndex.find(dr->pSample);
    if (sit == sampleToIndex.end())
        return;
    auto sidx = sit->second;

    auto sid = e.getSampleManager()->loadSampleFromGIG(p, md5, gig, -1, -1, sidx);
    if (!sid.has_value())
        return;
    e.getSampleManager()->getSample(*sid)->md5Sum = md5;

    auto zn = std::make_unique<engine::Zone>(*sid);
    zn->engine = &e;

    // DLS::Sampler::FineTune is documented as int16 cents.
    float pitchOffsetSemitones = import_support::centsToSemitones((float)dr->FineTune);

    import_support::importZoneMapping(*zn, ctx,
                                      {
                                          .rootKey = dr->UnityNote,
                                          .keyStart = region->KeyRange.low,
                                          .keyEnd = region->KeyRange.high,
                                          .velStart = velRange.lo,
                                          .velEnd = velRange.hi,
                                          .pitchOffsetSemitones = pitchOffsetSemitones,
                                      });

    if (!zn->attachToSample(*(e.getSampleManager()), 0,
                            engine::Zone::ENDPOINTS | engine::Zone::LOOP))
    {
        ctx.warn("GIG Import", "Zone unable to attach to sample; skipping");
        return;
    }

    // Sample start offset: gig stores this on the dim region (0..2000 samples).
    if (dr->SampleStartOffset > 0)
        zn->variantData.variants[0].startSample = dr->SampleStartOffset;

    // EG1 → ampeg (slot 0).
    import_support::importZoneEnvelope(*zn, ctx, 0,
                                       {
                                           .attackSeconds = (float)dr->EG1Attack,
                                           .decaySeconds = (float)dr->EG1Decay1,
                                           .sustainLevel = (float)dr->EG1Sustain / 1000.f,
                                           .releaseSeconds = (float)dr->EG1Release,
                                       });
    if (dr->EG1Decay2 > 0 && !dr->EG1InfiniteSustain)
        ctx.unsupported("GIG EG1", "two-stage decay (Decay2) collapsed to Decay1");

    // EG2 → fileg (slot 1). Only routed if a filter exists below.
    auto egFileg =
        import_support::importZoneEnvelope(*zn, ctx, 1,
                                           {
                                               .attackSeconds = (float)dr->EG2Attack,
                                               .decaySeconds = (float)dr->EG2Decay1,
                                               .sustainLevel = (float)dr->EG2Sustain / 1000.f,
                                               .releaseSeconds = (float)dr->EG2Release,
                                           });
    if (dr->EG2Decay2 > 0 && !dr->EG2InfiniteSustain)
        ctx.unsupported("GIG EG2", "two-stage decay (Decay2) collapsed to Decay1");

    // VCF → filter slot 0. GIG cutoff is a MIDI-key value; SCXT cutoff is
    // semitones offset from key 69.
    import_support::FilterHandle filterHandle;
    if (dr->VCFEnabled)
    {
        auto ft = gigVCFTypeToFilterType(dr->VCFType);
        if (ft == import_support::FilterType::Other)
            ctx.unsupported("GIG filter type",
                            "unmapped VCFType=" + std::to_string((int)dr->VCFType));
        filterHandle =
            import_support::importZoneFilter(*zn, ctx, 0,
                                             {
                                                 .type = ft,
                                                 .cutoff = (float)dr->VCFCutoff - 69.f,
                                                 .resonance = (float)dr->VCFResonance / 127.f,
                                             });

        // EG2 in gig is hardwired as the filter envelope. Depth isn't an
        // explicit field; use 2 octaves as a sensible default and surface
        // it as approximate.
        if (filterHandle.procSlot >= 0)
        {
            import_support::addImportedModRoute(
                *zn, ctx,
                {
                    .source = import_support::ImportedSource::fromEG(egFileg),
                    .target = import_support::ImportedTarget::filter(
                        filterHandle, import_support::FilterParam::Cutoff),
                    .depth = 24.f,
                });
            ctx.unsupported("GIG EG2→cutoff depth", "approximated as 24 semitones");
        }
    }

    // Loop: per-DimRgn (gig's per-sample loop fields are deprecated).
    if (dr->SampleLoops > 0 && dr->pSampleLoops)
    {
        const auto &lp = dr->pSampleLoops[0];
        import_support::LoopArgs la{
            .startSamples = (int64_t)lp.LoopStart,
            .endSamples = (int64_t)(lp.LoopStart + lp.LoopLength),
            .active = true,
        };
        switch (lp.LoopType)
        {
        case ::gig::loop_type_normal:
            la.direction = engine::Zone::FORWARD_ONLY;
            break;
        case ::gig::loop_type_bidirectional:
            la.direction = engine::Zone::ALTERNATE_DIRECTIONS;
            break;
        case ::gig::loop_type_backward:
            la.direction = engine::Zone::FORWARD_ONLY;
            zn->variantData.variants[0].playReverse = true;
            ctx.unsupported("GIG loop direction",
                            "backward loop approximated as forward+playReverse");
            break;
        default:
            la.direction = engine::Zone::FORWARD_ONLY;
            break;
        }
        import_support::importZoneLoop(*zn, ctx, 0, la);
    }

    if (dr->Pan != 0)
    {
        // -64..0..63 → -1..0..1
        zn->outputInfo.pan = dr->Pan < 0 ? (float)dr->Pan / 64.f : (float)dr->Pan / 63.f;
    }

    if (!dr->PitchTrack)
        ctx.unsupported("GIG PitchTrack=false", "pitch tracking disable not yet imported");

    ctx.addZoneToGroup(grNum, std::move(zn));
}
} // namespace

bool importGIG(const fs::path &p, engine::Engine &e, int preset)
{
    import_support::ImporterContext ctx(e, "Loading GIG '" + p.filename().u8string() + "'");
    SCLOG_IF(sampleCompoundParsers, "Loading " << p.u8string() << " ps=" << preset);

    try
    {
        auto riff = std::make_unique<RIFF::File>(p.u8string());
        auto gig = std::make_unique<::gig::File>(riff.get());
        auto md5 = infrastructure::createMD5SumFromFile(p);

        auto spc = 0;
        auto epc = gig->Instruments;

        if (preset >= 0)
        {
            spc = preset;
            epc = preset + 1;
        }

        std::map<::gig::Sample *, int> sampleToIndex;
        {
            auto smp = gig->GetFirstSample();
            int si = 0;
            while (smp)
            {
                sampleToIndex[smp] = si;
                smp = gig->GetNextSample();
                si++;
            }
        }

        for (int pc = spc; pc < epc; ++pc)
        {
            auto *gigPreset = gig->GetInstrument(pc);
            std::string pnm = "Instrument " + std::to_string(pc + 1);
            if (gigPreset->pInfo)
                pnm = gigPreset->pInfo->Name;

            // For now everything from one instrument lands in one group.
            // Phase 4 (keyswitch) will split keyboard-dim cases into
            // separate groups; phase 5 (release trigger) likewise.
            auto grNum = ctx.addGroup(pnm);

            auto *region = gigPreset->GetFirstRegion();
            while (region)
            {
                // Discover velocity and layer dims; their bit positions in the
                // packed pDimensionRegions index. Warn on every other dim type
                // so users know what we're collapsing to bit=0.
                int velDimIdx = -1, layerDimIdx = -1;
                int velBitPos = 0, layerBitPos = 0;
                {
                    int bitpos = 0;
                    for (int d = 0; d < (int)region->Dimensions; ++d)
                    {
                        const auto &dd = region->pDimensionDefinitions[d];
                        if (dd.dimension == ::gig::dimension_velocity)
                        {
                            velDimIdx = d;
                            velBitPos = bitpos;
                        }
                        else if (dd.dimension == ::gig::dimension_layer)
                        {
                            layerDimIdx = d;
                            layerBitPos = bitpos;
                        }
                        else if (dd.dimension != ::gig::dimension_none)
                        {
                            ctx.unsupported(
                                "GIG dimension",
                                gigDimensionName(dd.dimension) +
                                    " (collapsed to first zone) — see end-of-file plan");
                        }
                        bitpos += dd.bits;
                    }
                }

                int velZones =
                    (velDimIdx >= 0) ? region->pDimensionDefinitions[velDimIdx].zones : 1;
                int layerZones =
                    (layerDimIdx >= 0) ? region->pDimensionDefinitions[layerDimIdx].zones : 1;

                for (int velIdx = 0; velIdx < velZones; ++velIdx)
                {
                    VelRange velRange{region->VelocityRange.low, region->VelocityRange.high};
                    if (velDimIdx >= 0)
                        velRange = computeVelRange(region, velDimIdx, velBitPos, velZones, velIdx);

                    for (int layerIdx = 0; layerIdx < layerZones; ++layerIdx)
                    {
                        int dimIdx = (velIdx << velBitPos) | (layerIdx << layerBitPos);
                        auto *dr = region->pDimensionRegions[dimIdx];
                        if (!dr || !dr->pSample)
                            continue;

                        importDimensionRegion(region, dr, velRange, grNum, e, p, md5, gig.get(),
                                              sampleToIndex, ctx);
                    }
                }

                region = gigPreset->GetNextRegion();
            }
        }
    }
    catch (RIFF::Exception ex)
    {
        return ctx.fail("GIG Load Error", ex.Message);
    }
    catch (const SCXTError &ex)
    {
        return ctx.fail("GIG Load Error", ex.what());
    }
    catch (...)
    {
        return false;
    }
    return ctx.finish();
}
} // namespace scxt::gig_support

/*
 * GIG importer — what's done, what's next.
 *
 * Implemented (Phases 1 + 2):
 *   - Iterate every DimensionRegion in each Region, split by velocity and
 *     layer dimensions. Each (vel × layer) slice becomes one SCXT Zone.
 *   - Per-DimRgn articulation: EG1 → ampeg, EG2 → fileg + auto-route to
 *     filter cutoff (depth approximated), VCF → filter (cutoff/resonance/
 *     type), loop (forward / bidirectional / backward), pan, sample start
 *     offset, fine tune. Unknown VCF types and other gig dimensions are
 *     surfaced via ctx.unsupported.
 *
 * Phase 3 — Round robin / random.
 *   Map dimension_roundrobin and dimension_random onto SCXT zone variants
 *   (Zone::attachToSampleAtVariation, capped at scxt::maxVariantsPerZone).
 *   See SFZ pattern at sfz_import.cpp:857-892 — collapse N DimRgn cases
 *   sharing the same key/vel range into one Zone with N variants.
 *
 * Phase 4 — Keyboard (keyswitch).
 *   dimension_keyboard selects DimRgn by held key. Map to per-keyswitch
 *   Groups with KEYSWITCH_LATCH / KEYSWITCH_MOMENTARY trigger conditions
 *   (engine/group_triggers.h:54-65). One group per keyswitch zone, each
 *   carrying the zones that play under that selection.
 *
 * Phase 5 — Release trigger.
 *   dimension_releasetrigger fires DimRgn on note-off. SCXT has no native
 *   release-trigger group primitive yet — that work needs to land first.
 *   Until then, warn-and-skip via ctx.unsupported. Also note gig's
 *   SustainReleaseTrigger / NoNoteOffReleaseTrigger DimRgn fields.
 *
 * Phase 6 — LFOs.
 *   GIG has three LFOs (LFO1 amp, LFO2 cutoff, LFO3 pitch) with internal-
 *   vs-controller depth, controller assignment, wave form, phase, sync,
 *   flip phase. The SCXT importZoneLFO helper covers shape + rate; depth
 *   and target go through addImportedModRoute. See sf2_import.cpp:213-269
 *   for the LFO + modroute pattern.
 *
 * Phase 7 — Continuous controller dimensions.
 *   dimension_modwheel, _breath, _foot, _aftertouch, _CCx etc. Either map
 *   to mod routes (when the dim continuously varies a parameter) or to
 *   group trigger conditions (when it gates which case plays). Most
 *   non-obvious mapping; defer until phases 3/4 are in.
 *
 * Phase 8 — MIDI rules and real-time scripts.
 *   MidiRuleCtrlTrigger, MidiRuleLegato, MidiRuleAlternator, and the gig
 *   instrument scripts in pScriptPool. Realistically not portable; record
 *   as ctx.unsupported and move on.
 *
 * Open questions for whichever future hands pick this up:
 *   - Layer crossfade (DimensionRegion::Crossfade in/out start/end) — model
 *     as velocity-style fades on a synthetic axis, or skip?
 *   - dimension_keyboard → one Group per keyswitch bit, or one Group whose
 *     trigger-condition row distinguishes per-case zones?
 *   - EG2-to-cutoff depth — gig has no explicit field; 24 semitones is
 *     a guess. Maybe drive from VCFCutoff headroom (127 - VCFCutoff)?
 *   - dr->Gain encoding (DLS dB * 65536 * 10, negated): currently dropped;
 *     should fold into zone amplitude.
 *
 * References:
 *   - libgig gig.h (dimension_t, DimensionRegion, Region):
 *     https://download.linuxsampler.org/doc/libgig/api/gig_8h_source.html
 *   - libgig namespace docs:
 *     http://download.linuxsampler.org/doc/libgig/api/namespacegig.html
 *   - GigaStudio format overview:
 *     http://www.chickensys.com/translator/documentation/formatinfo/giga.html
 *   - LinuxSampler features:
 *     http://www.linuxsampler.org/features.html
 *   - In-tree reference: libs/sst/libgig-modified/src/tools/gigdump.cpp
 *     (most exhaustive printout of every libgig field).
 */
