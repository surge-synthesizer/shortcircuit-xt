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

#include "import_generator.h"
#include "import_harness.h"
#include "engine/engine.h"
#include "engine/zone.h"
#include "messaging/messaging.h"

namespace scxt::import_support
{

namespace
{
enum class GenKind
{
    Sine,
    Saw,
    Square,
    Triangle,
    Noise,
    Silence,
    Unknown,
};

GenKind parseKind(const std::string &name)
{
    if (name == "*sine")
        return GenKind::Sine;
    if (name == "*saw")
        return GenKind::Saw;
    if (name == "*square")
        return GenKind::Square;
    if (name == "*triangle" || name == "*tri")
        return GenKind::Triangle;
    if (name == "*noise")
        return GenKind::Noise;
    if (name == "*silence")
        return GenKind::Silence;
    return GenKind::Unknown;
}

// EllipticBlepWaveforms (proct_osc_EBWaveforms) param indices, mirroring
// the enums in libs/sst/sst-effects/.../EllipticBlepWaveforms.h. These are
// stable across SCXT versions but kept local so the importer doesn't pull
// in the processor header.
constexpr int kEbIpWaveform = 0;
constexpr int kEbFpPulseWidth = 2;
constexpr int kEbWaveSaw = 0;
constexpr int kEbWavePulse = 2;
constexpr int kEbWaveTri = 3;
constexpr int kEbWaveNearSin = 4;
} // namespace

bool isVirtualSampleName(const std::string &name) { return parseKind(name) != GenKind::Unknown; }

bool installVirtualSample(engine::Zone &zone, ImporterContext &ctx, const std::string &name)
{
    auto kind = parseKind(name);
    if (kind == GenKind::Unknown)
        return false;

    // SCXT plays zones without attached samples — voice runs processors on a
    // zero input. For `*silence` that's all we need; for the others we install
    // a generator processor in slot 0 which produces the audio.
    if (kind == GenKind::Silence)
        return true;

    auto procType = (kind == GenKind::Noise) ? dsp::processor::ProcessorType::proct_osc_tiltnoise
                                             : dsp::processor::ProcessorType::proct_osc_EBWaveforms;
    {
        auto guard = ctx.getEngine().getMessageController()->threadingChecker.bypassChecksInScope();
        zone.setProcessorType(0, procType);
    }
    auto &ps = zone.processorStorage[0];
    ps.mix = 1.f; // generator should be fully wet

    if (procType == dsp::processor::ProcessorType::proct_osc_EBWaveforms)
    {
        int wave = kEbWaveSaw;
        switch (kind)
        {
        case GenKind::Sine:
            wave = kEbWaveNearSin;
            break;
        case GenKind::Saw:
            wave = kEbWaveSaw;
            break;
        case GenKind::Square:
            wave = kEbWavePulse;
            ps.floatParams[kEbFpPulseWidth] = 0.5f;
            break;
        case GenKind::Triangle:
            wave = kEbWaveTri;
            break;
        default:
            break;
        }
        ps.intParams[kEbIpWaveform] = wave;
    }
    return true;
}
} // namespace scxt::import_support
