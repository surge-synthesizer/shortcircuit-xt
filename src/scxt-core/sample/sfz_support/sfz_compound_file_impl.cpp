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

#include <set>

#include "sample/compound_file.h"
#include "sfz_parse.h"

namespace scxt::sample::compound
{
std::vector<CompoundElement> getSFZCompoundList(const fs::path &p)
{
    std::vector<CompoundElement> result;
    CompoundElement main;
    main.type = CompoundElement::INSTRUMENT;
    main.name = p.filename().replace_extension().u8string();
    main.sampleAddress = {Sample::SFZ_FILE, p, "", -1, -1, -1};
    result.push_back(main);

    auto parser = sfz_support::SFZParser();
    auto doc = parser.parse(p);
    auto sampleDir = p.parent_path();

    std::set<fs::path> samples;
    for (const auto &d : doc)
    {
        if (d.first.type == sfz_support::SFZParser::Header::Type::region ||
            d.first.type == sfz_support::SFZParser::Header::Type::group)
        {
            for (const auto &oc : d.second)
            {
                if (oc.name == "sample")
                {
                    auto sampleFileString = oc.value;
                    std::replace(sampleFileString.begin(), sampleFileString.end(), '\\', '/');
                    auto sampleFile = fs::path{sampleFileString};
                    auto samplePath = (sampleDir / sampleFile).lexically_normal();

                    if (fs::exists(samplePath))
                    {
                        samples.insert(samplePath);
                    }
                }
            }
        }
    }

    for (const auto &sp : samples)
    {
        CompoundElement main;
        main.type = CompoundElement::SAMPLE;
        main.name = sp.filename().u8string();
        if (extensionMatches(sp, ".flac"))
        {
            main.sampleAddress = {Sample::FLAC_FILE, sp, "", -1, -1, -1};
        }
        else
        {
            main.sampleAddress = {Sample::WAV_FILE, sp, "", -1, -1, -1};
        }
        result.push_back(main);
    }

    return result;
}
} // namespace scxt::sample::compound