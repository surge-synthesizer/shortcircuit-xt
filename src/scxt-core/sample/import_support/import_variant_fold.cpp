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

#include <algorithm>

#include "import_variant_fold.h"
#include "import_harness.h"

namespace scxt::import_support
{
namespace
{
// Two zones fold only if a voice could not tell which it landed on: the same
// key and velocity ground, fades included. Root key is deliberately not part of
// it - it says how the sample is pitched once chosen, not where it is chosen,
// and a stack whose members disagree about it is the usual library typo.
bool sameGeometry(const engine::Zone &a, const engine::Zone &b)
{
    return a.mapping.keyboardRange == b.mapping.keyboardRange &&
           a.mapping.velocityRange == b.mapping.velocityRange;
}
} // namespace

std::vector<std::unique_ptr<engine::Zone>>
foldZonesToVariants(ImporterContext &ctx, std::vector<FoldableZone> &&candidates,
                    engine::Zone::VariantPlaybackMode mode)
{
    // an explicit sequence reorders the set; a partial one is meaningless, so
    // it takes the whole set naming its position to move anything
    bool allSequenced = std::all_of(candidates.begin(), candidates.end(),
                                    [](const auto &c) { return c.sequencePosition >= 0; });
    if (allSequenced)
    {
        std::stable_sort(candidates.begin(), candidates.end(), [](const auto &a, const auto &b) {
            return a.sequencePosition < b.sequencePosition;
        });
    }

    const auto &manager = *ctx.getEngine().getSampleManager();

    std::vector<std::unique_ptr<engine::Zone>> out;
    for (auto &c : candidates)
    {
        if (!c.zone)
            continue;

        // the most recent zone of this geometry is the one still filling up
        auto host = std::find_if(out.rbegin(), out.rend(),
                                 [&c](const auto &z) { return sameGeometry(*z, *c.zone); });

        if (host == out.rend())
        {
            out.push_back(std::move(c.zone));
            continue;
        }

        int nv{0};
        while (nv < engine::Zone::maxVariantsPerZone && (*host)->variantData.variants[nv].active)
            ++nv;

        if (nv == engine::Zone::maxVariantsPerZone)
        {
            ctx.unsupported("round robin beyond " +
                                std::to_string(engine::Zone::maxVariantsPerZone) + " variants",
                            (*host)->getName());
            out.push_back(std::move(c.zone));
            continue;
        }

        // the host's mapping now pitches this sample, so carry the difference on
        // the variant rather than transposing it. Root key moves through the
        // host's keytrack, the zone's own offset one for one
        const auto &hm = (*host)->mapping;
        const auto &cm = c.zone->mapping;

        auto variant = c.zone->variantData.variants[0];
        variant.pitchOffset +=
            (hm.rootKey - cm.rootKey) * hm.tracking + cm.pitchOffset - hm.pitchOffset;

        (*host)->variantData.variantPlaybackMode = mode;
        (*host)->insertVariant(nv, variant, manager);
    }

    return out;
}
} // namespace scxt::import_support
