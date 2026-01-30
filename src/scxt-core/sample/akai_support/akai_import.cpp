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

#include "akai_import.h"
#include <fstream>
#include "configuration.h"

namespace scxt::akai_support
{

// Thanks https://burnit.co.uk/AKPspec/

struct KLOC
{
    uint16_t lo, hi;
    uint16_t semi, fine;
    uint16_t grp;

    void load(const char *data)
    {
        lo = data[4];
        hi = data[5];
        semi = data[6];
        fine = data[7];
        grp = data[14];
    }
};

struct ZONE
{
    bool mapped{false};
    std::string sample;
    uint16_t lov{0}, hiv{127};
    void load(const char *data)
    {
        if (data[1] > 0)
        {
            mapped = true;
            auto slen = data[1];
            for (int i = 0; i < slen; ++i)
                sample += (char)data[2 + i];
            constexpr int start = 0x148 - 0x126;
            lov = data[start];
            hiv = data[start + 1];
        }
    }
};

std::string fourCC(const char *c)
{
    std::string fourCC;
    fourCC += c[0];
    fourCC += c[1];
    fourCC += c[2];
    fourCC += c[3];
    return fourCC;
}
std::pair<std::string, uint32_t> fourCCSize(const char *c)
{
    std::string fourCC;
    fourCC += c[0];
    fourCC += c[1];
    fourCC += c[2];
    fourCC += c[3];
    uint32_t size = 0;
    for (int i = 0; i < 4; ++i)
        size += (uint32_t(c[i + 4]) << (i * 8));
    return {fourCC, size};
}
void dumpAkaiToLog(const fs::path &path)
{
    SCLOG_IF(sampleCompoundParsers, "Dumping " << path.u8string());
    auto f = std::ifstream(path, std::ios::binary);
    if (!f.is_open())
    {
        SCLOG_IF(sampleCompoundParsers, "Failed to open file");
        return;
    }
    f.seekg(0, std::ios::end);
    auto size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    f.read(buffer.data(), size);
    SCLOG_IF(sampleCompoundParsers, "File size: " << size);

    if (fourCC(buffer.data()) != "RIFF" || fourCC(buffer.data() + 8) != "APRG")
    {
        SCLOG_IF(sampleCompoundParsers, "Not an AKAI file");
        return;
    }

    SCLOG_IF(sampleCompoundParsers, "Akai file identified. Dumping chunks");

    auto pos = 12;
    while (pos < size)
    {
        auto [ss, s] = fourCCSize(buffer.data() + pos);
        SCLOG_IF(sampleCompoundParsers, "  '" << ss << "' size= " << s);

        if (ss == "kgrp")
        {
            auto subSz = s;
            pos += 8;
            while (subSz > 0)
            {
                auto [ks, kz] = fourCCSize(buffer.data() + pos);
                SCLOG_IF(sampleCompoundParsers, "   -  '" << ks << "' size= " << kz);
                if (ks == "kloc")
                {
                    KLOC k;
                    k.load(buffer.data() + pos + 8);
                    SCLOG_IF(sampleCompoundParsers,
                             "      range: " << k.lo << " " << k.hi << " tune:  " << k.semi << " "
                                             << k.fine << " group: " << k.grp);
                }
                if (ks == "zone")
                {
                    ZONE z;
                    z.load(buffer.data() + pos + 8);
                    if (z.mapped)
                    {
                        SCLOG_IF(sampleCompoundParsers, "      sample: '" << z.sample
                                                                          << "' vel range " << z.lov
                                                                          << "-" << z.hiv);
                    }
                }
                pos += 8 + kz;
                subSz -= 8 + kz;
            }
        }
        else
        {
            pos += s + 8;
        }
    }
}
} // namespace scxt::akai_support