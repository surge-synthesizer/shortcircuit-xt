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

#ifndef SCXT_SRC_BROWSER_BROWSER_DB_H
#define SCXT_SRC_BROWSER_BROWSER_DB_H

#include "filesystem/import.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>

namespace scxt::messaging
{
struct MessageController;
}

namespace scxt::browser
{
struct WriterWorker;
struct Scanner;
struct BrowserDB
{
    BrowserDB(const fs::path &, messaging::MessageController &);
    ~BrowserDB();

    void writeDebugMessage(const std::string &);
    void addRemoveBrowserLocation(const fs::path &, bool index, bool add);
    void reindexLocation(const fs::path &);

    template <typename T> void addClientToSerialCallback(const T &);

    std::vector<std::pair<fs::path, bool>> getBrowserLocations();

    int numberOfJobsOutstanding() const;
    int waitForJobsOutstandingComplete(int maxWaitInMS) const;

    void scanToUpdateSamples();

  private:
    messaging::MessageController &mc;
    std::unique_ptr<WriterWorker> writerWorker;
    std::unique_ptr<Scanner> scanner;
};
} // namespace scxt::browser
#endif // SHORTCIRCUITXT_BROWSER_DB_H
