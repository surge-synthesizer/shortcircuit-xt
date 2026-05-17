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

#ifndef SCXT_SRC_SCXT_CORE_SAMPLE_IMPORT_SUPPORT_IMPORT_MODULATION_H
#define SCXT_SRC_SCXT_CORE_SAMPLE_IMPORT_SUPPORT_IMPORT_MODULATION_H

#include <optional>

#include "engine/zone.h"
#include "import_envelope.h"
#include "import_filter.h"
#include "import_lfo.h"

namespace scxt::import_support
{
class ImporterContext;

// Depths are always expressed in the *target's* native (unmodulated) unit.
// e.g. a Pitch target's depth is in semitones; a FilterParam(Cutoff) target's
// depth is in semitones; an EGTime target's depth is in seconds; a ZonePan
// depth is in -1..1. Each importer is responsible for converting its
// format-native scaling.

enum class ImportedSourceKind
{
    LFO,
    EG,
    MidiCC, // index = CC number 0..127
    Velocity,
    ReleaseVelocity,
    ChannelAT,
    PolyAT,
    ModWheel, // dedicated high-level source, not CC1
    PitchBend,
    KeyTrack,
};

struct ImportedSource
{
    ImportedSourceKind kind{};
    int index{0};

    static ImportedSource fromLFO(LFOHandle h) { return {ImportedSourceKind::LFO, h.lfoSlot}; }
    static ImportedSource fromEG(EGHandle h) { return {ImportedSourceKind::EG, h.egSlot}; }
    static ImportedSource midiCC(int cc) { return {ImportedSourceKind::MidiCC, cc}; }
    static ImportedSource velocity() { return {ImportedSourceKind::Velocity, 0}; }
    static ImportedSource releaseVelocity() { return {ImportedSourceKind::ReleaseVelocity, 0}; }
    static ImportedSource channelAT() { return {ImportedSourceKind::ChannelAT, 0}; }
    static ImportedSource polyAT() { return {ImportedSourceKind::PolyAT, 0}; }
    static ImportedSource modWheel() { return {ImportedSourceKind::ModWheel, 0}; }
    static ImportedSource pitchBend() { return {ImportedSourceKind::PitchBend, 0}; }
    static ImportedSource keyTrack() { return {ImportedSourceKind::KeyTrack, 0}; }
};

enum class FilterParam
{
    Cutoff,
    Resonance,
};

enum class EGWhich
{
    Delay,
    Attack,
    Hold,
    Decay,
    Sustain,
    Release,
};

enum class ImportedTargetKind
{
    Pitch,
    FilterParam,
    EGTime, // procSlot = egSlot, paramIdx = (int)EGWhich
    ZonePan,
    ZoneAmplitude,
};

struct ImportedTarget
{
    ImportedTargetKind kind{};
    int procSlot{0};
    int paramIdx{0};

    static ImportedTarget pitch() { return {ImportedTargetKind::Pitch}; }
    static ImportedTarget filter(FilterHandle h, FilterParam p);
    static ImportedTarget egTime(int egSlot, EGWhich w)
    {
        return {ImportedTargetKind::EGTime, egSlot, (int)w};
    }
    static ImportedTarget zonePan() { return {ImportedTargetKind::ZonePan}; }
    static ImportedTarget zoneAmplitude() { return {ImportedTargetKind::ZoneAmplitude}; }
};

struct ImportedModRoute
{
    ImportedSource source;
    std::optional<ImportedSource> via{}; // optional secondary source (multiplied)
    ImportedTarget target;
    float depth{0.f};
    bool active{true};
};

// Returns the row index used, or -1 if no free row.
int addImportedModRoute(engine::Zone &, ImporterContext &, const ImportedModRoute &);
} // namespace scxt::import_support

#endif
