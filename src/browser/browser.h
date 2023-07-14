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

#ifndef SCXT_SRC_BROWSER_BROWSER_H
#define SCXT_SRC_BROWSER_BROWSER_H

#include <vector>
#include <string>
#include <utility>
#include "filesystem/import.h"

namespace scxt::infrastructure
{
struct DefaultsProvider;
}

namespace scxt::browser
{
struct BrowserDB;

/*
 * The Browser is the DATA api to allow you to ask questions about the
 * samples installed on this users computer. There's an instance owned by
 * the engine.
 *
 * The browser provides three basic functions
 *
 * 1. Filesystem view roots
 * 2. Favorites and Search
 * 3. Static functions which generate data on files
 */
struct Browser
{
    Browser(BrowserDB &, const infrastructure::DefaultsProvider &);

    /*
     * Filesystem Views: Very simple. The Browser gives you a set of
     * filesystem roots and allows you to add your own roots to the
     * browser (which will be persisted system wide). This is safe to call
     * from any thread other than the sql thread but is really intended
     * to be called from the UI thread.
     */
    std::vector<std::pair<fs::path, std::string>> getRootPathsForDeviceView() const;
    void addRootPathForDeviceView(const fs::path &);
    bool isLoadableFile(const fs::path &) const;

    std::vector<std::pair<fs::path, std::string>> getOSDefaultRootPathsForDeviceView() const;

    const infrastructure::DefaultsProvider &defaultsProvider;
    BrowserDB &browserDb;
};
} // namespace scxt::browser
#endif // SHORTCIRCUITXT_BROWSER_H
