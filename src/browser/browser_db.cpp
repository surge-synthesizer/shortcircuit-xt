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

#include "browser_db.h"
#include "utils.h"
#include "sqlite3.h"

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>

#define TRACE_DB 0

namespace scxt::browser
{

namespace SQL
{
struct Exception : public std::runtime_error
{
    explicit Exception(sqlite3 *h) : std::runtime_error(sqlite3_errmsg(h)), rc(sqlite3_errcode(h))
    {
        // printStackTrace();
    }
    Exception(int rc, const std::string &msg) : std::runtime_error(msg), rc(rc)
    {
        // printStackTrace();
    }
    const char *what() const noexcept override
    {
        static char msg[1024];
        snprintf(msg, 1024, "SQL Error[%d]: %s", rc, std::runtime_error::what());
        return msg;
    }
    int rc;
};

struct LockedException : public Exception
{
    explicit LockedException(sqlite3 *h) : Exception(h)
    {
        // Surge::Debug::stackTraceToStdout();
    }
    LockedException(int rc, const std::string &msg) : Exception(rc, msg) {}
    const char *what() const noexcept override
    {
        static char msg[1024];
        snprintf(msg, 1024, "SQL Locked Error[%d]: %s", rc, std::runtime_error::what());
        return msg;
    }
    int rc;
};

void Exec(sqlite3 *h, const std::string &statement)
{
    char *emsg;
    auto rc = sqlite3_exec(h, statement.c_str(), nullptr, nullptr, &emsg);
    if (rc != SQLITE_OK)
    {
        std::string sm = emsg;
        sqlite3_free(emsg);
        throw Exception(rc, sm);
    }
}

/*
 * A little class to make prepared statements less clumsy
 */
struct Statement
{
    bool prepared{false};
    std::string statementCopy;
    Statement(sqlite3 *h, const std::string &statement) : h(h), statementCopy(statement)
    {
        auto rc = sqlite3_prepare_v2(h, statement.c_str(), -1, &s, nullptr);
        if (rc != SQLITE_OK)
            throw Exception(rc, "Unable to prepare statement [" + statement + "]");
        prepared = true;
    }
    ~Statement()
    {
        if (prepared)
        {
            std::cout << "ERROR: Prepared Statement never Finalized \n"
                      << statementCopy << "\n"
                      << std::endl;
        }
    }
    void finalize()
    {
        if (s)
            if (sqlite3_finalize(s) != SQLITE_OK)
                throw Exception(h);
        prepared = false;
    }

    int col_int(int c) const { return sqlite3_column_int(s, c); }
    int64_t col_int64(int c) const { return sqlite3_column_int64(s, c); }
    const char *col_charstar(int c) const
    {
        return reinterpret_cast<const char *>(sqlite3_column_text(s, c));
    }
    std::string col_str(int c) const { return col_charstar(c); }

    void bind(int c, const std::string &val)
    {
        if (!s)
            throw Exception(-1, "Statement not initialized in bind");

        auto rc = sqlite3_bind_text(s, c, val.c_str(), val.length(), SQLITE_STATIC);
        if (rc != SQLITE_OK)
            throw Exception(h);
    }

    void bind(int c, int val)
    {
        if (!s)
            throw Exception(-1, "Statement not initialized in bind");

        auto rc = sqlite3_bind_int(s, c, val);
        if (rc != SQLITE_OK)
            throw Exception(h);
    }

    void bindi64(int c, int64_t val)
    {
        if (!s)
            throw Exception(-1, "Statement not initialized in bind");

        auto rc = sqlite3_bind_int64(s, c, val);
        if (rc != SQLITE_OK)
            throw Exception(h);
    }

    void clearBindings()
    {
        if (!s)
            throw Exception(-1, "Statement not initialized in bind");

        auto rc = sqlite3_clear_bindings(s);
        if (rc != SQLITE_OK)
            throw Exception(h);
    }

    void reset()
    {
        if (!s)
            throw Exception(-1, "Statement not initialized in bind");

        auto rc = sqlite3_reset(s);
        if (rc != SQLITE_OK)
            throw Exception(h);
    }

    bool step() const
    {
        if (!s)
            throw Exception(-1, "Statement not initialized in step");

        auto rc = sqlite3_step(s);
        if (rc == SQLITE_ROW)
            return true;
        if (rc == SQLITE_DONE)
            return false;
        throw Exception(h);
    }

    sqlite3_stmt *s{nullptr};
    sqlite3 *h;
};

/*
 * RAII on transactions
 */
struct TxnGuard
{
    bool open{false};
    explicit TxnGuard(sqlite3 *e) : d(e)
    {
        auto rc = sqlite3_exec(d, "BEGIN IMMEDIATE TRANSACTION", nullptr, nullptr, nullptr);
        if (rc == SQLITE_LOCKED || rc == SQLITE_BUSY)
        {
            throw LockedException(d);
        }
        else if (rc != SQLITE_OK)
        {
            throw Exception(d);
        }
        open = true;
    }

    void end()
    {
        auto rc = sqlite3_exec(d, "END TRANSACTION", nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK)
        {
            throw Exception(d);
        }
        open = false;
    }
    ~TxnGuard()
    {
        if (open)
        {
            auto rc = sqlite3_exec(d, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr);
        }
    }
    sqlite3 *d;
};
} // namespace SQL

struct WriterWorker
{
    static constexpr const char *schema_version =
        "1003"; // I will rebuild if this is not my version

    static constexpr const char *setup_sql = R"SQL(
DROP TABLE IF EXISTS "DebugJunk";
DROP TABLE IF EXISTS "Version";
CREATE TABLE "Version" (
    id integer primary key,
    schema_version varchar(256)
);
CREATE TABLE DebugJunk (
    id integer primary key,
    time varchar(256),
    junk varchar(2048)
);
-- We create these tables only if missing of course since it is user data
CREATE TABLE IF NOT EXISTS DeviceLocations (
    id integer primary key,
    path varchar(2048)
);
    )SQL";
    struct EnQAble
    {
        virtual ~EnQAble() = default;
        virtual void go(WriterWorker &) = 0;
    };

    struct EnQDebugMsg : public EnQAble
    {
        std::string msg;
        EnQDebugMsg(const std::string &msg) : msg(msg) {}
        void go(WriterWorker &w) override { w.addDebug(msg); }
    };

    struct EnQDeviceLocation : public EnQAble
    {
        fs::path path;
        EnQDeviceLocation(const fs::path &msg) : path(msg) {}
        void go(WriterWorker &w) override { w.addDeviceLocation(path); }
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

    explicit WriterWorker(const fs::path &p)
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
        static constexpr auto transChunkSize = 10; // How many FXP to load in a single txn
        int lock_retries{0};
        while (keepRunning)
        {
            std::vector<EnQAble *> doThis;
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
                    std::copy(b, e, std::back_inserter(doThis));
                    pathQ.erase(b, e);
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
                            << "Patch database is locked for writing. Most likely, another Surge "
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
        }
    }

    void addDebug(const std::string &m)
    {
        try
        {
            using namespace std::chrono;
            using clock = system_clock;

            auto there =
                SQL::Statement(dbh, "INSERT INTO DebugJunk  (\"time\", \"junk\") VALUES (?1, ?2)");

            const auto current_time_point{clock::now()};
            const auto current_time{clock::to_time_t(current_time_point)};
            const auto current_localtime{*std::localtime(&current_time)};

            std::ostringstream oss;
            oss << std::put_time(&current_localtime, "%c");
#if TRACE_DB
            SCLOG("Writing Test Startup Sentinel : " << oss.str());
#endif
            std::string res = oss.str();
            there.bind(1, res);

            there.bind(2, m);
            there.step();
            there.finalize();
        }
        catch (const SQL::Exception &e)
        {
            // storage->reportError(e.what(), "PatchDB - Junk gave Junk");
            SCLOG(e.what());
        }
    }

    void addDeviceLocation(const fs::path &m)
    {
        try
        {
            auto there = SQL::Statement(dbh, "INSERT INTO DeviceLocations  (\"path\") VALUES (?1)");

            std::string res = m.u8string();
            there.bind(1, res);

            there.step();
            there.finalize();
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

BrowserDB::BrowserDB(const fs::path &p)
{
    writerWorker = std::make_unique<scxt::browser::WriterWorker>(p);
    writerWorker->openForWrite();
}

BrowserDB::~BrowserDB() {}

void BrowserDB::writeDebugMessage(const std::string &s)
{
    writerWorker->enqueueWorkItem(new WriterWorker::EnQDebugMsg(s));
}

void BrowserDB::addDeviceLocation(const fs::path &p)
{
    writerWorker->enqueueWorkItem(new WriterWorker::EnQDeviceLocation(p));
}

std::vector<fs::path> BrowserDB::getDeviceLocations()
{
    auto conn = writerWorker->getReadOnlyConn();
    std::vector<fs::path> res;

    // language=SQL
    std::string query = "SELECT path FROM DeviceLocations;";
    try
    {
        auto q = SQL::Statement(conn, query);
        while (q.step())
        {
            res.emplace_back(fs::path{q.col_str(0)});
        }
        q.finalize();
    }
    catch (SQL::Exception &e)
    {
        // storage->reportError(e.what(), "PatchDB - readFeatures");
        SCLOG(e.what());
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