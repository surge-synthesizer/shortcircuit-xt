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

#include "bus.h"

#include "sst/basic-blocks/simd/setup.h"

#include "configuration.h"

#include "dsp/data_tables.h"
#include "engine.h"

#include "dsp/data_tables.h"
#include "tuning/equal.h"

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/PanLaws.h"

#include "messaging/messaging.h"

namespace mech = sst::basic_blocks::mechanics;

namespace scxt::engine
{
std::string getBusAddressLabel(BusAddress rt, const std::string &defaultName, bool shortName)
{
    if (rt == scxt::engine::BusAddress::ERROR_BUS)
    {
        if (shortName)
            return "ERR";
        return "Error";
    }
    else if (rt == scxt::engine::BusAddress::DEFAULT_BUS)
    {
        return defaultName;
    }
    else if (rt == scxt::engine::BusAddress::MAIN_0)
    {
        if (shortName)
            return "MAIN";

        return "Main";
    }
    else if (rt < scxt::engine::BusAddress::AUX_0)
    {
        if (shortName)
            return "PT" + std::to_string(rt - scxt::engine::BusAddress::PART_0 + 1);
        return "Part " + std::to_string(rt - scxt::engine::BusAddress::PART_0 + 1);
    }
    else
    {
        if (shortName)
            return "AX" + std::to_string(rt - scxt::engine::BusAddress::AUX_0 + 1);
        return "Aux " + std::to_string(rt - scxt::engine::BusAddress::AUX_0 + 1);
    }
}

void Bus::setBusEffectType(Engine &e, int idx, scxt::engine::AvailableBusEffects t)
{
    assert(idx >= 0 && idx < maxEffectsPerBus);
    busEffects[idx] = createEffect(t, &e, &busEffectStorage[idx]);
    if (busEffects[idx])
        busEffects[idx]->init(true);
}

void Bus::initializeAfterUnstream(Engine &e)
{
    for (int idx = 0; idx < maxEffectsPerBus; ++idx)
    {
        busEffects[idx] = createEffect(busEffectStorage[idx].type, &e, &busEffectStorage[idx]);
        if (busEffects[idx])
        {
            busEffects[idx]->init(false);
            sendBusEffectInfoToClient(e, idx);
        }
    }
}
void Bus::process()
{
    if (hasOSSignal)
    {
        if (!previousHadOSSignal)
        {
            downsampleFilter.reset();
        }
        downsampleFilter.process_block_D2(outputOS[0], outputOS[1], blockSize << 1);
        mech::accumulate_from_to<blockSize>(outputOS[0], output[0]);
        mech::accumulate_from_to<blockSize>(outputOS[1], output[1]);
    }
    previousHadOSSignal = hasOSSignal;
    // ToDo - we can skip these if theres no pre or at routing
    if (busSendStorage.supportsSends && busSendStorage.hasSends)
    {
        memcpy(auxoutputPreFX, output, sizeof(output));
    }

    int idx{0};
    for (auto &fx : busEffects)
    {
        if (fx && busEffectStorage[idx].isActive)
        {
            fx->process(output[0], output[1]);
        }
        idx++;
    }

    if (busSendStorage.supportsSends && busSendStorage.hasSends)
    {
        memcpy(auxoutputPreVCA, output, sizeof(output));
    }

    // If level becomes modulatable we want to lipol this
    if (busSendStorage.pan != 0.f)
    {
        namespace pl = sst::basic_blocks::dsp::pan_laws;
        // For now we don't interpolate over the block for pan
        auto pv = (std::clamp(busSendStorage.pan, -1.f, 1.f) + 1) * 0.5;
        pl::panmatrix_t pmat{1, 1, 0, 0};

        pl::stereoEqualPower(pv, pmat);

        for (int i = 0; i < blockSize; ++i)
        {
            auto il = output[0][i];
            auto ir = output[1][i];
            output[0][i] = pmat[0] * il + pmat[2] * ir;
            output[1][i] = pmat[1] * ir + pmat[3] * il;
        }
    }
    if (busSendStorage.level != 1.f)
    {
        auto lv = busSendStorage.level * busSendStorage.level * busSendStorage.level;
        mech::scale_by<blockSize>(lv, output[0], output[1]);
    }

    if ((busSendStorage.mute && !busSendStorage.solo) || mutedDueToSoloAway)
    {
        memset(output, 0, sizeof(output));
    }

    if (busSendStorage.supportsSends && busSendStorage.hasSends)
    {
        memcpy(auxoutputPostVCA, output, sizeof(output));
    }

    float a = vuFalloff;

    for (int c = 0; c < 2; ++c)
    {
        vuLevel[c] = std::min(2.f, a * vuLevel[c]);
        /* we use this as a proxy for a silent block without having to scan the entire
         * block. If the block isn't silent, we will get the next vlock if we just get
         * unlucky with sample 0 except in a super pathological case of an exactly
         * 16 sample sin wave or something.
         */
        if (std::fabs(output[c][0]) > 1e-15)
        {
            vuLevel[c] = std::max((float)vuLevel[c], mech::blockAbsMax<BLOCK_SIZE>(output[c]));
        }
    }
}

void Bus::sendBusEffectInfoToClient(const scxt::engine::Engine &e, int slot)
{
    std::array<datamodel::pmd, BusEffectStorage::maxBusEffectParams> pmds;

    int saz{0};
    if (busEffects[slot])
    {
        for (int i = 0; i < busEffects[slot]->numParams(); ++i)
        {
            pmds[i] = busEffects[slot]->paramAt(i);
        }
        saz = busEffects[slot]->numParams();
    }
    for (int i = saz; i < BusEffectStorage::maxBusEffectParams; ++i)
        pmds[i].type = sst::basic_blocks::params::ParamMetaData::NONE;

    messaging::client::serializationSendToClient(
        messaging::client::s2c_bus_effect_full_data,
        messaging::client::busEffectFullData_t{
            (int)address, -1, slot, {pmds, busEffectStorage[slot]}},
        *(e.getMessageController()));
}

void Bus::sendBusSendStorageToClient(const scxt::engine::Engine &e)
{
    messaging::client::serializationSendToClient(
        messaging::client::s2c_bus_send_data,
        messaging::client::busSendData_t{(int)address, -1, busSendStorage},
        *(e.getMessageController()));
}

void Bus::onSampleRateChanged()
{
    for (auto &fx : busEffects)
    {
        if (fx)
        {
            fx->onSampleRateChanged();
        }
    }
}

std::string
Bus::BusSendStorage::toStringAuxLocation(const scxt::engine::Bus::BusSendStorage::AuxLocation &p)
{
    switch (p)
    {
    case PRE_FX:
        return "pre_fx";
    case POST_FX_PRE_VCA:
        return "post_fx_pre_vca";
    case POST_VCA:
        return "post_vca";
    }
    return "post_vca";
}

Bus::BusSendStorage::AuxLocation Bus::BusSendStorage::fromStringAuxLocation(const std::string &s)
{
    static auto inverse =
        makeEnumInverse<Bus::BusSendStorage::AuxLocation, Bus::BusSendStorage::toStringAuxLocation>(
            Bus::BusSendStorage::AuxLocation::PRE_FX, Bus::BusSendStorage::AuxLocation::POST_VCA);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return POST_VCA;
    return p->second;
}
} // namespace scxt::engine
