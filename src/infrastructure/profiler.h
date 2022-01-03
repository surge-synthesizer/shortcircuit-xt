/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

/*
 * Code performance profiler (@rudeog)
 * Every time enter is called a timestamp is pushed on a stack. Every time exit is
 * called, the timestamp is popped from the stack and subtracted from the current
 * time. This effective elapsed time is added to the time stored in a bucket with the
 * same ID as is passed to the exit function. If other enter/exit pairs were pushed/popped
 * after the first enter call, their effective time is not counted in the effective time for
 * the first pair.
 */

#ifndef SHORTCIRCUIT_PROFILER_H
#define SHORTCIRCUIT_PROFILER_H

#include "logging.h"

namespace SC3::Perf
{

class Profiler
{
    void *mData;

  public:
    // implicit reset (msg is initial msg)
    Profiler(SC3::Log::LoggingCallback *logger, const char *msg = 0);

    ~Profiler();

    // reset the profiler
    void reset(const char *msg);

    // dump statistics to log at debug level
    void dump(const char *msg);

    // enter a section of code to profile
    void enter();

    // exit a section of code (id should not exceed max 255)
    void exit(const char *id);
};

} // namespace SC3::Perf
#endif // SHORTCIRCUIT_PROFILER_H
