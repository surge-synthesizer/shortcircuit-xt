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

#ifndef SCXT_SRC_SCXT_CORE_BROWSER_WRITER_WORKER_H
#define SCXT_SRC_SCXT_CORE_BROWSER_WRITER_WORKER_H

#define TRACE_DB 0

#include "sql_support.h"
#include "utils.h"
#include "messaging/messaging.h"
#include "messaging/client/client_messages.h"

namespace scxt::browser
{

struct WriterWorker
{
    static constexpr const char *schema_version =
        "1011"; // I will rebuild if this is not my version

    static constexpr const char *setup_sql = R"SQL(
DROP TABLE IF EXISTS "DebugJunk";
DROP TABLE IF EXISTS "DebugLog";
DROP TABLE IF EXISTS "Version";
DROP TABLE IF EXISTS "Favorites";
DROP TABLE IF EXISTS "IndexedDeviceLocations";
DROP TABLE IF EXISTS "SampleInfo";
DROP INDEX IF EXISTS "SampleInfoPath";
DROP INDEX IF EXISTS "SampleInfoMD5";

CREATE TABLE "Version" (
    id integer primary key,
    schema_version varchar(256)
);

CREATE TABLE DebugLog (
    id integer primary key,
    time varchar(256),
    since integer,
    msg varchar(2048)
);

CREATE TABLE SampleInfo (
    id integer primary key,
    path varchar(2048),
    format varchar(32),
    md5 varchar(64),
    size integer,
    mtime integer
);

CREATE INDEX SampleInfoPath ON SampleInfo (path);
CREATE INDEX SampleInfoMD5 ON SampleInfo (md5);

-- We create these tables only if missing of course since it is user data
CREATE TABLE IF NOT EXISTS BrowserLocations (
    id integer primary key,
    path varchar(2048),
    isIndexed bool
);


    )SQL";

    struct EnQAble
    {
        bool needsRW{true};
        virtual ~EnQAble() = default;
        virtual void go(WriterWorker &) = 0;
    };

    struct EnQDebugMsg : public EnQAble
    {
        std::string msg;
        EnQDebugMsg(const std::string &msg) : msg(msg) {}
        void go(WriterWorker &w) override { w.addDebug(msg); }
    };

    struct EnQBrowserLocation : public EnQAble
    {
        fs::path path;
        bool index;
        bool add;
        EnQBrowserLocation(const fs::path &msg, bool idx, bool add)
            : path(msg), index(idx), add(add)
        {
        }
        void go(WriterWorker &w) override { w.addRemoveBrowserLocation(path, index, add); }
    };

    struct EnQAddSampleInfo : public EnQAble
    {
        fs::path path;
        std::string md5;
        uint64_t time;
        uint64_t filesz;

        EnQAddSampleInfo(const fs::path &p, const std::string &md5, uint64_t time, uint64_t sz)
            : path(p), md5(md5), time(time), filesz(sz)
        {
        }
        void go(WriterWorker &w) override
        {
            try
            {
                auto del = SQL::Statement(w.dbh, "DELETE FROM SampleInfo WHERE path==?1");
                std::string res = path.u8string();
                del.bind(1, res);
                del.step();
                del.finalize();

                auto there =
                    SQL::Statement(w.dbh, "INSERT INTO SampleInfo  (\"path\", \"format\", \"md5\", "
                                          "\"size\", \"mtime\") VALUES (?1, ?2, ?3, ?4, ?5)");

                there.bind(1, res);
                there.bind(2, path.extension().u8string());
                there.bind(3, md5);
                there.bind(4, filesz);
                there.bind(5, time);

                there.step();
                there.finalize();
            }
            catch (SQL::Exception &e)
            {
                SCLOG_IF(warnings, "Unable to insert into SampleInfo: " << e.what())
            }
        }
    };

    void openDb()
    {
#if TRACE_DB
        SCLOG_IF(sqlDb, ">>> Opening r/w DB");
#endif
        auto flag = SQLITE_OPEN_NOMUTEX; // basically lock
        flag |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

        auto ec = sqlite3_open_v2(dbname.c_str(), &dbh, flag, nullptr);

        if (ec != SQLITE_OK)
        {
            // every batch retries the open, so only report the first failure
            if (!reportedOpenFailure)
            {
                std::ostringstream oss;
                oss << "Unable to open the browser database '" << dbname << "'. The error was '"
                    << sqlite3_errmsg(dbh) << "'.";
                RAISE_ERROR_FROM_WORKER(mc, "Browser Database Error", oss.str());
                reportedOpenFailure = true;
            }
            if (dbh)
            {
                // even if opening fails we still need to close the database
                sqlite3_close(dbh);
            }
            dbh = nullptr;
            return;
        }
        reportedOpenFailure = false;
    }

    void closeDb()
    {
#if TRACE_DB
        SCLOG_IF(sqlDb, "<<<< Closing r/w DB");
#endif
        if (dbh)
            sqlite3_close(dbh);
        dbh = nullptr;
    }

    std::string dbname;
    fs::path dbpath;
    messaging::MessageController &mc;

    explicit WriterWorker(const fs::path &p, messaging::MessageController &m) : mc(m)
    {
        dbpath = p / "SCXTBrowser.db";
        dbname = dbpath.u8string();
    }

    struct EnQSetup : EnQAble
    {
        EnQSetup() {}
        void go(WriterWorker &w)
        {
            w.setupDatabase();
#if TRACE_DB
            SCLOG_IF(sqlDb, "Done with EnQSetup");
#endif
        }
    };

    template <typename T> struct EnQClientCallback : EnQAble
    {
        T msg;
        EnQClientCallback(const T &msg) : msg(msg) { needsRW = false; }
        void go(WriterWorker &w) override
        {
            // This is a bit hacky
            messaging::client::clientSendToSerialization(msg, w.mc);
        }
    };

    std::atomic<bool> hasSetup{false};
    void setupDatabase()
    {
#if TRACE_DB
        SCLOG_IF(sqlDb, "PatchDB : Setup Database " << dbname);
#endif
        /*
         * OK check my version
         */
        bool rebuild = true;
        try
        {
            auto st = SQL::Statement(dbh, "SELECT * FROM Version");

            while (st.step())
            {
                int id = st.col_int(0);
                auto ver = st.col_charstar(1);
#if TRACE_DB
                SCLOG_IF(sqlDb, "        : schema check. DBVersion='" << ver << "' SchemaVersion='"
                                                                      << schema_version << "'");
#endif
                if (strcmp(ver, schema_version) == 0)
                {
#if TRACE_DB
                    SCLOG_IF(sqlDb, "        : Schema matches. Reusing database.");
#endif
                    rebuild = false;
                }
            }

            st.finalize();
        }
        catch (const SQL::Exception &e)
        {
            // no version table just means we rebuild
            rebuild = true;
        }

        char *emsg;
        if (rebuild)
        {
#if TRACE_DB
            SCLOG_IF(sqlDb,
                     "        : Schema missing or mismatched. Dropping and Rebuilding Database.");
#endif
            try
            {
                SQL::Exec(dbh, setup_sql);
                auto versql = std::string("INSERT INTO VERSION (\"schema_version\") VALUES (\"") +
                              schema_version + "\")";
                SQL::Exec(dbh, versql);
            }
            catch (const SQL::Exception &e)
            {
                RAISE_ERROR_FROM_WORKER(mc, "Browser Database Error",
                                        std::string("Unable to set up the browser database. ") +
                                            e.what());
            }
        }

        hasSetup = true;
    }

    bool haveOpenedForWriteOnce{false};
    void openForWrite()
    {
        if (haveOpenedForWriteOnce)
            return;
        // We know this is called in the lock so can manipulate pathQ properly
        haveOpenedForWriteOnce = true;
        qThread = std::thread([this]() { this->loadQueueFunction(); });
        // before setup, which can report errors
        mc.threadingChecker.addAsAClientThread(qThread.get_id());

        std::unique_lock<std::mutex> lk(qLock);
        pathQ.push_back(new EnQSetup());
        qCV.notify_all();
        waitingCV.wait(lk, [this]() { return waiting.load(); });
    }

    ~WriterWorker()
    {
        if (haveOpenedForWriteOnce)
        {
            {
                // set under the lock or the worker can miss the wakeup and never join
                std::lock_guard<std::mutex> g(qLock);
                keepRunning = false;
            }
            qCV.notify_all();
            qThread.join();
            for (auto *q : pathQ)
                delete q;
            pathQ.clear();
            // clean up all the prepared statements
            if (dbh)
                sqlite3_close(dbh);
            dbh = nullptr;
            haveOpenedForWriteOnce = false;
        }

        if (rodbh)
        {
            sqlite3_close(rodbh);
            rodbh = nullptr;
        }
    }

    // FIXME features should be an enum or something

    /*
     * Functions for the write thread
     */
    std::atomic<bool> waiting{false};
    bool reportedOpenFailure{false}, reportedWriteFailure{false};
    void loadQueueFunction()
    {
        static constexpr auto transChunkSize = 50;
        int lock_retries{0};
        int lastCount{0}, nextCount{0};
        while (keepRunning)
        {
            std::vector<EnQAble *> doThis;
            std::vector<EnQAble *> doThisNoWrite;
            {
                std::unique_lock<std::mutex> lk(qLock);

                while (keepRunning && pathQ.empty())
                {
                    if (dbh)
                        closeDb();
                    waiting = true;
                    waitingCV.notify_all();
                    qCV.wait(lk);
                    waiting = false;
                }

                if (keepRunning)
                {
                    auto b = pathQ.begin();
                    auto e = (pathQ.size() < transChunkSize) ? pathQ.end()
                                                             : pathQ.begin() + transChunkSize;
                    auto curr = b;
                    while (curr != e)
                    {
                        if ((*curr)->needsRW)
                        {
                            doThis.push_back(*curr);
                        }
                        else
                        {
                            doThisNoWrite.push_back(*curr);
                        }
                        ++curr;
                    }
                    pathQ.erase(b, e);
                    nextCount = pathQ.size();
                }
            }
            if (!doThis.empty())
            {
                if (!dbh)
                    openDb();
                if (dbh == nullptr)
                {
                    // openDb has reported, so drop the batch
                    for (auto *p : doThis)
                        delete p;
                }
                else
                {
                    try
                    {
                        SQL::TxnGuard tg(dbh);

                        for (auto *&p : doThis)
                        {
                            p->go(*this);
                            delete p;
                            p = nullptr;
                        }

                        tg.end();
                        lock_retries = 0;
                        reportedWriteFailure = false;
                    }
                    catch (SQL::LockedException &le)
                    {
                        SCLOG_IF(warnings, "Browser database locked for writing, attempt "
                                               << lock_retries << ": " << le.what());
                        // reload the unrun items onto the front of the queue and sleep
                        lock_retries++;
                        if (lock_retries < 10)
                        {
                            {
                                std::unique_lock<std::mutex> lk(qLock);
                                std::reverse(doThis.begin(), doThis.end());
                                for (auto p : doThis)
                                {
                                    if (p)
                                        pathQ.push_front(p);
                                }
                            }
                            std::this_thread::sleep_for(std::chrono::seconds(lock_retries * 3));
                        }
                        else
                        {
                            RAISE_ERROR_FROM_WORKER(
                                mc, "Browser Database Locked",
                                "The browser database is locked and unwritable after multiple "
                                "attempts. Most likely another Shortcircuit XT instance has "
                                "exclusive write access.");
                            for (auto *p : doThis)
                                delete p;
                            lock_retries = 0;
                        }
                    }
                    catch (SQL::Exception &e)
                    {
                        // a broken database fails every batch, so report once until one succeeds
                        if (!reportedWriteFailure)
                        {
                            RAISE_ERROR_FROM_WORKER(
                                mc, "Browser Database Error",
                                std::string("Unable to write to the browser database. ") +
                                    e.what());
                            reportedWriteFailure = true;
                        }
                        for (auto *p : doThis)
                            delete p;
                    }
                }
            }
            if (!doThisNoWrite.empty())
            {
                for (auto *p : doThisNoWrite)
                {
                    p->go(*this);
                    delete p;
                }
            }
            if (keepRunning && (lastCount != nextCount))
            {
                // Unlike the scanner, we transaction bundle here already
                auto msg = scxt::messaging::client::BrowserQueueRefresh({-1, (int32_t)nextCount});
                messaging::client::clientSendToSerialization(msg, mc);
                lastCount = nextCount;
            }
        }
    }

    void addDebug(const std::string &m)
    {
        try
        {
            using namespace std::chrono;
            using clock = system_clock;

            auto there = SQL::Statement(
                dbh, "INSERT INTO DebugLog (\"time\", \"since\", \"msg\") VALUES (?1, ?2, ?3)");

            const auto start_2025 = time_point<system_clock>(seconds(1735689600));
            const auto current_time_point{clock::now()};
            const auto current_time{clock::to_time_t(current_time_point)};
            const auto current_localtime{*std::localtime(&current_time)};

            auto duration = current_time_point - start_2025;

            // Convert the duration to seconds
            auto dt = std::chrono::duration_cast<std::chrono::minutes>(duration);
            uint64_t dt_count = dt.count();

            std::ostringstream oss;
            oss << std::put_time(&current_localtime, "%c");
#if TRACE_DB
            SCLOG_IF(sqlDb, "Writing Test Startup Sentinel : " << oss.str());
#endif
            std::string res = oss.str();
            there.bind(1, res);
            there.bind(2, dt_count);
            there.bind(3, m);
            there.step();
            there.finalize();
        }
        catch (const SQL::Exception &e)
        {
            SCLOG_IF(warnings, "Unable to write browser database debug message: " << e.what());
        }
    }

    void addRemoveBrowserLocation(const fs::path &m, bool index, bool add)
    {
        try
        {
            if (add)
            {
                auto there = SQL::Statement(
                    dbh, "INSERT INTO BrowserLocations  (\"path\", \"isIndexed\") VALUES (?1, ?2)");

                std::string res = m.u8string();
                there.bind(1, res);
                there.bind(2, index);

                there.step();
                there.finalize();
            }
            else
            {
                auto there = SQL::Statement(dbh, "DELETE FROM BrowserLocations WHERE path==?1");

                std::string res = m.u8string();
                there.bind(1, res);

                there.step();
                there.finalize();
            }
        }
        catch (const SQL::Exception &e)
        {
            RAISE_ERROR_FROM_WORKER(mc, "Browser Database Error",
                                    std::string("Unable to ") + (add ? "add" : "remove") +
                                        " browser location '" + m.u8string() + "'. " + e.what());
        }
    }

    // FIXME for now I am coding this with a locked vector but probably a
    // thread safe queue is the way to go
    std::thread qThread;
    std::mutex qLock;
    std::condition_variable qCV;
    std::condition_variable waitingCV; // worker announces it has gone idle
    std::deque<EnQAble *> pathQ;
    std::atomic<bool> keepRunning{true};

    /*
     * Call this from any thread
     */
    void enqueueWorkItem(EnQAble *p)
    {
        {
            std::lock_guard<std::mutex> g(qLock);

            pathQ.push_back(p);
        }
        qCV.notify_all();
    }

    sqlite3 *getReadOnlyConn(bool notifyOnError = true)
    {
        if (!rodbh)
        {
            auto flag = SQLITE_OPEN_NOMUTEX; // basically lock
            flag |= SQLITE_OPEN_READONLY;

            // SCLOG_IF(sqlDb,  ">>>RO> Opening r/o DB" );;
            auto ec = sqlite3_open_v2(dbname.c_str(), &rodbh, flag, nullptr);

            if (ec != SQLITE_OK)
            {
                // callers are on the serialization thread and raise their own error
                if (notifyOnError)
                {
                    SCLOG_IF(warnings, "Unable to open the browser database '"
                                           << dbname << "' read only. The error was '"
                                           << sqlite3_errmsg(rodbh) << "'.");
                }
                if (rodbh)
                    sqlite3_close(rodbh);
                rodbh = nullptr;
            }
        }
        return rodbh;
    }

  private:
    sqlite3 *rodbh{nullptr};
    sqlite3 *dbh{nullptr};
};

} // namespace scxt::browser
#endif // WRITER_WORKER_H
