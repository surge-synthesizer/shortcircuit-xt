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
        SCLOG("Starting key " << k)
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
        SCLOG("Starting key " << k)
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