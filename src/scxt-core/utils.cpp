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

#include <iostream>
#include "utils.h"
#include <thread>
#include <mutex>
#include <deque>

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

#include "sst/plugininfra/version_information.h"
#include "configuration.h"

#if MAC || LINUX
#include <execinfo.h>
#include <stdio.h>
#include <cstdlib>
#endif

namespace scxt
{
#if MAC || LINUX
void printStackTrace(int depth)
{
    void *callstack[128];
    int i, frames = backtrace(callstack, 128);
    char **strs = backtrace_symbols(callstack, frames);
    if (depth < 0)
        depth = frames;
    SCLOG_IF(always, "-------- Stack Trace (" << depth << " frames of " << frames
                                              << " depth showing) --------");
    for (i = 1; i < frames && i < depth; ++i)
    {
        SCLOG_IF(always, "  [" << i << "]:  " << strs[i]);
    }
    free(strs);
}
#else
void printStackTrace(int fd) { SCLOG_IF(warnings, "printStackTrace unavailable on this platform"); }
#endif

std::mutex logMutex;
std::deque<std::string> logMessages;
void postToLog(const std::string &s)
{
    // TODO this sucks also
    auto q = s;
    auto sp = q.find(sst::plugininfra::VersionInformation::cmake_source_dir);

    if (sp == 0)
    {
        q = q.substr(sp + strlen(sst::plugininfra::VersionInformation::cmake_source_dir) + 1);
    }
    std::cout << q << std::flush;
    std::lock_guard<std::mutex> g(logMutex);
    logMessages.push_back(q);

    if (logMessages.size() > 525)
    {
        logMessages.erase(logMessages.begin(), logMessages.begin() + 25);
    }
}

std::string getFullLog()
{
    std::ostringstream oss;
    {
        // TODO - obviously this sucks
        std::lock_guard<std::mutex> g(scxt::logMutex);
        for (const auto row : scxt::logMessages)
            oss << row;
    }
    return oss.str();
}

// Thanks
// https://stackoverflow.com/questions/16077299/how-to-print-current-time-with-milliseconds-using-c-c11
std::string logTimestamp()
{
    using namespace std::chrono;
    using clock = system_clock;

    const auto current_time_point{clock::now()};
    const auto current_time{clock::to_time_t(current_time_point)};
    const auto current_localtime{*std::localtime(&current_time)};
    const auto current_time_since_epoch{current_time_point.time_since_epoch()};
    const auto current_milliseconds{duration_cast<milliseconds>(current_time_since_epoch).count() %
                                    1000};

    std::ostringstream stream;
    stream << std::put_time(&current_localtime, "%T") << "." << std::setw(3) << std::setfill('0')
           << current_milliseconds;
    return stream.str();
}
} // namespace scxt