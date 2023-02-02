//
// Created by Paul Walker on 2/1/23.
//

#ifndef SHORTCIRCUIT_TIMEDATA_H
#define SHORTCIRCUIT_TIMEDATA_H

namespace scxt::datamodel
{
struct TimeData
{
    double tempo;
    double ppqPos;
    float pos_in_beat, pos_in_2beats, pos_in_bar, pos_in_2bars, pos_in_4bars;
    int timeSigNumerator{4}, timeSigDenominator{4};
};
}
#endif // SHORTCIRCUIT_TIMEDATA_H
