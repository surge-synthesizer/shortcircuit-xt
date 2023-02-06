//
// Created by Paul Walker on 1/30/23.
//

#ifndef SCXT_SRC_DSP_SINC_TABLE_H
#define SCXT_SRC_DSP_SINC_TABLE_H

#include "resampling.h"

namespace scxt::dsp
{
struct SincTable
{
    void init();

    // TODO Rename these when i'm all done
    float SincTableF32[(FIRipol_M + 1) * FIRipol_N];
    float SincOffsetF32[(FIRipol_M)*FIRipol_N];
    short SincTableI16[(FIRipol_M + 1) * FIRipol_N];
    short SincOffsetI16[(FIRipol_M)*FIRipol_N];

  private:
    bool initialized{false};
};

extern SincTable sincTable;
} // namespace scxt::dsp

#endif // __SCXT_DSP_SINC_TABLES_H
