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
float MidikeyRetuner::offsetKeyBy(int channel, int key)
{
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
    }
    return 0.f;
}

int MidikeyRetuner::getRepetitionInterval() const
{
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
    }
    return 12;
}

void MidikeyRetuner::setTuningMode(TuningMode tm) { tuningMode = tm; }

int MidikeyRetuner::remapKeyTo(int channel, int key)
{
    return (int)std::round(offsetKeyBy(channel, key) + key);
}

float MidikeyRetuner::retuningForRemappedKey(int channel, int key, int preRemappedKey)
{
    auto r = offsetKeyBy(channel, preRemappedKey);
    return r - key + preRemappedKey;
}

float MidikeyRetuner::retuningForRemappedKeyWithInterpolation(int channel, int key,
                                                              int preRemappedKey, float mods)
{
    auto actualKeyPlusMods = preRemappedKey + mods;
    auto ik = (int)(actualKeyPlusMods);
    auto frac = actualKeyPlusMods - ik;

    auto r1 = offsetKeyBy(channel, ik);
    auto r2 = offsetKeyBy(channel, ik + 1);
    auto r = r1 + frac * (r2 - r1);

    auto res = r - (int)(key + mods) + ik;
    return res;
}

} // namespace scxt::tuning