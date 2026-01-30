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

#ifndef SCXT_SRC_SCXT_CORE_BROWSER_BROWSER_DB_H
#define SCXT_SRC_SCXT_CORE_BROWSER_BROWSER_DB_H

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
