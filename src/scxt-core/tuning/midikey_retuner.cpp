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

#include "midikey_retuner.h"
#include "libMTSClient.h"
#include <algorithm>
#include <cmath>
#include "utils.h"

namespace scxt::tuning
{

MidikeyRetuner::MidikeyRetuner()
{
    mtsClient = MTS_RegisterClient();
    if (mtsClient && MTS_HasMaster(mtsClient))
    {
        tuningMode = MTS_ESP;
    }
}

MidikeyRetuner::~MidikeyRetuner()
{
    if (mtsClient)
    {
        MTS_DeregisterClient(mtsClient);
    }
}
bool MidikeyRetuner::hasMTSSource() const { return mtsClient && MTS_HasMaster(mtsClient); }

float MidikeyRetuner::offsetKeyBy(int channel, int key, bool force12TET)
{
    if (force12TET)
        return 0.f;
    switch (tuningMode)
    {
    case TWELVE_TET:
        return 0.f;
    case MTS_ESP:
    {
        if (!mtsClient)
            return 0.f;
        return MTS_RetuningInSemitones(mtsClient, key, channel);
    }
    case SCL_KBM:
    {
        if (!sclKbmValid)
            return 0.f;
        auto idx = std::clamp(key + RetuneTable::midiOffset, 0, RetuneTable::tableSize - 1);
        return sclKbmTable.semitones[idx];
    }
    }
    return 0.f;
}

int MidikeyRetuner::getRepetitionInterval(bool force12TET) const
{
    if (force12TET)
        return 12;
    switch (tuningMode)
    {
    case TWELVE_TET:
        return 12;
    case MTS_ESP:
    {
        if (!mtsClient)
            return 12;
        auto tmp = MTS_GetMapSize(mtsClient);
        if (tmp < 0)
            return 12;
        else
            return tmp;
    }
    case SCL_KBM:
        return sclKbmValid ? sclKbmTable.repetitionInterval : 12;
    }
    return 12;
}

void MidikeyRetuner::setTuningMode(TuningMode tm) { tuningMode = tm; }

void MidikeyRetuner::setSCLKBMTable(const RetuneTable &t)
{
    sclKbmTable = t;
    sclKbmValid = true;
}

void MidikeyRetuner::clearSCLKBM()
{
    sclKbmTable = RetuneTable();
    sclKbmValid = false;
}

int MidikeyRetuner::remapKeyTo(int channel, int key, bool force12TET)
{
    return (int)std::round(offsetKeyBy(channel, key, force12TET) + key);
}

float MidikeyRetuner::retuningForRemappedKey(int channel, int key, int preRemappedKey,
                                             bool force12TET)
{
    auto r = offsetKeyBy(channel, preRemappedKey, force12TET);
    return r - key + preRemappedKey;
}

float MidikeyRetuner::retuningForRemappedKeyWithInterpolation(int channel, int key,
                                                              int preRemappedKey, float mods,
                                                              bool force12TET)
{
    auto actualKeyPlusMods = preRemappedKey + mods;
    auto ik = (int)(actualKeyPlusMods);
    auto frac = actualKeyPlusMods - ik;

    auto r1 = offsetKeyBy(channel, ik, force12TET);
    auto r2 = offsetKeyBy(channel, ik + 1, force12TET);
    auto r = r1 + frac * (r2 - r1);

    auto res = r - (int)(key + mods) + ik;
    return res;
}

} // namespace scxt::tuning