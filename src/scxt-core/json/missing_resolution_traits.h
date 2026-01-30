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
