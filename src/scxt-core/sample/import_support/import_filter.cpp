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

#include "import_filter.h"
#include "import_harness.h"
#include "engine/engine.h"
#include "messaging/messaging.h"

namespace scxt::import_support
{
namespace
{
// Resolved low-level filter config that the helper writes to the processor.
// The int param values are indices into the per-model passbands/slopes/
// drives/submodels arrays the FiltersPlusPlus<Model> constructor builds,
// not raw enum values from filtersplusplus. Today only ipPassband and
// ipSlope are used here; drives stay at "Standard" (index 0).
struct ResolvedFilter
{
    dsp::processor::ProcessorType processorType{dsp::processor::ProcessorType::proct_CytomicSVF};
    int passbandIdx{0};
    int slopeIdx{0};
    bool bypassWithMixZero{false}; // for the Other fallback
};

// Per-model passband index assignments. These mirror the FiltersPlusPlus
// constructors in sst-effects:
//   VemberClassic: explicit order LP=0, HP=1, BP=2, Notch=3, Allpass=4
//   CytomicSVF: sorted by Passband enum value — LP=0, HP=1, BP=2, Notch=3,
//               Peak=4, Allpass=5, LowShelf=6, Bell=7, HighShelf=8
//   Comb: a single Comb passband
namespace cytomic
{
constexpr int LP = 0;
constexpr int HP = 1;
constexpr int BP = 2;
constexpr int Notch = 3;
constexpr int Peak = 4;
constexpr int Allpass = 5;
} // namespace cytomic
namespace vember
{
constexpr int LP = 0;
constexpr int HP = 1;
constexpr int BP = 2;
constexpr int Notch = 3;
// Slopes (sorted by enum value): 12dB=0, 24dB=1.
constexpr int Slope_12dB = 0;
constexpr int Slope_24dB = 1;
} // namespace vember

ResolvedFilter resolveFilterType(FilterType t)
{
    using Proc = dsp::processor::ProcessorType;
    switch (t)
    {
    // --- exact 12dB Cytomic-backed mappings ---
    case FilterType::LP12:
        return {Proc::proct_CytomicSVF, cytomic::LP, 0, false};
    case FilterType::HP12:
        return {Proc::proct_CytomicSVF, cytomic::HP, 0, false};
    case FilterType::BP12:
        return {Proc::proct_CytomicSVF, cytomic::BP, 0, false};
    case FilterType::Notch12:
        return {Proc::proct_CytomicSVF, cytomic::Notch, 0, false};
    case FilterType::Peak:
        return {Proc::proct_CytomicSVF, cytomic::Peak, 0, false};
    case FilterType::Allpass:
        return {Proc::proct_CytomicSVF, cytomic::Allpass, 0, false};

    // --- exact 24dB Vember-backed mappings ---
    case FilterType::LP24:
        return {Proc::proct_VemberClassic, vember::LP, vember::Slope_24dB, false};
    case FilterType::HP24:
        return {Proc::proct_VemberClassic, vember::HP, vember::Slope_24dB, false};
    case FilterType::BP24:
        return {Proc::proct_VemberClassic, vember::BP, vember::Slope_24dB, false};
    case FilterType::Notch24:
        return {Proc::proct_VemberClassic, vember::Notch, vember::Slope_24dB, false};

    // --- approximated slopes (silently rounded to the closest available) ---
    case FilterType::LP6:
        return {Proc::proct_CytomicSVF, cytomic::LP, 0, false}; // 6dB → 12dB
    case FilterType::HP6:
        return {Proc::proct_CytomicSVF, cytomic::HP, 0, false};
    case FilterType::BP6:
        return {Proc::proct_CytomicSVF, cytomic::BP, 0, false};
    case FilterType::LP18:
    case FilterType::LP36:
    case FilterType::LP48:
        return {Proc::proct_VemberClassic, vember::LP, vember::Slope_24dB, false};
    case FilterType::HP18:
    case FilterType::HP36:
    case FilterType::HP48:
        return {Proc::proct_VemberClassic, vember::HP, vember::Slope_24dB, false};

    case FilterType::Comb:
        return {Proc::proct_comb, 0, 0, false};

    case FilterType::Other:
        return {Proc::proct_CytomicSVF, cytomic::LP, 0, true};
    }
    // Defensive: unknown enum value
    return {Proc::proct_CytomicSVF, cytomic::LP, 0, true};
}

// Int param slot indices in processorStorage[].intParams, matching the
// FiltersPlusPlus<>::IntParams enum order.
constexpr int kIpPassband = 1;
constexpr int kIpSlope = 2;

} // namespace

FilterHandle importZoneFilter(engine::Zone &zone, ImporterContext &ctx, int procSlot,
                              const FilterArgs &a)
{
    if (!a.type.has_value())
    {
        ctx.software_error("importZoneFilter", "type is required");
        return {};
    }

    auto resolved = resolveFilterType(*a.type);
    {
        auto guard = ctx.getEngine().getMessageController()->threadingChecker.bypassChecksInScope();
        zone.setProcessorType(procSlot, resolved.processorType);
    }

    auto &ps = zone.processorStorage[procSlot];
    ps.intParams[kIpPassband] = resolved.passbandIdx;
    ps.intParams[kIpSlope] = resolved.slopeIdx;

    // CytomicSVF / VemberClassic / Comb all share fpCutoffL=0, fpResonance=2
    // through the FiltersPlusPlus base class.
    if (a.cutoff)
        ps.floatParams[0] = *a.cutoff;
    if (a.resonance)
        ps.floatParams[2] = *a.resonance;

    if (resolved.bypassWithMixZero)
        ps.mix = 0.f;

    return {procSlot, resolved.processorType};
}
} // namespace scxt::import_support
