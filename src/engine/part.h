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
#ifndef SCXT_SRC_ENGINE_PART_H
#define SCXT_SRC_ENGINE_PART_H

#include <memory>
#include <vector>
#include <optional>
#include <cassert>

#include "selection/selection_manager.h"
#include "utils.h"
#include "group.h"
#include "dsp/smoothers.h"

namespace scxt::engine
{
struct Patch;

struct Part : MoveableOnly<Part>, SampleRateSupport
{
    Part(int16_t c) : id(PartID::next()), channel(c) { pitchBendSmoother.setTarget(0); }

    PartID id;
    int16_t channel;
    Patch *parentPatch{nullptr};

    float output alignas(16)[maxOutputs][2][blockSize];
    void process();

    // TODO: have a channel mode like OMNI and MPE and everything
    static constexpr int16_t omniChannel{-1};

    // TODO: editable name
    std::string getName() const
    {
        return fmt::format("Part ch={}",
                           (channel == omniChannel ? "OMNI" : std::to_string(channel + 1)));
    }

    // TODO: Multiple outputs
    size_t getNumOutputs() const { return 1; }

    size_t addGroup()
    {
        auto g = std::make_unique<Group>();
        g->parentPart = this;
        groups.push_back(std::move(g));
        return groups.size();
    }

    void guaranteeGroupCount(size_t count)
    {
        while (groups.size() < count)
            addGroup();
    }

    /**
     * Utility data structures to allow rapid draws and displays of the structure in clients
     */
    typedef std::tuple<KeyboardRange, VelocityRange, std::string> zoneMappingItem_t;
    typedef std::vector<std::pair<selection::SelectionManager::ZoneAddress, zoneMappingItem_t>>
        zoneMappingSummary_t;
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
    bool isActive() { return activeGroups != 0; }
    void addActiveGroup() { activeGroups++; }
    void removeActiveGroup()
    {
        assert(activeGroups);
        activeGroups--;
    }

    std::array<dsp::Smoother, 128> midiCCSmoothers;
    dsp::Smoother pitchBendSmoother;
    void onSampleRateChanged() override
    {
        pitchBendSmoother.setSampleRate(samplerate);
        for (auto &mcc : midiCCSmoothers)
            mcc.setSampleRate(samplerate);
    }

    // TODO: A group by ID which throws an SCXTError

    typedef std::vector<std::unique_ptr<Group>> groupContainer_t;

    const groupContainer_t &getGroups() const { return groups; }
    void clearGroups() { groups.clear(); }

    groupContainer_t::iterator begin() noexcept { return groups.begin(); }
    groupContainer_t::const_iterator cbegin() const noexcept { return groups.cbegin(); }

    groupContainer_t::iterator end() noexcept { return groups.end(); }
    groupContainer_t::const_iterator cend() const noexcept { return groups.cend(); }

  private:
    groupContainer_t groups;
};
} // namespace scxt::engine

#endif