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

#include "import_modulation.h"
#include "import_harness.h"
#include "modulation/voice_matrix.h"

namespace scxt::import_support
{
using SR = voice::modulation::MatrixConfig::SourceIdentifier;
using TG = voice::modulation::MatrixConfig::TargetIdentifier;

static int filterParamIdx(dsp::processor::ProcessorType type, FilterParam p)
{
    // FiltersPlusPlus<*> (CytomicSVF, VemberClassic, comb) all share the
    // same float-param layout: fpCutoffL=0, fpCutoffR=1, fpResonance=2.
    return (p == FilterParam::Resonance) ? 2 : 0;
}

ImportedTarget ImportedTarget::filter(FilterHandle h, FilterParam p)
{
    return {ImportedTargetKind::FilterParam, h.procSlot, filterParamIdx(h.type, p)};
}

// (max-min) for each target, used to normalize depth. SCXT's mod matrix
// applies `target += depth * (max-min) * source`, so callers pass `r.depth`
// in *target-native* units (semitones, dB, etc.) and this helper divides by
// the target range to land in the normalized depth the matrix expects.
// EGTime is in log2(seconds) units — a "seconds delta at full source"
// can't be expressed cleanly in a linear log2 mod; callers should pass
// log2-seconds delta or expect non-linear behavior. Flagged with TODO.
static float targetRange(const ImportedTarget &t)
{
    switch (t.kind)
    {
    case ImportedTargetKind::Pitch:
        return 192.f; // -96..96 semis
    case ImportedTargetKind::FilterParam:
        // paramIdx here is the FiltersPlusPlus float-param slot: 0=fpCutoffL,
        // 1=fpCutoffR, 2=fpResonance.
        if (t.paramIdx == 0 || t.paramIdx == 1)
            return 130.f; // -60..70 semis (asAudibleFrequency)
        return 1.f;       // resonance 0..1 (asPercent)
    case ImportedTargetKind::EGTime:
        return 1.f; // params are normalized 0..1 (25-second exp mapping)
    case ImportedTargetKind::ZonePan:
        return 2.f; // -1..1
    case ImportedTargetKind::ZoneAmplitude:
        return 72.f; // -36..36 dB
    }
    return 1.f;
}

int addImportedModRoute(engine::Zone &zone, ImporterContext &ctx, const ImportedModRoute &r)
{
    int rowIdx = -1;
    auto &routes = zone.routingTable.routes;
    for (int i = 0; i < (int)routes.size(); ++i)
    {
        if (routes[i].hasDefaultValues())
        {
            rowIdx = i;
            break;
        }
    }
    if (rowIdx < 0)
    {
        ctx.software_error("addImportedModRoute", "no free routing row available");
        return -1;
    }

    auto &row = routes[rowIdx];

    using MEnd = voice::modulation::MatrixEndpoints;
    using MidiS = MEnd::Sources::MIDISources;
    using KeyS = MEnd::Sources::KeyAndPitchSources;
    using CCs = decltype(MEnd::Sources::midiCCSources);

    auto resolveSource = [&](const ImportedSource &is) -> std::optional<SR> {
        switch (is.kind)
        {
        case ImportedSourceKind::LFO:
            return MEnd::Sources::lfoSource(is.index);
        case ImportedSourceKind::EG:
            return MEnd::Sources::egSource(is.index);
        case ImportedSourceKind::MidiCC:
            if (is.index < 0 || is.index > 127)
                return std::nullopt;
            return CCs::ccSourceA((uint32_t)is.index);
        case ImportedSourceKind::Velocity:
            return MidiS::velocityA;
        case ImportedSourceKind::ReleaseVelocity:
            return MidiS::releaseVelocityA;
        case ImportedSourceKind::ChannelAT:
            return MidiS::chanATA;
        case ImportedSourceKind::PolyAT:
            return MidiS::polyATA;
        case ImportedSourceKind::ModWheel:
            return MidiS::modWheelA;
        case ImportedSourceKind::PitchBend:
            return MidiS::pbpm1A;
        case ImportedSourceKind::KeyTrack:
            return KeyS::keyTrackA;
        }
        return std::nullopt;
    };

    auto srcRes = resolveSource(r.source);
    if (!srcRes)
    {
        ctx.software_error("addImportedModRoute", "could not resolve source");
        return -1;
    }
    row.source = *srcRes;

    if (r.via.has_value())
    {
        if (auto viaRes = resolveSource(*r.via); viaRes.has_value())
            row.sourceVia = *viaRes;
        else
            ctx.software_error("addImportedModRoute", "could not resolve via");
    }

    TG tgt{};
    switch (r.target.kind)
    {
    case ImportedTargetKind::Pitch:
        tgt = MEnd::MappingTarget::pitchOffsetA;
        break;
    case ImportedTargetKind::FilterParam:
        tgt = MEnd::ProcessorTarget::floatParam(r.target.procSlot, r.target.paramIdx);
        break;
    case ImportedTargetKind::EGTime:
    {
        using EGT = MEnd::EGTarget;
        const auto slot = (uint32_t)r.target.procSlot;
        switch ((EGWhich)r.target.paramIdx)
        {
        case EGWhich::Delay:
            tgt = EGT::delayA(slot);
            break;
        case EGWhich::Attack:
            tgt = EGT::attackA(slot);
            break;
        case EGWhich::Hold:
            tgt = EGT::holdA(slot);
            break;
        case EGWhich::Decay:
            tgt = EGT::decayA(slot);
            break;
        case EGWhich::Sustain:
            tgt = EGT::sustainA(slot);
            break;
        case EGWhich::Release:
            tgt = EGT::releaseA(slot);
            break;
        }
        break;
    }
    case ImportedTargetKind::ZonePan:
        tgt = MEnd::MappingTarget::panA;
        break;
    case ImportedTargetKind::ZoneAmplitude:
        tgt = MEnd::MappingTarget::ampA;
        break;
    }
    row.target = tgt;

    // Normalize: importer passes depth in target-native units; the matrix
    // applies `depth * (max-min) * source`, so divide here.
    const float range = targetRange(r.target);
    row.depth = (range > 0.f) ? r.depth / range : r.depth;
    row.active = r.active;

    zone.onRoutingChanged();
    return rowIdx;
}
} // namespace scxt::import_support
