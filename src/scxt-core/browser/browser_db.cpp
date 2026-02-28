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

#include "browser_db.h"

#include "scanner.h"
#include "utils.h"

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>

#include "sql_support.h"
#include "writer_worker.h"
#include "scanner.h"

namespace scxt::browser
{

BrowserDB::BrowserDB(const fs::path &p, messaging::MessageController &m) : mc(m)
{
    writerWorker = std::make_unique<scxt::browser::WriterWorker>(p, m);
    writerWorker->openForWrite();
    m.threadingChecker.addAsAClientThread(writerWorker->qThread.get_id());

    scanner = std::make_unique<Scanner>(*writerWorker, mc);
}

BrowserDB::~BrowserDB() {}

void BrowserDB::writeDebugMessage(const std::string &s)
{
    writerWorker->enqueueWorkItem(new WriterWorker::EnQDebugMsg(s));
}

void BrowserDB::addRemoveBrowserLocation(const fs::path &p, bool index, bool add)
{
    writerWorker->enqueueWorkItem(new WriterWorker::EnQBrowserLocation(p, index, add));
    if (add && index)
    {
        scanner->scanPath(p, 0);
    }
}

void BrowserDB::reindexLocation(const fs::path &p) { scanner->scanPath(p, 0); }

std::vector<std::pair<fs::path, bool>> BrowserDB::getBrowserLocations()
{
    auto conn = writerWorker->getReadOnlyConn();
    std::vector<std::pair<fs::path, bool>> res;

    // language=SQL
    std::string query = "SELECT path FROM BrowserLocations;";
    try
    {
        auto q = SQL::Statement(conn, query);
        while (q.step())
        {
            res.emplace_back(fs::path{q.col_str(0)}, true);
        }
        q.finalize();
    }
    catch (SQL::Exception &e)
    {
        SCLOG_IF(sqlDb, e.what());
        RAISE_ERROR_CONT(mc, "Database Error", e.what());
    }
    return res;
}

int BrowserDB::numberOfJobsOutstanding() const
{
    std::lock_guard<std::mutex> guard(writerWorker->qLock);
    return writerWorker->pathQ.size();
}

int BrowserDB::waitForJobsOutstandingComplete(int maxWaitInMS) const
{
    int maxIts = maxWaitInMS / 10;
    int njo = 0;
    while ((njo = numberOfJobsOutstanding()) > 0 && maxIts > 0)
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
        maxIts--;
    }
    return numberOfJobsOutstanding();
}

} // namespace scxt::browser