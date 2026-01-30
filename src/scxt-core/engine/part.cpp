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

#include <ranges>
#include "part.h"
#include "bus.h"
#include "patch.h"
#include "engine.h"
#include "feature_enums.h"

#include "selection/selection_manager.h"

#include "sst/basic-blocks/simd/setup.h"
#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/PanLaws.h"
#include "messaging/messaging.h"

namespace scxt::engine
{
void Part::process(Engine &e)
{
    namespace blk = sst::basic_blocks::mechanics;

    float lcp alignas(16)[2][blockSize];
    float defOut alignas(16)[2][blockSize];

    macroLagHandler.process();
    externalSignalLag.processAll();

    auto lev = configuration.level;
    lev = lev * lev * lev;

    bool defaultAssigned{false};
    bool noGroups{true};

    namespace pl = sst::basic_blocks::dsp::pan_laws;
    auto pan = configuration.pan;
    pl::panmatrix_t pmat{1, 1, 0, 0};
    if (pan != 0)
        pl::stereoEqualPower(0.5 * (pan + 1), pmat);

    for (const auto &g : groups)
    {
        if (g->isActive())
        {
            noGroups = false;
            g->process(e);

            auto bi = g->outputInfo.routeTo;
            if (bi == DEFAULT_BUS || bi == configuration.routeTo)
            {
                blk::mul_block<blockSize>(g->output[0], lev, lcp[0]);
                blk::mul_block<blockSize>(g->output[1], lev, lcp[1]);

                if (pan != 0.0)
                {
                    for (int i = 0; i < blockSize; ++i)
                    {
                        auto il = lcp[0][i];
                        auto ir = lcp[1][i];
                        lcp[0][i] = pmat[0] * il + pmat[2] * ir;
                        lcp[1][i] = pmat[1] * ir + pmat[3] * il;
                    }
                }

                if (defaultAssigned)
                {
                    blk::accumulate_from_to<blockSize>(lcp[0], defOut[0]);
                    blk::accumulate_from_to<blockSize>(lcp[1], defOut[1]);
                }
                else
                {
                    blk::copy_from_to<blockSize>(lcp[0], defOut[0]);
                    blk::copy_from_to<blockSize>(lcp[1], defOut[1]);
                    defaultAssigned = true;
                }
            }
            else
            {
                auto &obus = e.getPatch()->getBusForOutput(bi);

                blk::accumulate_from_to<blockSize>(g->output[0], obus.output[0]);
                blk::accumulate_from_to<blockSize>(g->output[1], obus.output[1]);
            }
        }
    }

    if (noGroups)
        memset(defOut, 0, sizeof(defOut));
    else
        silenceTime = 0;

    if (defaultAssigned || noGroups)
    {
        silenceMax = 0;
        // this should be the route to point
        auto bi = configuration.routeTo;
        if (configuration.routeTo == DEFAULT_BUS)
            bi = (BusAddress)(PART_0 + partNumber);

        int idx{0};
        for (auto &p : partEffects)
        {
            if (p && partEffectStorage[idx].isActive)
            {
                p->process(defOut[0], defOut[1]);
                silenceMax += p->silentSamplesLength();
            }
            idx++;
        }

        auto &obus = e.getPatch()->getBusForOutput(bi);

        blk::accumulate_from_to<blockSize>(defOut[0], obus.output[0]);
        blk::accumulate_from_to<blockSize>(defOut[1], obus.output[1]);
    }
    auto lv = blk::blockAbsMax<blockSize>(defOut[0]) + blk::blockAbsMax<blockSize>(defOut[1]);
    if (lv > silenceThresh)
    {
        silenceTime = 0;
    }
    else
    {
        silenceTime += blockSize;
    }
}

bool Part::isActive()
{
    if (!configuration.active)
        return false;
    auto res = activeGroups != 0;
    auto ringout = silenceMax > 0 && silenceTime < silenceMax;

    if (log::ringout)
    {
        static bool was = false;
        auto act = res || ringout;
        if (act != was)
            SCLOG_IF(ringout, "Part " << partNumber << " active= " << act << " " << silenceTime
                                      << " " << silenceMax << " ");
        static int ag{-1};
        if (ag != activeGroups)
            SCLOG_IF(ringout, "Part " << partNumber << " activeGroups=" << activeGroups);
        ag = activeGroups;

        was = act;
    }

    return res || ringout;
}

Part::zoneMappingSummary_t Part::getZoneMappingSummary()
{
    zoneMappingSummary_t res;

    int pidx{partNumber};
    int gidx{0};
    for (const auto &g : groups)
    {
        int zidx{0};
        for (const auto &z : *g)
        {
            // get address for zone
            auto addr = selection::SelectionManager::ZoneAddress(pidx, gidx, zidx);
            int32_t features{0};
            if (z->missingSampleCount() > 0)
            {
                features |= GroupZoneFeatures::MISSING_SAMPLE;
            }
            auto data = zoneMappingItem_t{addr, z->mapping.keyboardRange, z->mapping.velocityRange,
                                          z->getName(), features};
            // res[addr] = data;
            res.emplace_back(data);
            zidx++;
        }
        gidx++;
    }
    return res;
}

std::vector<SampleID> Part::getSamplesUsedByPart() const
{
    std::unordered_set<SampleID> resSet;
    for (const auto &g : groups)
    {
        for (const auto &z : g->getZones())
        {
            for (const auto &var : z->variantData.variants)
            {
                if (var.sampleID.isValid())
                {
                    resSet.insert(var.sampleID);
                }
            }
        }
    }
    return std::vector<SampleID>(resSet.begin(), resSet.end());
}

size_t Part::addGroup()
{
    auto g = std::make_unique<Group>(this->parentPatch->parentEngine->rng);

    g->parentPart = this;
    g->setSampleRate(getSampleRate());

    std::unordered_set<std::string> gn;
    std::string gpfx = "New Group";
    for (const auto &og : groups)
    {
        if (og->name.find(gpfx) != std::string::npos)
            gn.insert(og->name);
    }

    std::string cn = gpfx;
    auto ngid = 1;
    auto found = gn.find(cn) != gn.end();
    while (found)
    {
        ngid++;
        cn = gpfx + " (" + std::to_string(ngid) + ")";
        found = gn.find(cn) != gn.end();
    }

    g->name = cn;

    groups.push_back(std::move(g));
    return groups.size();
}

void Part::moveGroupToAfter(size_t whichGroup, size_t toAfter)
{
    if (whichGroup < 0 || whichGroup >= groups.size() || toAfter < 0 || toAfter >= groups.size() ||
        whichGroup == toAfter)
    {
        return;
    }
    if (whichGroup < toAfter)
    {
        auto og = std::move(groups[whichGroup]);
        for (int i = whichGroup; i < toAfter; ++i)
        {
            groups[i] = std::move(groups[i + 1]);
        }
        groups[toAfter] = std::move(og);
    }
    else
    {
        // so move whichgroup=4 to after toAfter=1
        // 1 2 3 4 becomes 1 4 2 3
        // so grab 4 (1 2 3 x)
        // 3-> 4, 2-> 3 (1 x 2 3)
        // position
        auto og = std::move(groups[whichGroup]);
        for (int i = whichGroup - 1; i > toAfter; --i)
        {
            groups[i + 1] = std::move(groups[i]);
        }
        groups[toAfter + 1] = std::move(og);
    }
}

void Part::swapGroups(size_t gA, size_t gB)
{
    if (gA < 0 || gA >= groups.size() || gB < 0 || gB >= groups.size() || gA == gB)
    {
        return;
    }

    std::swap(groups[gA], groups[gB]);
}

void Part::setupOnUnstream(Engine &e)
{
    for (int idx = 0; idx < maxEffectsPerPart; ++idx)
    {
        partEffects[idx] = createEffect(partEffectStorage[idx].type, &e, &partEffectStorage[idx]);
        if (partEffects[idx])
        {
            partEffects[idx]->init(false);
            sendBusEffectInfoToClient(e, idx);
        }
    }
    rebuildGroupChannelMask();
    guaranteeKeyswitchLatchCoherence(e);
}

void Part::guaranteeKeyswitchLatchCoherence(Engine &e)
{
    int kslCount{0};
    int muteCount{0};
    for (auto &g : groups)
    {
        if (g->triggerConditions.containsKeySwitchLatch)
        {
            SCLOG_IF(groupTrigggers, "Group has trigger " << g->id.to_string());
            kslCount++;
        }
        if (g->outputInfo.mutedByLatch)
        {
            SCLOG_IF(groupTrigggers, "Group is Muted " << g->id.to_string());
            muteCount++;
        }
    }
    if (kslCount == 0 && muteCount == 0)
        return;

    if ((kslCount == 0 && muteCount != 0) || (kslCount == 1 && muteCount != 0))
    {
        // Either there's no switches, or theres just one so it is on
        SCLOG_IF(debug, "Adjusting mutes in zero or 1 ksl case");
        for (auto &g : groups)
        {
            g->outputInfo.mutedByLatch = false;
        }
    }
    else if (muteCount != kslCount - 1)
    {
        SCLOG_IF(groupTrigggers,
                 "Key Switches not set up properly : " << SCD(kslCount) << SCD(muteCount));
        if (muteCount > kslCount - 1)
        {
            // Too maky groups are muted. Unmute some
            for (auto &g : groups)
            {
                if (g->triggerConditions.containsKeySwitchLatch && g->outputInfo.mutedByLatch)
                {
                    SCLOG_IF(groupTrigggers, "Stream Un-Muting " << g->id.to_string()
                                                                 << SCD(kslCount)
                                                                 << SCD(muteCount));
                    g->outputInfo.mutedByLatch = false;
                    muteCount--;
                    if (muteCount == kslCount - 1)
                        break;
                }
            }
        }
        else
        {
            // Not enough groups are muted. Mute from back forwards
            for (auto &g : groups | std::views::reverse)
            {
                if (g->triggerConditions.containsKeySwitchLatch && !g->outputInfo.mutedByLatch)
                {
                    SCLOG_IF(groupTrigggers, "Stream Muting " << g->id.to_string() << SCD(kslCount)
                                                              << SCD(muteCount));
                    g->outputInfo.mutedByLatch = true;
                    muteCount++;
                    if (muteCount == kslCount - 1)
                        break;
                }
            }
        }
    }
}

void Part::setBusEffectType(Engine &e, int idx, AvailableBusEffects t)
{
    assert(idx >= 0 && idx < maxEffectsPerPart);
    partEffects[idx] = createEffect(t, &e, &partEffectStorage[idx]);
    if (partEffects[idx])
        partEffects[idx]->init(true);
}

void Part::sendBusEffectInfoToClient(const Engine &e, int slot)
{
    std::array<datamodel::pmd, BusEffectStorage::maxBusEffectParams> pmds;

    int saz{0};
    if (partEffects[slot])
    {
        for (int i = 0; i < partEffects[slot]->numParams(); ++i)
        {
            pmds[i] = partEffects[slot]->paramAt(i);
        }
        saz = partEffects[slot]->numParams();
    }
    for (int i = saz; i < BusEffectStorage::maxBusEffectParams; ++i)
        pmds[i].type = sst::basic_blocks::params::ParamMetaData::NONE;

    messaging::client::serializationSendToClient(
        messaging::client::s2c_bus_effect_full_data,
        messaging::client::busEffectFullData_t{
            (int)-1, partNumber, slot, {pmds, partEffectStorage[slot]}},
        *(e.getMessageController()));
}

void Part::rebuildGroupChannelMask()
{
    std::fill(groupChannelMask.begin(), groupChannelMask.end(), false);
    if (hasFeature::hasGroupMIDIChannel)
    {
        for (const auto &g : groups)
        {
            if (g->outputInfo.midiChannel >= 0)
                groupChannelMask[g->outputInfo.midiChannel] = true;
        }
    }
}

int Part::getChannelBasedTransposition(int16_t channel) const

{
    if (configuration.channel == PartConfiguration::chPerOctaveChannel)
    {
        float shift = 0;
        if (channel > 7)
        {
            shift = channel - 16;
        }
        else
        {
            shift = channel;
        }
        auto ri = parentPatch->parentEngine->midikeyRetuner.getRepetitionInterval();
        return shift * ri;
    }
    return 0;
}

} // namespace scxt::engine