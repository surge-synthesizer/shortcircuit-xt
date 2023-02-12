/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef __SCXT_LOGGING_H
#define __SCXT_LOGGING_H
#include <string>

namespace scxt::log
{

enum class Level
{
    None = 0, // turn off logging
    Error,
    Warning,
    Info,
    Debug
};

class LoggingCallback
{
  public:
    // These functions may be called from a separate thread. Implementers responsibility to
    // do any synchronization necessary.

    // highest desired logging level. higher values will be filtered out
    virtual Level getLevel() = 0;

    // message received
    virtual void message(Level lev, const std::string &msg) = 0;
};

} // namespace scxt::log

#endif