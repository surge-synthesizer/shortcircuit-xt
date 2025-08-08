/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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
#ifndef SCXT_SRC_ENGINE_PART_H
#define SCXT_SRC_ENGINE_PART_H

#include <memory>
#include <vector>
#include <optional>
#include <cassert>

#include "sst/basic-blocks/dsp/LagCollection.h"

#include "selection/selection_manager.h"
#include "utils.h"
#include "group.h"

#include "bus.h"
#include "macros.h"
#include "group_triggers.h"

#include "bus_effect.h"

namespace scxt::engine
{
struct Patch;
struct Engine;

struct Part : MoveableOnly<Part>, SampleRateSupport
{
    Part(int16_t c) : id(PartID::next()), partNumber(c)
    {
        int idx{0};
        for (auto &m : macros)
        {
            m.index = idx;
            m.part = partNumber;
            m.name = Macro::defaultNameFor(idx);
            idx++;
        }
        configuration.channel = c;
        if (c == 0)
        {
            configuration.active = true;
        }

        snprintf(configuration.name, sizeof(configuration.name), "Part %d", partNumber + 1);
        std::fill(groupChannelMask.begin(), groupChannelMask.end(), false);
    }
    virtual ~Part() = default;

    PartID id;
    int16_t partNumber;
    Patch *parentPatch{nullptr};

    std::array<bool, 16> groupChannelMask{};

    bool respondsToMIDIChannelExcludingGroupMask(int16_t channel) const
    {
        return channel < 0 || configuration.channel == PartConfiguration::omniChannel ||
               configuration.channel == PartConfiguration::mpeChannel ||
               channel == configuration.channel;
    }

    bool respondsToMIDIChannel(int16_t channel) const
    {
        return respondsToMIDIChannelExcludingGroupMask(channel) || groupChannelMask[channel];
    }
    bool isMPEVoiceChannel(int16_t channel) const
    {
        return configuration.channel == PartConfiguration::mpeChannel &&
               channel != configuration.mpeGlobalChannel;
    }
    void rebuildGroupChannelMask()
    {
        std::fill(groupChannelMask.begin(), groupChannelMask.end(), false);
        for (const auto &g : groups)
        {
            if (g->outputInfo.midiChannel >= 0)
                groupChannelMask[g->outputInfo.midiChannel] = true;
        }
    }

    struct PartConfiguration
    {
        static constexpr size_t maxName{256};
        static constexpr int16_t omniChannel{-1};
        static constexpr int16_t mpeChannel{-2};

        char name[maxName]{0};
        int mpePitchBendRange{24};
        int mpeGlobalChannel{0};

        bool active{false};
        int16_t channel{omniChannel}; // a midi channel or a special value like omni
        bool mute{false};
        bool solo{false};
        bool muteDueToSolo{false}; // this is unstreamed and calcs in onPartConfigurationUpdated

        int32_t polyLimitVoices{0}; // poly limit. 0 means unlimited.

        BusAddress routeTo{DEFAULT_BUS};

        float level{1.f};
        float pan{0.f};
        int32_t transpose{0};
        float tuning{0.f};

        // This needs to be a standard object, and windows msvc doesn't like
        // std::strings in those objects, so use a char*
        static constexpr int maxDescription{2048};
        char blurb[maxDescription]{0};
    } configuration;
    void process(Engine &onto);

    // TODO: editable name
    std::string getName() const
    {
        return fmt::format("Part ch={}", (configuration.channel == PartConfiguration::omniChannel
                                              ? "OMNI"
                                              : std::to_string(configuration.channel + 1)));
    }

    // TODO: Multiple outputs
    size_t getNumOutputs() const { return 1; }

    size_t addGroup()
    {
        auto g = std::make_unique<Group>();

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

    void guaranteeGroupCount(size_t count)
    {
        while (groups.size() < count)
            addGroup();
    }

    /**
     *The state of this part for group trigger caches and so on
     */
    GroupTriggerInstrumentState groupTriggerInstrumentState;

    /**
     * Utility data structures to allow rapid draws and displays of the structure in clients
     */
    struct ZoneMappingItem
    {
        selection::SelectionManager::ZoneAddress address;
        KeyboardRange kr;
        VelocityRange vr;
        std::string name;
        int32_t features{0};
    };
    using zoneMappingItem_t = ZoneMappingItem; // legacy name from when we were a tuple
    typedef std::vector<ZoneMappingItem> zoneMappingSummary_t;
    zoneMappingSummary_t getZoneMappingSummary();

    // TODO GroupID -> index
    // TODO: Remove Group by both - Copy from group basically

    const std::unique_ptr<Group> &getGroup(size_t i) const
    {
        if (!(i < groups.size()))
        {
            printStackTrace();
        }
        assert(i < groups.size());
        return groups[i];
    }

    uint32_t activeGroups{0};
    bool isActive() { return activeGroups != 0 && configuration.active; }
    void addActiveGroup() { activeGroups++; }
    void removeActiveGroup()
    {
        assert(activeGroups);
        activeGroups--;
    }
    void moveGroupToAfter(size_t whichGroup, size_t toAfter);

    std::array<float, 128> midiCCValues{}; // 0 .. 1 so the 128 taken out
    float channelAT{0.f};
    float pitchBendValue{0.f}; // -1..1 so the 8192 taken out

    struct MacroUILagHandler : sst::basic_blocks::dsp::UIComponentLagHandlerBase<MacroUILagHandler>
    {
        Part &part;
        int macro{-1};
        MacroUILagHandler(Part &p) : part(p) {}

        void setTargetOnMacro(int index, float value)
        {
            macro = index;
            setTarget(value);
        }

        void updateDestination(float val)
        {
            if (macro >= 0 && macro < scxt::macrosPerPart)
            {
                part.macros[macro].setValueConstrained(val);
            }
        }
    } macroLagHandler{*this};

    enum LagIndexes
    {
        pitchBend,
        numLags
    };
    sst::basic_blocks::dsp::LagCollection<numLags> externalSignalLag;

    void onSampleRateChanged() override
    {
        for (auto &g : groups)
            g->setSampleRate(samplerate);
        externalSignalLag.setRateInMilliseconds(1000.0 * 3 * blockSize / 48000, samplerate,
                                                blockSizeInv);
        externalSignalLag.snapAllActiveToTarget();
        macroLagHandler.setRate(120, scxt::blockSize, samplerate);
    }

    std::array<Macro, macrosPerPart> macros;

    // TODO: A group by ID which throws an SCXTError
    typedef std::vector<std::unique_ptr<Group>> groupContainer_t;

    const groupContainer_t &getGroups() const { return groups; }
    void clearGroups() { groups.clear(); }
    int getGroupIndex(const GroupID &zid) const
    {
        for (const auto &[idx, r] : sst::cpputils::enumerate(groups))
            if (r->id == zid)
                return idx;
        return -1;
    }
    std::unique_ptr<Group> removeGroup(const GroupID &zid)
    {
        auto idx = getGroupIndex(zid);
        if (idx < 0 || idx >= (int)groups.size())
            return {};

        auto res = std::move(groups[idx]);
        groups.erase(groups.begin() + idx);
        res->parentPart = nullptr;
        return res;
    }
    groupContainer_t::iterator begin() noexcept { return groups.begin(); }
    groupContainer_t::const_iterator cbegin() const noexcept { return groups.cbegin(); }

    groupContainer_t::iterator end() noexcept { return groups.end(); }
    groupContainer_t::const_iterator cend() const noexcept { return groups.cend(); }

    std::vector<SampleID> getSamplesUsedByPart() const;

    std::array<BusEffectStorage, maxEffectsPerPart> partEffectStorage{};
    std::array<std::unique_ptr<BusEffect>, maxEffectsPerPart> partEffects{};
    void setBusEffectType(Engine &e, int idx, AvailableBusEffects t);
    void initializeAfterUnstream(Engine &e);
    void sendAllBusEffectInfoToClient(const Engine &e)
    {
        for (int i = 0; i < maxEffectsPerPart; ++i)
            sendBusEffectInfoToClient(e, i);
    }
    void sendBusEffectInfoToClient(const Engine &, int slot);

    void prepareToStream();

  private:
    groupContainer_t groups;
};
} // namespace scxt::engine

SC_DESCRIBE(scxt::engine::Part::PartConfiguration,
            SC_FIELD(channel, pmd().asInt().withRange(-1, 15));
            SC_FIELD(level, pmd().asCubicDecibelAttenuation().withName("Level"));
            SC_FIELD(pan, pmd().asPercentBipolar().withName("Pan"));
            SC_FIELD(tuning, pmd()
                                 .asFloat()
                                 .withRange(-36, 36)
                                 .withLinearScaleFormatting("semitones")
                                 .withName("Tuning"));
            SC_FIELD(transpose,
                     pmd().asInt().withRange(-36, 36).withLinearScaleFormatting("keys").withName(
                         "Transpose")););

#endif