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
#include <algorithm>
#include <string_view>
#include <system_error>

namespace scxt::sfz_support
{

namespace
{
bool readWholeFile(const fs::path &f, std::string &into)
{
    std::ifstream ifs;
    ifs.open(f);
    if (!ifs.is_open())
        return false;
    std::ostringstream sstr;
    sstr << ifs.rdbuf();
    into = sstr.str();
    return true;
}

fs::path canonicalOrNormal(const fs::path &f)
{
    std::error_code ec;
    auto c = fs::weakly_canonical(f, ec);
    return ec ? f.lexically_normal() : c;
}

// Updates the running block-comment flag for one line. We only honor `/*`,
// `*/` and `//` here; a lone `/` (which the tokenizer does treat as a comment)
// is ignored so that `sample=foo/bar.wav` doesn't truncate the scan.
bool advanceCommentState(std::string_view line, bool inBlock)
{
    for (size_t i = 0; i < line.size(); ++i)
    {
        auto next = (i + 1 < line.size()) ? line[i + 1] : '\0';
        if (inBlock)
        {
            if (line[i] == '*' && next == '/')
            {
                inBlock = false;
                ++i;
            }
        }
        else if (line[i] == '/' && next == '*')
        {
            inBlock = true;
            ++i;
        }
        else if (line[i] == '/' && next == '/')
        {
            return inBlock;
        }
    }
    return inBlock;
}

// Matches a leading `#word`, lower-casing the word and handing back the
// remainder of the line. Leading whitespace is allowed.
bool matchHashDirective(std::string_view line, std::string &directive, std::string_view &rest)
{
    size_t i{0};
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
        ++i;
    if (i >= line.size() || line[i] != '#')
        return false;
    ++i;
    directive.clear();
    while (i < line.size() &&
           ((line[i] >= 'a' && line[i] <= 'z') || (line[i] >= 'A' && line[i] <= 'Z')))
    {
        auto c = line[i];
        if (c >= 'A' && c <= 'Z')
            c = (char)(c - 'A' + 'a');
        directive += c;
        ++i;
    }
    rest = line.substr(i);
    return !directive.empty();
}

// `rest` is everything after `#include`. Prefer the quoted form; fall back to
// the trimmed remainder since some files in the wild omit the quotes.
bool extractIncludePath(std::string_view rest, std::string &path)
{
    auto q = rest.find('"');
    if (q != std::string_view::npos)
    {
        auto e = rest.find('"', q + 1);
        if (e == std::string_view::npos)
            return false;
        path = std::string(rest.substr(q + 1, e - q - 1));
    }
    else
    {
        size_t b{0};
        while (b < rest.size() && (rest[b] == ' ' || rest[b] == '\t'))
            ++b;
        auto e = rest.size();
        while (e > b && (rest[e - 1] == ' ' || rest[e - 1] == '\t' || rest[e - 1] == '\r' ||
                         rest[e - 1] == '\n'))
            --e;
        path = std::string(rest.substr(b, e - b));
    }
    return !path.empty();
}

struct IncludeExpander
{
    fs::path rootDir;
    const std::function<void(const std::string &)> &onError;
    // Files currently being expanded. A stack rather than a visited-set: a
    // diamond include is legal and must expand twice; only a cycle is an error.
    std::vector<fs::path> stack;
    int fileCount{0};

    void expand(const std::string &contents, const fs::path &includingDir, std::string &out);
    void expandFile(const fs::path &file, std::string &out);
};

void IncludeExpander::expandFile(const fs::path &file, std::string &out)
{
    auto canon = canonicalOrNormal(file);
    if (std::find(stack.begin(), stack.end(), canon) != stack.end())
    {
        onError("Recursive #include of '" + file.u8string() + "' ignored");
        return;
    }
    if ((int)stack.size() >= SFZParser::maxIncludeDepth)
    {
        onError("#include nested deeper than " + std::to_string(SFZParser::maxIncludeDepth) +
                " at '" + file.u8string() + "'; ignored");
        return;
    }
    if (fileCount >= SFZParser::maxIncludeFiles)
    {
        onError("#include expanded more than " + std::to_string(SFZParser::maxIncludeFiles) +
                " files; ignoring '" + file.u8string() + "'");
        return;
    }

    std::string contents;
    if (!readWholeFile(file, contents))
    {
        onError("Unable to read #include '" + file.u8string() + "'");
        return;
    }

    fileCount++;
    stack.push_back(canon);
    expand(contents, file.parent_path(), out);
    stack.pop_back();
}

void IncludeExpander::expand(const std::string &contents, const fs::path &includingDir,
                             std::string &out)
{
    const auto n = contents.size();
    bool inBlockComment{false};
    size_t pos{0};
    std::string directive;
    std::string_view rest;

    while (pos < n)
    {
        auto eol = contents.find('\n', pos);
        auto lineEnd = (eol == std::string::npos) ? n : eol + 1;
        auto line = std::string_view(contents).substr(pos, lineEnd - pos);
        pos = lineEnd;

        // The directive is live only if the line *started* outside a comment;
        // advance the flag afterwards so a trailing `/*` still carries over.
        auto wasInComment = inBlockComment;
        inBlockComment = advanceCommentState(line, inBlockComment);

        if (wasInComment || !matchHashDirective(line, directive, rest))
        {
            out += line;
            continue;
        }

        if (directive == "include")
        {
            std::string raw;
            if (!extractIncludePath(rest, raw))
            {
                onError("Malformed #include directive");
            }
            else
            {
                // fs handles '/' everywhere, including on windows
                std::replace(raw.begin(), raw.end(), '\\', '/');
                auto rel = fs::path{raw};
                // ARIA resolves against the top level file; fall back to the
                // including file's own directory so both conventions work.
                auto cand = (rootDir / rel).lexically_normal();
                if (!fs::exists(cand))
                    cand = (includingDir / rel).lexically_normal();

                if (fs::exists(cand))
                    expandFile(cand, out);
                else
                    onError("Unable to resolve #include \"" + raw + "\"");
            }
        }
        else
        {
            // #define and friends. Recognized here purely so the tokenizer
            // never sees the '#' and reports it as a syntax error.
            onError("Unsupported directive '#" + directive + "' ignored");
        }
        out += '\n';
    }
}
} // namespace

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
        // Opcodes are case-insensitive per the SFZ spec — `Sample=` and
        // `sample=` should both parse. Normalize to lower-case at the source
        // so every downstream consumeOpcode("foo") match works.
        opcode.clear();
        while (from < s.size() && s[from] != ' ' && s[from] != '\n' && s[from] != '\r')
        {
            if (s[from] == '=')
                return true;
            char c = s[from];
            if (c >= 'A' && c <= 'Z')
                c = (char)(c - 'A' + 'a');
            opcode += c;
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
        auto ep = s.size(); // one past last kept char; size_t, so never go below 0
        while (ep > 0 && (s[ep - 1] == ' ' || s[ep - 1] == '\n' || s[ep - 1] == '\r'))
        {
            ep--;
        }
        auto res = s.substr(0, ep);
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

std::string SFZParser::expandIncludes(const std::string &contents, const fs::path &rootDir)
{
    IncludeExpander ex{rootDir, onError};
    std::string res;
    res.reserve(contents.size());
    ex.expand(contents, rootDir, res);
    return res;
}

std::string SFZParser::preprocessIncludes(const fs::path &f)
{
    std::string contents;
    if (!readWholeFile(f, contents))
    {
        onError("Unable to read SFZ file '" + f.u8string() + "'");
        return {};
    }

    IncludeExpander ex{f.parent_path(), onError};
    // Seed the stack so a file which includes itself is caught as a cycle
    ex.stack.push_back(canonicalOrNormal(f));
    std::string res;
    res.reserve(contents.size());
    ex.expand(contents, f.parent_path(), res);
    return res;
}

SFZParser::document_t SFZParser::parse(const fs::path &f) { return parse(preprocessIncludes(f)); }
} // namespace scxt::sfz_support
