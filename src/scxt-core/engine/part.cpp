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
#include <bit>
#include <limits>
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
#include "messaging/client/detail/client_serial_impl.h"
#include "messaging/client/mixer_messages.h"
#include "json/engine_traits.h"

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

    // active groups which all route elsewhere leave defOut unwritten, and the silence
    // check below reads it either way
    if (!defaultAssigned)
        memset(defOut, 0, sizeof(defOut));
    if (!noGroups)
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
    g->warmup();

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

    // A fresh group inherits whether the part has a limit, so parent its polyphony group now.
    g->updatePolyphonyGroupParent(*parentPatch->parentEngine);

    groups.push_back(std::move(g));
    return groups.size();
}

size_t Part::addGroup(std::unique_ptr<Group> &g)
{
    g->parentPart = this;
    g->setSampleRate(getSampleRate());
    g->warmup();
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
    groupTriggerInstrumentState.resetRoundRobin();
}

void Part::guaranteeKeyswitchLatchCoherence(Engine &e)
{
    /*
     * Any number of groups can share a switch key, so the live articulation is "every group
     * latched to one particular key" rather than "one group". Settle on which key that is -
     * whatever is already live if anything is, otherwise the first switch we find - and bring
     * up exactly the groups on it. That leaves a shared-key pair both sounding, and never
     * leaves an instrument with keyswitches and nothing selected.
     */
    int16_t selectedKey{-1};
    for (auto &g : groups)
    {
        auto k = g->triggerConditions.firstKeySwitchLatchKey();
        if (k < 0)
            continue;
        if (selectedKey < 0)
            selectedKey = k;
        if (!g->mutedByLatch)
        {
            selectedKey = k;
            break;
        }
    }

    if (selectedKey < 0)
    {
        // No keyswitches at all, so nothing may claim to be latched off
        for (auto &g : groups)
            g->mutedByLatch = false;
        return;
    }

    for (auto &g : groups)
    {
        auto k = g->triggerConditions.firstKeySwitchLatchKey();
        g->mutedByLatch = (k >= 0 && k != selectedKey);
        SCLOG_IF(groupTrigggers, "Coherence " << g->id.to_string() << SCD(k) << SCD(selectedKey)
                                              << SCD(g->mutedByLatch));
    }
}

roundRobinMask_t Part::roundRobinSetsForNote(const Engine &e, int16_t channel, int16_t key,
                                             int16_t midiKey, int16_t velocity,
                                             int16_t keyTranspose)
{
    roundRobinMask_t res{};
    auto prex = respondsToMIDIChannelExcludingGroupMask(channel);
    bool any{false};

    for (const auto &g : groups)
    {
        const auto &tc = g->triggerConditions;
        if (!tc.inRoundRobin())
            continue;

        auto kind = roundRobinKindIndex(tc.roundRobinKind);
        auto bit = 1u << tc.roundRobinSet;
        if (res[kind] & bit) // a note landing in two groups of a set still only spends one slot
            continue;

        if (g->mutedByLatch)
            continue;

        if (hasFeature::hasGroupMIDIChannel)
        {
            if (!g->respondsToChannelOrUsesPartChannel(channel, prex))
                continue;
        }

        // A note the keyswitch has already ruled out shouldn't burn a slot on its way past
        if (!tc.groupShouldPlayIgnoringRoundRobin(e, *g, channel, midiKey))
            continue;

        for (const auto &z : *g)
        {
            if (z->mapping.keyboardRange.includes(key + keyTranspose) &&
                z->mapping.velocityRange.includes(velocity))
            {
                res[kind] |= bit;
                any = true;
                break;
            }
        }
    }

    if (!any)
        return {};

    /*
     * A keyswitch latch press is consumed by the switch and sounds nothing, so it must not
     * advance anything. Only worth the scan once we know a round robin is actually in play.
     */
    for (const auto &g : groups)
    {
        if (g->triggerConditions.isKeySwitchLatchKey(midiKey))
            return {};
    }

    return res;
}

void Part::advanceRoundRobinSets(Engine &e, const roundRobinMask_t &setMask)
{
    for (int k = 0; k < numRoundRobinKinds; ++k)
    {
        for (int s = 0; s < maxRoundRobinSets; ++s)
        {
            if (!(setMask[k] & (1u << s)))
                continue;

            // Members of one set always agree on the kind, since the kind is half of the set's name
            auto isMember = [k, s](const auto &g) {
                const auto &tc = g->triggerConditions;
                return tc.inRoundRobin() && tc.roundRobinSet == s &&
                       roundRobinKindIndex(tc.roundRobinKind) == k;
            };

            int memberCount{0};
            for (const auto &g : groups)
                memberCount += isMember(g) ? 1 : 0;
            if (memberCount == 0)
                continue;

            auto &st = groupTriggerInstrumentState.roundRobin[k][s];

            if (k == roundRobinKindIndex(GroupTriggerID::ROUND_ROBIN_CYCLE))
            {
                /*
                 * The sequence is the ordinals actually in use, ascending - an unassigned number
                 * is not a silent step. So: the smallest ordinal above the live one, or the
                 * smallest of all when there is none. Groups sharing an ordinal share a step, and
                 * an ordinal that just got edited away hands the next press to the one above it.
                 */
                constexpr auto none{std::numeric_limits<int32_t>::max()};
                int32_t lowest{none}, next{none};
                for (const auto &g : groups)
                {
                    if (!isMember(g))
                        continue;
                    auto o = (int32_t)g->triggerConditions.roundRobinOrdinal;
                    lowest = std::min(lowest, o);
                    if (o > st.ordinal)
                        next = std::min(next, o);
                }
                st.ordinal = (next == none ? lowest : next);
            }
            else
            {
                // RANDOM and SHUFFLE pick one member group; slots are member index in group order
                int slot{0};
                if (k == roundRobinKindIndex(GroupTriggerID::ROUND_ROBIN_SHUFFLE))
                {
                    if (memberCount > maxRoundRobinGroupsPerSet)
                    {
                        SCLOG_IF(warnings, "Round robin shuffle set "
                                               << s << " has more than "
                                               << maxRoundRobinGroupsPerSet
                                               << " groups; shuffling the first ones only");
                    }
                    auto bagCount = std::min(memberCount, (int)maxRoundRobinGroupsPerSet);
                    uint32_t all = (bagCount >= 32 ? ~0u : ((1u << bagCount) - 1));
                    auto avail = all & ~st.drawn;
                    if (!avail)
                    {
                        // Every member has had its turn, so start a fresh pass
                        st.drawn = 0;
                        avail = all;
                    }
                    auto nth = (int)(e.rng.unifU32() % (uint32_t)std::popcount(avail));
                    for (int i = 0; i < bagCount; ++i)
                    {
                        if (!(avail & (1u << i)))
                            continue;
                        if (nth == 0)
                        {
                            slot = i;
                            break;
                        }
                        nth--;
                    }
                    st.drawn |= (1u << slot);
                }
                else
                {
                    slot = (int)(e.rng.unifU32() % (uint32_t)memberCount);
                }

                int i{0};
                for (const auto &g : groups)
                {
                    if (!isMember(g))
                        continue;
                    if (i == slot)
                    {
                        st.winner = g->id;
                        break;
                    }
                    i++;
                }
            }

            SCLOG_IF(groupTrigggers, "Round robin kind " << k << " set " << s
                                                         << " advanced to ordinal " << st.ordinal
                                                         << " winner " << st.winner.to_string());
        }
    }
}

partKeySwitchDisplay_t Part::keySwitchDisplay() const
{
    partKeySwitchDisplay_t res{};
    for (int k = 0; k < 128; ++k)
    {
        bool isSwitch{false}, isLive{false};
        for (const auto &g : groups)
        {
            if (g->triggerConditions.isKeySwitchKey(k))
                isSwitch = true;
            // A momentary switch is live only while held, which is audio thread state this
            // snapshot cannot see, so only latches report as live here.
            if (g->triggerConditions.firstKeySwitchLatchKey() == k && !g->mutedByLatch)
                isLive = true;
        }
        res[k] = (int32_t)(isSwitch ? (isLive ? KeySwitchDisplayState::ACTIVE
                                              : KeySwitchDisplayState::INACTIVE)
                                    : KeySwitchDisplayState::NOT_A_SWITCH);
    }
    return res;
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

bool Part::respondsToMIDIChannelExcludingGroupMask(int16_t channel) const
{
    if (parentPatch->parentEngine->runtimeConfig.omniFlavor != Engine::OmniFlavor::OMNI)
        return true;
    return channel < 0 || configuration.channel == PartConfiguration::omniChannel ||
           channel == configuration.channel;
}

bool Part::respondsToMIDIChannel(int16_t channel) const
{
    if (respondsToMIDIChannelExcludingGroupMask(channel))
        return true;
    return channel >= 0 && channel < (int16_t)groupChannelMask.size() && groupChannelMask[channel];
}

bool Part::isMPEVoiceChannel(int16_t channel) const
{
    return parentPatch->parentEngine->runtimeConfig.omniFlavor == Engine::OmniFlavor::MPE &&
           channel != configuration.mpeGlobalChannel;
}

int Part::getChannelBasedTransposition(int16_t channel) const
{
    if (parentPatch->parentEngine->runtimeConfig.omniFlavor != Engine::OmniFlavor::CHOCT)
        return 0;
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

} // namespace scxt::engine