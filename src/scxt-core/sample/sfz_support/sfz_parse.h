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

#ifndef SCXT_SRC_SCXT_CORE_SAMPLE_SFZ_SUPPORT_SFZ_PARSE_H
#define SCXT_SRC_SCXT_CORE_SAMPLE_SFZ_SUPPORT_SFZ_PARSE_H

#include <string>
#include <variant>
#include <vector>
#include <filesystem>
#include "filesystem/import.h"
#include "utils.h"
#include "configuration.h"

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
    struct OpCode
    {
        std::string name;
        // std::variant<float, std::string, bool> value; // the bool means no value
        std::string value;
    };

    typedef std::vector<OpCode> opCodes_t;
    typedef std::pair<Header, opCodes_t> section_t;
    typedef std::vector<section_t> document_t;

    std::function<void(const std::string &)> onError = [](auto s) {
        SCLOG_IF(warnings, "SFZParser error: " << s);
    };
    document_t parse(const std::string &contents);
    document_t parse(const fs::path &file);
};
} // namespace scxt::sfz_support

#endif // SHORTCIRCUITXT_SFZ_PARSE_H
