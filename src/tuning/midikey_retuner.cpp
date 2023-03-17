/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#include "midikey_retuner.h"
#include "libMTSClient.h"
#include <cmath>

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
        return 0;
    case MTS_ESP:
    {
        if (!mtsClient)
            return 0;
        return MTS_RetuningInSemitones(mtsClient, key, channel);
    }
    }
}

void MidikeyRetuner::setTuningMode(TuningMode tm) { tuningMode = tm; }

int MidikeyRetuner::remapKeyTo(int channel, int key)
{
    return (int)std::round(offsetKeyBy(channel, key) + key);
}

float MidikeyRetuner::retuneRemappedKey(int channel, int key, int preRemappedKey)
{
    auto r = offsetKeyBy(channel, preRemappedKey);
    return r - key + preRemappedKey;
}

} // namespace scxt::tuning