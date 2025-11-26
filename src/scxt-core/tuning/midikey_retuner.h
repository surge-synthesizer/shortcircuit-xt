/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
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

    size_t getRepetitionInterval() const;

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
    float retuneRemappedKey(int channel, int key, int preRemappedKey);

  private:
    MTSClient *mtsClient{nullptr};
};
} // namespace scxt::tuning

#endif // SHORTCIRCUIT_MIDIKEY_RETUNER_H
