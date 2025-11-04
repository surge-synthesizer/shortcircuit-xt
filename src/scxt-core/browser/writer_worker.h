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
                SCLOG("Unable to insert into SampleInfo: " << e.what())
            }
        }
    };

    void openDb()
    {
#if TRACE_DB
        SCLOG(">>> Opening r/w DB");
#endif
        auto flag = SQLITE_OPEN_NOMUTEX; // basically lock
        flag |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

        auto ec = sqlite3_open_v2(dbname.c_str(), &dbh, flag, nullptr);

        if (ec != SQLITE_OK)
        {
            std::ostringstream oss;
            oss << "An error occurred opening sqlite file '" << dbname << "'. The error was '"
                << sqlite3_errmsg(dbh) << "'.";
            // storage->reportError(oss.str(), "Surge Patch Database Error");
            SCLOG(oss.str());
            if (dbh)
            {
                // even if opening fails we still need to close the database
                sqlite3_close(dbh);
            }
            dbh = nullptr;
            return;
        }
    }

    void closeDb()
    {
#if TRACE_DB
        SCLOG("<<<< Closing r/w DB");
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
            SCLOG("Done with EnQSetup");
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
        SCLOG("PatchDB : Setup Database " << dbname);
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
                SCLOG("        : schema check. DBVersion='" << ver << "' SchemaVersion='"
                                                            << schema_version << "'");
#endif
                if (strcmp(ver, schema_version) == 0)
                {
#if TRACE_DB
                    SCLOG("        : Schema matches. Reusing database.");
#endif
                    rebuild = false;
                }
            }

            st.finalize();
        }
        catch (const SQL::Exception &e)
        {
            rebuild = true;
            /*
             * In this case, we choose to not report the error since it means
             * that we just need to rebuild everything
             */
            // storage->reportError(e.what(), "SQLLite3 Startup Error");
        }

        char *emsg;
        if (rebuild)
        {
#if TRACE_DB
            SCLOG("        : Schema missing or mismatched. Dropping and Rebuilding Database.");
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
                // storage->reportError(e.what(), "PatchDB Setup Error");
                SCLOG(e.what());
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

        {
            std::lock_guard<std::mutex> g(qLock);
            pathQ.push_back(new EnQSetup());
        }
        qCV.notify_all();
        while (!waiting)
        {
        }
    }

    ~WriterWorker()
    {
        if (haveOpenedForWriteOnce)
        {
            keepRunning = false;
            qCV.notify_all();
            qThread.join();
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
                }
                else
                {
                    try
                    {
                        SQL::TxnGuard tg(dbh);

                        for (auto *p : doThis)
                        {
                            p->go(*this);
                            delete p;
                        }

                        tg.end();
                    }
                    catch (SQL::LockedException &le)
                    {
                        std::ostringstream oss;
                        oss << le.what() << "\n"
                            << "Patch database is locked for writing. Most likely, another "
                               "Shortcircuit "
                               "XT instance has exclusive write access. We will attempt to retry "
                               "writing up to 10 more times. "
                               "Please dismiss this error in the meantime!\n\n Attempt: "
                            << lock_retries;
                        // storage->reportError(oss.str(), "Patch Database Locked");
                        SCLOG(oss.str());
                        // OK so in this case, we reload doThis onto the front of the queue and
                        // sleep
                        lock_retries++;
                        if (lock_retries < 10)
                        {
                            {
                                std::unique_lock<std::mutex> lk(qLock);
                                std::reverse(doThis.begin(), doThis.end());
                                for (auto p : doThis)
                                {
                                    pathQ.push_front(p);
                                }
                            }
                            std::this_thread::sleep_for(std::chrono::seconds(lock_retries * 3));
                        }
                        else
                        {
                            /*storage->reportError(
                                "Database is locked and unwritable after multiple attempts!",
                                "Patch Database Locked");*/
                            SCLOG("Database Locked");
                        }
                    }
                    catch (SQL::Exception &e)
                    {
                        // storage->reportError(e.what(), "Patch DB");
                        SCLOG(e.what());
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
            SCLOG("Writing Test Startup Sentinel : " << oss.str());
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
            // storage->reportError(e.what(), "PatchDB - Junk gave Junk");
            SCLOG(e.what());
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
            // storage->reportError(e.what(), "PatchDB - Junk gave Junk");
            SCLOG(e.what());
        }
    }

    // FIXME for now I am coding this with a locked vector but probably a
    // thread safe queue is the way to go
    std::thread qThread;
    std::mutex qLock;
    std::condition_variable qCV;
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

            // SCLOG( ">>>RO> Opening r/o DB" );;
            auto ec = sqlite3_open_v2(dbname.c_str(), &rodbh, flag, nullptr);

            if (ec != SQLITE_OK)
            {
                if (notifyOnError)
                {
                    std::ostringstream oss;
                    oss << "An error occurred opening r/o sqlite file '" << dbname
                        << "'. The error was '" << sqlite3_errmsg(dbh) << "'.";
                    // storage->reportError(oss.str(), "Surge Patch Database Error");
                    SCLOG(oss.str());
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

};     // namespace scxt::browser
#endif // WRITER_WORKER_H
