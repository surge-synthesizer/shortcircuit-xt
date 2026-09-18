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

#ifndef SCXT_SRC_SCXT_CORE_SAMPLE_IMPORT_SUPPORT_IMPORT_VARIANT_FOLD_H
#define SCXT_SRC_SCXT_CORE_SAMPLE_IMPORT_SUPPORT_IMPORT_VARIANT_FOLD_H

#include <memory>
#include <vector>

#include "engine/zone.h"

namespace scxt::import_support
{
class ImporterContext;

// A zone the format says is one of an alternating set, with the sequence
// position it named. -1 means the format only gives document order.
struct FoldableZone
{
    std::unique_ptr<engine::Zone> zone;
    int sequencePosition{-1};
};

/*
 * Several formats express "play one of these in turn" as a stack of zones
 * covering the same ground, which shortcircuit spells as one zone with several
 * variants. This folds the first spelling into the second.
 *
 * Zones sharing root key and key/velocity geometry collapse into the variants
 * of the first of them, which takes the supplied playback mode; a geometry
 * with only one zone comes back untouched. Past maxVariantsPerZone the surplus
 * spills into a further zone rather than being dropped.
 *
 * The mode is the caller's because the formats disagree about it: the
 * multisample zone-logic attribute means FORWARD_RR, while SFZ's lorand/hirand
 * stacks are a dice roll per note and want a random mode (#2470).
 */
std::vector<std::unique_ptr<engine::Zone>> foldZonesToVariants(ImporterContext &,
                                                               std::vector<FoldableZone> &&,
                                                               engine::Zone::VariantPlaybackMode);
} // namespace scxt::import_support

#endif
