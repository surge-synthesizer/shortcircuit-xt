//
// Created by Paul Walker on 2/1/23.
//

#ifndef SCXT_SRC_TUNING_EQUAL_H
#define SCXT_SRC_TUNING_EQUAL_H

#include <cstddef>

namespace scxt::tuning
{
struct EqualTuningProvider
{
    void init();
    /**
     * note is float offset from note 69 / A440
     * return is 2^(note * 12), namely frequency / 440.0
     */
    float note_to_pitch(float note);

  protected:
    static constexpr size_t tuning_table_size = 512;
    float table_pitch alignas(16)[tuning_table_size];
    float table_pitch_inv alignas(16)[tuning_table_size];
    float table_note_omega alignas(16)[2][tuning_table_size];
    // 2^0 -> 2^+/-1/12th. See comment in note_to_pitch
    float table_two_to_the alignas(16)[1001];
    float table_two_to_the_minus alignas(16)[1001];
};
extern EqualTuningProvider equalTuning;
} // namespace scxt::tuning

#endif // __SCXT_EQUAL_H
