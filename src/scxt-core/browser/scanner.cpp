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

#include <chrono>
#include "scanner.h"
#include "utils.h"
#include "writer_worker.h"
#include "browser.h"
#include "infrastructure/md5support.h"
#include "writer_worker.h"
#include "messaging/messaging.h"
#include "messaging/client/client_messages.h"

namespace scxt::browser
{

static uint64_t writeTimeInMinutes(const fs::path &p)
{
    auto tse = fs::last_write_time(p).time_since_epoch();
    auto mse = std::chrono::duration_cast<std::chrono::minutes>(tse).count();
    return mse;
}

struct ScanWorker
{
    Scanner &scanner;
    ScanWorker(Scanner &s) : scanner(s)
    {
        qThread = std::thread([this]() { this->processQueue(); });
        scanner.mc.threadingChecker.addAsAClientThread(qThread.get_id());
    }

    ~ScanWorker()
    {
        keepRunning = false;
        qCV.notify_all();
        qThread.join();
        for (auto *q : pathQLow)
            delete q;
        for (auto *q : pathQ)
            delete q;
    }

    void processQueue()
    {
        int lastCount{0}, nextCount{0};
        while (keepRunning)
        {
            EnQAble *p = nullptr;
            {
                std::unique_lock<std::mutex> g(qLock);
                if (pathQ.empty() && pathQLow.empty())
                    qCV.wait(g);
                if (keepRunning)
                {
                    if (!pathQ.empty())
                    {
                        p = pathQ.front();
                        pathQ.pop_front();
                    }
                    else if (!pathQLow.empty())
                    {
                        p = pathQLow.front();
                        pathQLow.pop_front();
                    }
                    nextCount = pathQ.size() + pathQLow.size();
                }
            }
            if (p && keepRunning)
            {
                p->go(*this);
                delete p;

                if (lastCount != nextCount)
                {
                    if (nextCount == 0 || lastCount == 0 || abs(lastCount - nextCount) > 20)
                    {
                        auto msg =
                            scxt::messaging::client::BrowserQueueRefresh({(int32_t)nextCount, -1});
                        messaging::client::clientSendToSerialization(msg, scanner.mc);
                        lastCount = nextCount;
                    }
                }
            }
        }
    }

    struct EnQAble
    {
        virtual ~EnQAble() = default;
        virtual void go(ScanWorker &w) = 0;
    };

    struct ScanFile : ScanWorker::EnQAble
    {
        fs::path path;
        ScanFile(const fs::path &p) : path(p) {}
        void go(ScanWorker &w) override
        {
            if (fs::exists(path))
            {
                auto sc = infrastructure::createMD5SumFromFile(path);
                auto sz = fs::file_size(path);
                auto mt = writeTimeInMinutes(path);
                w.scanner.writer.enqueueWorkItem(
                    new WriterWorker::EnQAddSampleInfo(path, sc, mt, sz));
            }
        }
    };
    struct ScanDir : EnQAble
    {
        fs::path path;
        uint64_t afterMtime;

        ScanDir(const fs::path &p, uint64_t mtime) : path(p), afterMtime(mtime) {}

        void go(ScanWorker &w) override
        {
            auto files = fs::directory_iterator(path);
            std::vector<fs::path> toScan;
            for (auto &dirent : files)
            {
                if (dirent.is_regular_file())
                {
                    if (browser::Browser::isLoadableFile(dirent.path()))
                    {
                        auto mse = writeTimeInMinutes(dirent.path());
                        if (mse > afterMtime)
                        {
                            toScan.push_back(dirent.path());
                        }
                    }
                }
                else if (dirent.is_directory())
                {
                    w.enqueueWorkItem(new ScanDir{dirent.path(), (uint64_t)afterMtime});
                }
            }

            for (const auto &s : toScan)
            {
                w.enqueueWorkLowPri(new ScanFile{s});
            }
        }
    };

    std::thread qThread;
    std::mutex qLock;
    std::condition_variable qCV;
    std::deque<EnQAble *> pathQ, pathQLow;
    std::atomic<bool> keepRunning{true};

    void enqueueWorkItem(EnQAble *p)
    {
        {
            std::lock_guard<std::mutex> g(qLock);

            pathQ.push_back(p);
        }
        qCV.notify_all();
    }

    void enqueueWorkLowPri(EnQAble *p)
    {
        {
            std::lock_guard<std::mutex> g(qLock);

            pathQLow.push_back(p);
        }
        qCV.notify_all();
    }
};

Scanner::Scanner(WriterWorker &w, messaging::MessageController &m) : writer(w), mc(m)
{
    scanWorker = std::make_unique<ScanWorker>(*this);
}

Scanner::~Scanner() {}

void Scanner::scanPath(const fs::path &path, uint64_t afterMtime)
{
    scanWorker->enqueueWorkItem(new ScanWorker::ScanDir{path, afterMtime});
}

} // namespace scxt::browser
