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

#include "sfz_parse.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace scxt::sfz_support
{

SFZParser::document_t SFZParser::parse(const std::string &s)
{
    enum ParseState
    {
        NOTHING,
        IN_SLCOM,
        IN_MLCOM,
        IN_REGION,
    } state{NOTHING};

    document_t res;

    auto lookAheadForOpcode = [&](auto from, auto &opcode) {
        opcode.clear();
        while (from < s.size() && s[from] != ' ' && s[from] != '\n' && s[from] != '\r')
        {
            if (s[from] == '=')
                return true;
            opcode += s[from];
            from++;
        }
        return false;
    };

    auto readUntilEndOfKey = [&](auto from, bool isSample) -> std::pair<std::string, int> {
        std::ostringstream oss;
        std::string tmp;
        auto mightBeOpcode{false};
        while (from < s.size())
        {
            auto c = s[from];
            auto cn = from < s.size() - 1 ? s[from + 1] : c;
            if (c == ' ')
            {
                mightBeOpcode = true;
                oss << c;
            }
            else if (c == '/' && cn == '*')
            {
                return {oss.str(), from};
            }
            else if (c == '/' && (!isSample || cn == '/'))
            {
                return {oss.str(), from};
            }
            else if (c == '<')
            {
                return {oss.str(), from};
            }
            else if (c == '\n' || c == '\r')
            {
                return {oss.str(), from};
            }
            else if (mightBeOpcode && lookAheadForOpcode(from, tmp))
            {
                return {oss.str(), from - 1};
            }
            else
            {
                oss << c;
            }
            from++;
        }
        return {oss.str(), from};
    };

    auto stripTrailingAndQuotes = [](const auto &s) {
        auto ep = s.size() - 1;
        while (ep >= 0 && (s[ep] == ' ' || s[ep] == '\n' || s[ep] == '\r'))
        {
            ep--;
        }
        auto res = s.substr(0, ep + 1);
        if (!res.empty())
        {
            if (res.size() > 1 && res[0] == '"' && res.back() == '"')
            {
                res = res.substr(1, res.size() - 2);
            }
        }
        return res;
    };

    auto e = s.size();
    std::string opcode;
    int line{1};
    for (auto cp = 0; cp < e; ++cp)
    {
        auto c = s[cp];
        auto cn = (cp < e - 1) ? s[cp + 1] : c;
        if (c == '\n')
            line++;

        switch (state)
        {
        case NOTHING:
        {
            if (c == '/' && cn == '*')
            {
                state = IN_MLCOM;
                cp++;
            }
            else if (c == '/')
            {
                state = IN_SLCOM;
            }
            else if (c == '<')
            {
                state = IN_REGION;
                res.push_back({{}, {}});
            }
            else if (c == ' ' || c == '\n' || c == '\r')
            {
            }
            else if (lookAheadForOpcode(cp, opcode))
            {
                cp += opcode.size() + 1;
                auto [key, pos] = readUntilEndOfKey(cp, opcode == "sample");
                cp = pos - 1;
                OpCode oc;
                oc.name = opcode;
                oc.value = stripTrailingAndQuotes(key);
                // After discussions on SFZ discord, SFZ with opcodes
                // before a header are invalid; and those opcodes can be
                // dropped
                if (!res.empty())
                    res.back().second.push_back(oc);
            }
            else
            {
                onError("Invalid syntax at " + std::to_string(line));
            }
        }
        break;
        case IN_REGION:
        {
            if (c == '>')
            {
                state = NOTHING;
#define HDR_HELPER(a)                                                                              \
    if (res.back().first.name == #a)                                                               \
        res.back().first.type = Header::a;
                HDR_HELPER(region);
                HDR_HELPER(group);
                HDR_HELPER(control);
                HDR_HELPER(global);
                HDR_HELPER(curve);
                HDR_HELPER(effect);
                HDR_HELPER(master);
                HDR_HELPER(midi);
                HDR_HELPER(sample);
                HDR_HELPER(unknown);
            }
            else if (c != ' ')
            {
                res.back().first.name += c;
            }
            else
            {
                onError("Invalid character '" + std::to_string(c) + "' in region at " +
                        std::to_string(line));
            }
        }
        break;
        case IN_SLCOM:
        {
            if (c == '\n' || c == '\r')
            {
                state = NOTHING;
            }
        }
        break;
        case IN_MLCOM:
        {
            if (c == '*' && cn == '/')
            {
                state = NOTHING;
                cp += 2;
            }
        }
        break;
        }
    }
    return res;
}

SFZParser::document_t SFZParser::parse(const fs::path &f)
{
    std::ifstream ifs;
    ifs.open(f);
    std::ostringstream sstr;
    sstr << ifs.rdbuf();
    return parse(sstr.str());
}
} // namespace scxt::sfz_support
