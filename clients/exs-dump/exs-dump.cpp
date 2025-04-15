/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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
#include "sst/plugininfra/version_information.h"

#include "utils.h"

#include "sample/exs_support/exs_import.h"

int main(int argc, char **argv)
{
    SCLOG("EXS-DUMP");
    SCLOG("    Version   = " << sst::plugininfra::VersionInformation::git_implied_display_version
                             << " / "
                             << sst::plugininfra::VersionInformation::project_version_and_hash);
    if (argc < 2)
    {
        SCLOG("Usage: " << argv[0] << " <exs-file>");
        return 1;
    }
    SCLOG("Opening " << argv[1]);

    scxt::exs_support::dumpEXSToLog(fs::path(argv[1]));
    return 0;
}