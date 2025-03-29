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

#include "selection/selection_manager.h"
#include "utils.h"
#include "group.h"

#include "bus.h"
#include "macros.h"
#include "group_triggers.h"

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
    }
    virtual ~Part() = default;

    PartID id;
    int16_t partNumber;
    Patch *parentPatch{nullptr};

    bool respondsToMIDIChannel(int16_t channel) const
    {
        return channel < 0 || configuration.channel == PartConfiguration::omniChannel ||
               configuration.channel == PartConfiguration::mpeChannel ||
               channel == configuration.channel;
    }
    bool isMPEVoiceChannel(int16_t channel) const
    {
        return configuration.channel == PartConfiguration::mpeChannel &&
               channel != configuration.mpeGlobalChannel;
    }

    struct PartConfiguration
    {
        static constexpr int16_t omniChannel{-1};
        static constexpr int16_t mpeChannel{-2};

        int mpePitchBendRange{24};
        int mpeGlobalChannel{0};

        bool active{false};
        int16_t channel{omniChannel}; // a midi channel or a special value like omni
        bool mute{false};
        bool solo{false};

        bool polyLimitVoices{0}; // poly limit. 0 means unlimited.

        BusAddress routeTo{DEFAULT_BUS};

        static constexpr int maxDescription{2048};
        char blurb[maxDescription];
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

    std::array<float, 128> midiCCValues{}; // 0 .. 1 so the 128 taken out
    float pitchBendValue{0.f};             // -1..1 so the 8192 taken out

    void onSampleRateChanged() override
    {
        for (auto &g : groups)
            g->setSampleRate(samplerate);
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

  private:
    groupContainer_t groups;
};
} // namespace scxt::engine

SC_DESCRIBE(scxt::engine::Part::PartConfiguration,
            SC_FIELD(channel, pmd().asInt().withRange(-1, 15)););

#endif