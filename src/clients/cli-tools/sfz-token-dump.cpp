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
#include <iostream>
#include "sample/sfz_support/sfz_parse.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " (sfzfile)" << std::endl;
        return 1;
    }

    auto f = fs::path{argv[1]};
    scxt::sfz_support::SFZParser parser;
    auto res = parser.parse(f);

    std::cout << "<sfzdump file=\"" << f.u8string() << "\">" << std::endl;

    for (const auto &[header, opcodes] : res)
    {
        std::cout << "  <header type=\"" << header.name << "\">" << std::endl;
        for (const auto &[key, value] : opcodes)
        {
            std::cout << "    <opcode key=\"" << key << "\" value=\"" << value << "\"/>"
                      << std::endl;
        }
        std::cout << "  </header>" << std::endl;
    }

    std::cout << "</sfzdump>" << std::endl;

    return 0;
}
