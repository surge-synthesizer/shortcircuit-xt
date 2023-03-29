/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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
    printf("-------- Stack Trace (%d frames of %d depth showing) --------\n", depth, frames);
    for (i = 1; i < frames && i < depth; ++i)
    {
        printf("  [%3d]: %s\n", i, strs[i]);
    }
    free(strs);
}
#else
void printStackTrace(int fd)
{
    SCDBGCOUT << "Implement printStackTrace for this OS please" << std::endl;
}
#endif

#if USE_SIMPLE_LEAK_DETECTOR
std::map<std::string, std::pair<int, int>> allocLog;
void showLeakLog()
{
    for (const auto &[k, v] : allocLog)
    {
        auto [a, d] = v;
        if (a != d)
            std::cout << "LEAK:   class=" << k << "  ctor=" << a << " dtor=" << d
                      << ((a != d) ? " ERROR!!" : " OK") << std::endl;
    }
}
#endif

} // namespace scxt