/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_SCXT_CORE_TUNING_MIDIKEY_RETUNER_H
#define SCXT_SRC_SCXT_CORE_TUNING_MIDIKEY_RETUNER_H

struct MTSClient;

namespace scxt::tuning
{
struct MidikeyRetuner
{
    enum TuningMode
    {
        TWELVE_TET,
        MTS_ESP
    } tuningMode{TWELVE_TET};

    MidikeyRetuner();
    ~MidikeyRetuner();

    void setTuningMode(TuningMode);

    int getRepetitionInterval() const;

    /*
     * Return the number of 12-tet smitones to retune by
     */
    float offsetKeyBy(int channel, int key);

    /*
     * We decompose offsetKeyBy into remapKeyTo and retuneRemappedKey
     * so we can find the appropriate key then retune it. This is just an
     * int part and frac part really.
     */
    int remapKeyTo(int channel, int key);
    float retuningForRemappedKey(int channel, int key, int preRemappedKey);
    float retuningForRemappedKeyWithInterpolation(int channel, int key, int preRemappedKey,
                                                  float mods);

  private:
    MTSClient *mtsClient{nullptr};
};
} // namespace scxt::tuning

#endif // SHORTCIRCUIT_MIDIKEY_RETUNER_H
