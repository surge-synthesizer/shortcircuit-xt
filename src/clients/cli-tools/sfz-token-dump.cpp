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
