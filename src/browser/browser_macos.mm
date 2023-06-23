/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#include <stdlib.h>
#include <Foundation/Foundation.h>

#include "browser.h"

namespace scxt::browser
{

std::vector<fs::path> Browser::getOSDefaultRootPathsForDeviceView() const
{
    std::vector<fs::path> res;

    res.emplace_back("/");
    if (getenv("HOME"))
        res.emplace_back(getenv("HOME"));

    auto *fileManager = [NSFileManager defaultManager];
    auto *resultURLs = [fileManager URLsForDirectory:NSMusicDirectory inDomains:NSUserDomainMask];
    if (resultURLs)
    {
        auto *u = [resultURLs objectAtIndex:0];
        res.emplace_back([u fileSystemRepresentation]);
    }

    return res;
}
} // namespace scxt::browser