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

#include "akai_import.h"
#include <fstream>

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
    SCLOG("Dumping " << path.u8string());
    auto f = std::ifstream(path, std::ios::binary);
    if (!f.is_open())
    {
        SCLOG("Failed to open file");
        return;
    }
    f.seekg(0, std::ios::end);
    auto size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    f.read(buffer.data(), size);
    SCLOG("File size: " << size);

    if (fourCC(buffer.data()) != "RIFF" || fourCC(buffer.data() + 8) != "APRG")
    {
        SCLOG("Not an AKAI file");
        return;
    }

    SCLOG("Akai file identified. Dumping chunks");

    auto pos = 12;
    while (pos < size)
    {
        auto [ss, s] = fourCCSize(buffer.data() + pos);
        SCLOG("  '" << ss << "' size= " << s);

        if (ss == "kgrp")
        {
            auto subSz = s;
            pos += 8;
            while (subSz > 0)
            {
                auto [ks, kz] = fourCCSize(buffer.data() + pos);
                SCLOG("   -  '" << ks << "' size= " << kz);
                if (ks == "kloc")
                {
                    KLOC k;
                    k.load(buffer.data() + pos + 8);
                    SCLOG("      range: " << k.lo << " " << k.hi << " tune:  " << k.semi << " "
                                          << k.fine << " group: " << k.grp);
                }
                if (ks == "zone")
                {
                    ZONE z;
                    z.load(buffer.data() + pos + 8);
                    if (z.mapped)
                    {
                        SCLOG("      sample: '" << z.sample << "' vel range " << z.lov << "-"
                                                << z.hiv);
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