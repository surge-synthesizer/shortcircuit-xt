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

#include <fstream>

#include "engine/engine.h"
#include "patch_io/patch_io.h"
#include "json/scxt_traits.h"
#include "json/engine_traits.h"

#include "tao/json/msgpack/to_string.hpp"

int main(int argc, char **argv)
{
    if (argc == 3)
    {
        auto e = std::make_unique<scxt::engine::Engine>();
        e->getMessageController()->threadingChecker.bypassThreadChecks = true;

        auto outp = fs::path(argv[2]);
        auto inp = fs::path(argv[1]);

        SCLOG_IF(cliTools, "Loading multi " << inp.u8string());

        scxt::patch_io::loadMulti(inp, *e);

        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_MULTI);
        auto msg = tao::json::msgpack::to_string(scxt::json::scxt_value(*e));

        SCLOG_IF(cliTools, "Message is of size " << msg.size());

        SCLOG_IF(cliTools, "Writing to file " << outp.u8string());

        std::ofstream f(outp, std::ios::binary);
        if (!f.is_open())
        {
            SCLOG_IF(cliTools, "Unable to open file");
            exit(2);
        }

        f.write(msg.c_str(), msg.size());

        SCLOG_IF(cliTools, "Output complete");
    }
    else if (argc == 2)
    {
        auto e = std::make_unique<scxt::engine::Engine>();
        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_MULTI);
        auto msg = tao::json::msgpack::to_string(scxt::json::scxt_value(*e));

        SCLOG_IF(cliTools, "Message is of size " << msg.size());

        auto p = fs::path(argv[1]);

        SCLOG_IF(cliTools, "Writing to file " << p.u8string());

        std::ofstream f(p, std::ios::binary);
        if (!f.is_open())
        {
            SCLOG_IF(cliTools, "Unable to open file");
            exit(2);
        }

        f.write(msg.c_str(), msg.size());

        SCLOG_IF(cliTools, "Output complete");
    }
    else
    {
        SCLOG_IF(cliTools, "Usage: init-maker <output-file> for default or");
        SCLOG_IF(cliTools, "Usage: init-maker <input-scm> <output-file> for scm conversion");
        exit(1);
    }
    exit(0);
}