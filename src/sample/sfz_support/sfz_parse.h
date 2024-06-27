/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#ifndef SCXT_SRC_SAMPLE_SFZ_SUPPORT_SFZ_PARSE_H
#define SCXT_SRC_SAMPLE_SFZ_SUPPORT_SFZ_PARSE_H

#include <string>
#include <variant>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include "filesystem/import.h"

namespace scxt::sfz_support
{
struct SFZParser
{
    struct Header
    {
        enum Type
        {
            region,
            group,
            control,
            global,
            curve,
            effect,
            master,
            midi,
            sample,
            unknown
        } type{unknown};
        std::string name;
    };

    typedef std::unordered_map<std::string, std::string> opCodes_t;
    typedef std::pair<Header, opCodes_t> section_t;
    typedef std::vector<section_t> document_t;

    document_t parse(const std::string &contents);
    document_t parse(const fs::path &file);
};
} // namespace scxt::sfz_support

#endif // SHORTCIRCUITXT_SFZ_PARSE_H
