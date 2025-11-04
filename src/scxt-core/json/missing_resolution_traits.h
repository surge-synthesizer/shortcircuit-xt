/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_CORE_JSON_MISSING_RESOLUTION_TRAITS_H
#define SCXT_SRC_SCXT_CORE_JSON_MISSING_RESOLUTION_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "stream.h"
#include "extensions.h"

#include "engine/missing_resolution.h"

namespace scxt::json
{
SC_STREAMDEF(scxt::engine::MissingResolutionWorkItem, SC_FROM({
                 v = {{"i", from.missingID}, {"p", from.path.u8string()}, {"m", from.isMultiUsed}};
             }),
             SC_TO({
                 findIf(v, "i", to.missingID);
                 findOrSet(v, "m", false, to.isMultiUsed);
                 std::string fp;
                 findIf(v, "p", fp);
                 to.path = fs::path(fs::u8path(fp));
             }));
} // namespace scxt::json

#endif // MISSING_RESOLUTION_TRAITS_H
