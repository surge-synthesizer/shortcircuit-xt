/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

#include "test_main.h"
#include "infrastructure/profiler.h"
#include "infrastructure/ticks.h"
#include <chrono>
#include <thread>
#if WINDOWS
#include <windows.h>
#endif

#if 1
TEST_CASE("Profiler Basic", "[profiler]")
{

    SECTION("Timestamp functions")
    {
        SC3::Time::Timestamp a,b,d;
        SC3::Time::getCurrentTimestamp(&a);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        SC3::Time::getCurrentTimestamp(&b);
        SC3::Time::getTimestampDiff(&a, &b, &d);
        // this will not be exact
        std::cout << "Elapsed milliseconds " << d / 1000 << std::endl;
    }
    SECTION("Time One")
    {

        gTestLevel=SC3::Log::Level::Debug;
        SC3::Perf::Profiler p(gLogger);
        p.reset("Basic profile test");
        p.enter();
        p.enter();
        std::this_thread::sleep_for(std::chrono::milliseconds(123));
        p.exit("First");
        p.enter();
        std::this_thread::sleep_for(std::chrono::milliseconds(123));
        p.exit("Second");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        p.exit("Outer");


        p.dump("Final result");

    }
}
#endif
