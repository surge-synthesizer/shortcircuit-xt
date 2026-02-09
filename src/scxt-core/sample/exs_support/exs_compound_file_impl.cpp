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

#include "sample/compound_file.h"
#include "sample/exs_support/exs_import.h"

namespace scxt::sample::compound
{
std::vector<CompoundElement> getEXSCompoundList(const fs::path &p)
{
    auto elts = exs_support::getEXSCompoundElements(p);

    std::vector<CompoundElement> result;

    for (const auto &e : elts)
    {
        CompoundElement res;
        res.type = CompoundElement::Type::SAMPLE;
        res.name = e.name;

        res.sampleAddress = {Sample::EXS_FILE, p, "", -1, -1, e.sampleIndex};
        result.push_back(res);
    }

    return result;
}

} // namespace scxt::sample::compound
