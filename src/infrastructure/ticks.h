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

#ifndef SHORTCIRCUIT_TICKS_H
#define SHORTCIRCUIT_TICKS_H

#include "globals.h"

namespace scxt::Time
{

typedef int64_t Timestamp;
void getCurrentTimestamp(Timestamp *val);
void getTimestampDiff(Timestamp *a, Timestamp *b, Timestamp *diff);

} // namespace scxt::Time

#endif // SHORTCIRCUIT_TICKS_H
