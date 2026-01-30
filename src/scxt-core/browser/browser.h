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

#ifndef SCXT_SRC_SCXT_CORE_BROWSER_BROWSER_H
#define SCXT_SRC_SCXT_CORE_BROWSER_BROWSER_H

#include <vector>
#include <string>
#include <utility>
#include <functional>
#include "filesystem/import.h"
#include "sample/compound_file.h"

namespace scxt::infrastructure
{
struct DefaultsProvider;
}

namespace scxt::messaging
{
struct MessageController;
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
    using errorReporter_t = std::function<void(const std::string &, const std::string &)>;

    Browser(BrowserDB &, const infrastructure::DefaultsProvider &, const fs::path &userDirectory,
            errorReporter_t reportError);

    /*
     * Paths for user content
     */
    fs::path userDirectory;            // like "Documents"/Shortcircuit XT
    fs::path patchIODirectory;         // user/Patches
    fs::path themeDirectory;           // user/Themes
    fs::path modulatorPresetDirectory; // user/Modulator\ Presets
    fs::path fxPresetDirectory;        // user/FX\ Presets
    fs::path voiceFxPresetDirectory;   // user/FX\ Presets/Voice
    fs::path busFxPresetDirectory;     // user/FX\ Presets/Bus

    /*
     * Filesystem Views: Very simple. The Browser gives you a set of
     * filesystem roots and allows you to add your own roots to the
     * browser (which will be persisted system wide). This is safe to call
     * from any non realtime thread other than the sql thread but is really intended
     * to be called from the serialization thread.
     */
    using indexedRootPath_t = std::tuple<fs::path, std::string, bool>;
    std::vector<indexedRootPath_t> getRootPathsForDeviceView() const;
    void addRootPathForDeviceView(const fs::path &, bool indexed, messaging::MessageController &);
    void removeRootPathForDeviceView(const fs::path &, messaging::MessageController &);
    void reindexLocation(const fs::path &);

    static bool isLoadableFile(const fs::path &);
    static bool isLoadableSample(const fs::path &);
    static bool isLoadableSingleSample(const fs::path &);
    static bool isLoadableMultiSample(const fs::path &);
    static bool isShortCircuitFormatFile(const fs::path &);

    static bool isExpandableInBrowser(const fs::path &p) { return isLoadableMultiSample(p); }
    static std::vector<sample::compound::CompoundElement> expandForBrowser(const fs::path &p);

    /*
     * If this item contains more than one instrument, present a list of them. If the resulting
     * vector is empty then the file contains zero or 1 instruments. Basicaly this is how you build
     * the pick-one-on-drop-of-an-sf2 UI
     */
    static std::vector<sample::compound::CompoundElement>
    getMultiInstrumentElements(const fs::path &);

    struct LoadableFile
    {
        static const std::vector<std::string> singleSample;
        static const std::vector<std::string> multiSample;
        static const std::vector<std::string> shortcircuitFormats;
    };

    std::vector<std::pair<fs::path, std::string>> getOSDefaultRootPathsForDeviceView() const;

    const infrastructure::DefaultsProvider &defaultsProvider;
    BrowserDB &browserDb;
    errorReporter_t errorReporter;
};
} // namespace scxt::browser
#endif // SHORTCIRCUITXT_BROWSER_H
