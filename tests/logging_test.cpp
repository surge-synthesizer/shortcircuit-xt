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

#include "test_main.h"
#include <vector>
// ensure debug is tested
#define LOGGING_DEBUG_ENABLED 1
#include "infrastructure/logfile.h"


using namespace SC3::Log;

class TestCallback : public LoggingCallback {
public:
    TestCallback() : mTotal(0){}
    int *mTotal;
    std::vector<std::pair<Level,std::string> > mResults;
    SC3::Log::Level mLevel;
    SC3::Log::Level getLevel() {return mLevel;}
    void message(Level lev, const std::string &msg) {
        if(mTotal) (*mTotal)++;
        mResults.push_back(std::make_pair(lev,msg));
     }
};


TEST_CASE("Test logging","[logging]")
{

    SECTION("basic"){
        TestCallback cb;
        StreamLogger logger(&cb);
        // error and below (warning, error)
        cb.mLevel=Level::Warning;
        LOGERROR(logger) << "error 1" << std::flush;
        LOGWARNING(logger) << "warning" << std::flush;
        LOGERROR(logger) << "error 2" << std::flush;
        LOGINFO(logger) << "info" << std::flush;
        LOGDEBUG(logger) << "debug" << std::flush;
        LOGERROR(logger) << "error " ;
        logger << "3" << std::flush; // multi-part
        REQUIRE(cb.mResults.size()==4); // should not have debug case or info case
        REQUIRE((cb.mResults[0].first==Level::Error && cb.mResults[0].second.compare("error 1")==0));
        REQUIRE((cb.mResults[1].first==Level::Warning && cb.mResults[1].second.compare("warning")==0));
        REQUIRE((cb.mResults[2].first==Level::Error && cb.mResults[2].second.compare("error 2")==0));
        // multi part
        REQUIRE((cb.mResults[3].first==Level::Error && cb.mResults[3].second.compare("error 3")==0));
    }

    SECTION("avoid evaluations"){
        // make sure that expressions are not evaluated if the level is filtered out
        bool first_eval=false;
        bool second_eval=false;

        auto eval1 = [&]() -> int {first_eval=true; return 0;};
        auto eval2 = [&]() -> int {second_eval=true; return 0;};

        TestCallback cb;
        StreamLogger logger(&cb);
        // error and below (warning, error)
        cb.mLevel=Level::Warning;
        LOGERROR(logger) << "should eval" << eval1() << std::flush;
        LOGINFO(logger) << "should not eval" << eval2() << std::flush;
        
        REQUIRE(cb.mResults.size()==1); 
        REQUIRE(first_eval);
        REQUIRE(!second_eval);
    }

    SECTION("flush when close"){
        // make sure that when the logger goes out of scope it flushes the buffer
        TestCallback cb;
        int tot=0;
        {
            StreamLogger logger(&cb);
            cb.mTotal=&tot;
            // error and below (warning, error)
            cb.mLevel=Level::Warning;
            LOGERROR(logger) << "no flush";
        }
        
        REQUIRE(tot==1); 
    }

    SECTION("null callback avoid evaluations"){
        // make sure that expressions are not evaluated if cb is null
        bool first_eval=false;
        
        auto eval1 = [&]() -> int {first_eval=true; return 0;};

        StreamLogger logger(0);
        LOGERROR(logger) << "should not eval" << eval1() << std::flush;
        REQUIRE(!first_eval);
    }
    
}