/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include "configuration.h"
#include <regex>
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "tinyxml/tinyxml.h"
#include "util/scxtstring.h"

#include "version.h"

configuration::configuration(scxt::log::StreamLogger &logger) : mLogger(logger) {}

fs::path configuration::resolve_path(const fs::path &in)
{
    // this assumes the path is a straight path without the | or > suffix
    auto s = path_to_string(in);
    auto r = std::regex_replace(s, std::regex("<relative>"), path_to_string(mRelative));
    auto p = string_to_path(r);
    // try to stat the file and if we can't get to it, it's possible we are on unix and the file
    // extension is in uppercase
    // since we build filenames with lowercase extensions, and all other filenames are coming
    // from the outer world (so presumably will be correct) we only need to try uppercase
    // it's possible that the ext may be mixed case, but not bothering right now
#if !defined(_WIN32) && !defined(__APPLE__)
    std::error_code ec;
    if (!exists(p, ec))
    {

        std::string ext, name;
        fs::path pathOnly;

        decode_path(p, 0, &ext, &name, &pathOnly);
        std::transform((ext).begin(), (ext).end(), (ext).begin(), ::toupper);

        auto test = build_path(pathOnly, name, ext);
        if (exists(test, ec))
        {
            return test;
        }
    }
#endif
    return p;
}

void configuration::set_relative_path(const fs::path &in) { mRelative = in; }

void decode_path(const fs::path &in, fs::path *out, std::string *extension, std::string *name_only,
                 fs::path *path_only, int *program_id, int *sample_id)
{
    if (path_only)
    {

        *path_only = in;
        if (path_only->has_filename())
        {
            path_only->remove_filename();
        }
        // rem trailing separator if there
        auto s = path_to_string(*path_only);
        if (s.back() == static_cast<char>(fs::path::preferred_separator))
        {
            s.pop_back();
            *path_only = string_to_path(s);
        }
    }
    if (out)
        out->clear();
    if (extension)
        extension->clear();
    if (name_only)
        name_only->clear();
    if (sample_id)
        *sample_id = -1;
    if (program_id)
        *program_id = -1;

    auto tmp = path_to_string(in);

    // get and strip off program/sample id if available
    const char *separator = strrchr(tmp.c_str(), '|');
    if (separator)
    {
        std::string sidstr = separator + 1;

        if (sample_id)
            *sample_id = std::strtol(sidstr.c_str(), NULL, 10);

        tmp = tmp.substr(0, separator - tmp.c_str());
    }

    separator = strrchr(tmp.c_str(), '>');
    if (separator)
    {
        std::string pidstr = separator + 1;
        if (program_id)
            *program_id = strtol(pidstr.c_str(), NULL, 10);
        tmp = tmp.substr(0, separator - tmp.c_str());
    }
    // extract filename portion only
    auto p = string_to_path(tmp);
    auto no = path_to_string(p.filename());

    // extract extension and cvt to lower, also get name only without ext
    separator = strrchr(no.c_str(), '.');
    if (separator)
    {
        if (extension)
        {
            *extension = separator + 1;
            std::transform((*extension).begin(), (*extension).end(), (*extension).begin(),
                           ::tolower);
        }
        // strip it off
        no = no.substr(0, separator - no.c_str());
    }

    if (name_only)
    {
        *name_only = no;
    }

    if (out)
        *out = string_to_path(tmp);
}

fs::path build_path(const fs::path &in, const std::string &filename, const std::string &ext)
{
    std::string s = path_to_string(in);

    if (s.back() != static_cast<char>(fs::path::preferred_separator))
    {
        s.push_back(static_cast<char>(fs::path::preferred_separator));
    }
    s.append(filename);
    if (ext.length())
    {
        s.append(".");
        s.append(ext);
    }
    return string_to_path(s);
}
