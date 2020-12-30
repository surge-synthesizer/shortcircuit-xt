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

} // namespace SC3::Log

#endif