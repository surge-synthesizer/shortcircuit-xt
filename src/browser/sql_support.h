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

#ifndef SCXT_SRC_BROWSER_SQL_SUPPORT_H
#define SCXT_SRC_BROWSER_SQL_SUPPORT_H

#include "sqlite3.h"
#include "utils.h"

namespace scxt::browser::SQL
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

inline void Exec(sqlite3 *h, const std::string &statement)
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
} // namespace scxt::browser::SQL
#endif // SQL_SUPPORT_H
