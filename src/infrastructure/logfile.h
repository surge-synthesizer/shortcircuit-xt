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

#ifndef __SC3_INFRA_LOGFILE_H
#define __SC3_INFRA_LOGFILE_H

#include <iostream>
#include <sstream>
#include "logging.h"

// TODO add var to cmake and use it to determine this var's value
#define LOGGING_DEBUG_ENABLED 1

namespace SC3::Log
{
/*
 * A function which writes a line to the log
 */
void write_log(const char *text);

/*
 * A little class which is an ostream so you can do
 * SC3::Log::logos() << "This is my message " << foo << "\n";
 */
class logos : public std::ostream
{
  private:
    struct lbuf : public std::stringbuf
    {
        lbuf() = default;
        int sync() override {
            int ret = std::stringbuf::sync();
            write_log(str().c_str());
            return ret;
        }
    } buff;
  public:
    logos() : buff(), std::ostream(&buff) {}
};

// And baconpaul's favorite debug helper
#define _D(x) " " << (#x) << "=" << x

// TODO above is deprecated. refactor to use below

// macro helpers for stream based logging. pass the StreamLogger object as parameter
#ifdef LOGGING_DEBUG_ENABLED
#define LOGDEBUG(x) if(x.setLevel(SC3::Log::Level::Debug)) x
#else
#define LOGDEBUG(x) if(0) x
#endif
#define LOGINFO(x) if(x.setLevel(SC3::Log::Level::Info)) x
#define LOGWARNING(x) if(x.setLevel(SC3::Log::Level::Warning)) x
#define LOGERROR(x) if(x.setLevel(SC3::Log::Level::Error)) x

// StreamLogger class is for stream based logging eg:
// logger << "logmessage" ; // add message to buffer at the current level 
// logger << "logmessage" << std::flush ; // send a log message at the current level now (ie flush now)
// logger->setLevel(ERROR); // change the current level (preferable to use above macros though)
class StreamLogger : public std::ostream
{
  private:
    struct lbuf : public std::stringbuf
    {
        public:
        LoggingCallback *mCB;
        Level mLevel;
        int sync() override {
            int ret = std::stringbuf::sync();
            if(ret)
              return ret;

            if(mCB && mCB->getLevel() >= mLevel) {
                auto s=str();
                if(!s.empty()) {
                  mCB->message(mLevel, s);
                }
            }
            str("");
            return 0;
        }
        lbuf(LoggingCallback *cb) : mCB(cb) , mLevel(Level::Error) {}
        ~lbuf() {pubsync();}
    } buff;
  public:

    StreamLogger(LoggingCallback *cb) : buff(cb), std::ostream(&buff) {}

    bool setLevel(const Level lev) {
      buff.pubsync(); // flush old before setting new level
      buff.mLevel=lev;
      // avoid unnecessary stream evaluations
      return (buff.mCB && buff.mCB->getLevel() >= buff.mLevel);
    }
};

inline const char *getShortLevelStr(Level lev) {
  return lev==SC3::Log::Level::Debug ? "[DEBUG] " :
            lev==SC3::Log::Level::Info ? "[INFO ] " :
            lev==SC3::Log::Level::Warning ? "[WARN ] " :
            lev==SC3::Log::Level::Error ? "[ERROR] " : "";
}

} // namespace SC3::Log

#endif