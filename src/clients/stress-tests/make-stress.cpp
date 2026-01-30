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

#include <fstream>
#include "console_harness.h"

void makeNbyN(int dc, const std::string &path)
{
    auto ch = std::make_unique<scxt::clients::console_ui::ConsoleHarness>();
    ch->start(false);

    ch->stepUI();

    namespace cmsg = scxt::messaging::client;

    using abz = cmsg::AddBlankZone;
    for (int k = 0; k < 127; k += dc)
    {
        SCLOG_IF(cliTools, "Starting key " << k)
        for (int v = 0; v < 127; v += dc)
        {
            ch->sendToSerialization(abz({0, 0, k, k + dc - 1, v, v + dc - 1}));
        }
        ch->stepUI();
    }

    ch->sendToSerialization(cmsg::SaveMulti({path, 0}));
    ch->stepUI();
}

void makeNbyNGroupPer(int dc, const std::string &path)
{
    auto ch = std::make_unique<scxt::clients::console_ui::ConsoleHarness>();
    ch->start(false);

    ch->stepUI();

    namespace cmsg = scxt::messaging::client;

    using abz = cmsg::AddBlankZone;
    int gi{0};
    for (int k = 0; k < 127; k += dc)
    {
        SCLOG_IF(cliTools, "Starting key " << k)
        for (int v = 0; v < 127; v += dc)
        {
            ch->sendToSerialization(cmsg::CreateGroup(0));
            ch->sendToSerialization(abz({0, gi, k, k + dc - 1, v, v + dc - 1}));
            gi++;
        }
        ch->stepUI();
    }

    ch->sendToSerialization(cmsg::SaveMulti({path, 0}));
    ch->stepUI();
}

int main(int argc, char **argv)
{
    makeNbyNGroupPer(5, "/tmp/stressG.scm");
    return 0;
}