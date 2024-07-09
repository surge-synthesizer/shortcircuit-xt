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
#ifndef SCXT_SRC_ENGINE_GROUP_H
#define SCXT_SRC_ENGINE_GROUP_H

#include <infrastructure/sse_include.h>

#include <memory>
#include <vector>
#include <cassert>
#include <cmath>

#include <sst/cpputils.h>
#include <sst/basic-blocks/modulators/AHDSRShapedSC.h>
#include <sst/basic-blocks/dsp/BlockInterpolators.h>
#include <sst/filters/HalfRateFilter.h>

#include "utils.h"
#include "zone.h"
#include "selection/selection_manager.h"
#include "group_and_zone.h"
#include "bus.h"
#include "modulation/modulators/steplfo.h"
#include "modulation/group_matrix.h"
#include "modulation/has_modulators.h"

namespace scxt::engine
{
struct Part;
struct Engine;

constexpr int lfosPerGroup{scxt::lfosPerGroup};

struct Group : MoveableOnly<Group>,
               HasGroupZoneProcessors<Group>,
               modulation::shared::HasModulators<Group>,
               SampleRateSupport
{
    Group();
    virtual ~Group()
    {
        for (auto *p : processors)
        {
            if (p)
            {
                SCLOG("Group Destructor: Unspawning Processors");
                dsp::processor::unspawnProcessor(p);
            }
        }
    }
    GroupID id;

    std::string name{};
    Part *parentPart{nullptr};

    struct GroupOutputInfo
    {
        float amplitude{1.f}, pan{0.f}, velocitySensitivity{0.6f};
        bool muted{false};
        bool oversample{true};
        ProcRoutingPath procRouting{procRoute_linear};
        BusAddress routeTo{DEFAULT_BUS};
    } outputInfo;

    Engine *getEngine();

    float output alignas(16)[2][blockSize << 1];
    void attack();
    void resetLFOs(int whichLFO = -1);
    void process(Engine &onto);
    template <bool OS> void processWithOS(Engine &onto);
    bool lastOversample{true};

    void setupOnUnstream(const engine::Engine &e);

    // ToDo editable name
    std::string getName() const { return name; }

    size_t addZone(std::unique_ptr<Zone> &z)
    {
        z->parentGroup = this;
        zones.push_back(std::move(z));
        return zones.size();
    }

    size_t addZone(std::unique_ptr<Zone> &&z)
    {
        z->parentGroup = this;
        zones.push_back(std::move(z));
        return zones.size();
    }

    void clearZones() { zones.clear(); }

    int getZoneIndex(const ZoneID &zid) const
    {
        for (const auto &[idx, r] : sst::cpputils::enumerate(zones))
            if (r->id == zid)
                return idx;
        return -1;
    }

    const std::unique_ptr<Zone> &getZone(int idx) const
    {
        assert(idx >= 0 && idx < (int)zones.size());
        return zones[idx];
    }

    const std::unique_ptr<Zone> &getZone(const ZoneID &zid) const
    {
        auto idx = getZoneIndex(zid);
        if (idx < 0 || idx >= (int)zones.size())
            throw SCXTError("Unable to locate part " + zid.to_string() + " in patch " +
                            id.to_string());
        return zones[idx];
    }

    std::unique_ptr<Zone> removeZone(ZoneID &zid)
    {
        auto idx = getZoneIndex(zid);
        if (idx < 0 || idx >= (int)zones.size())
            return {};

        auto res = std::move(zones[idx]);
        zones.erase(zones.begin() + idx);
        res->parentGroup = nullptr;
        return res;
    }

    bool isActive() { return activeZones != 0; }
    void addActiveZone();
    void removeActiveZone();

    void onSampleRateChanged() override;

    sst::filters::HalfRate::HalfRateFilter osDownFilter;

    /*
     * One of the core differences between zone and group is, since group is monophonic,
     * it contains both the data for the process evaluators and also the evaluators themselves
     * whereas with zone the zone is the data, but it contains a set of voices related to that
     * data. Just something to think about as you read the code here!
     */
    using lipol = sst::basic_blocks::dsp::lipol_sse<blockSize, false>;
    using lipolOS = sst::basic_blocks::dsp::lipol_sse<blockSize << 1, false>;

    lipol outputAmp;
    lipolOS outputAmpOS;

    std::array<modulation::modulators::AdsrStorage, egPerGroup> gegStorage{};

    std::array<modulation::ModulatorStorage, lfosPerGroup> modulatorStorage;
    std::array<modulation::modulators::StepLFO, lfosPerGroup> stepLfos;
    std::array<bool, lfosPerGroup> lfoUsed{};
    bool initializedLFOs{false};

    modulation::GroupMatrix modMatrix;
    modulation::GroupMatrixEndpoints endpoints;
    modulation::GroupMatrix::RoutingTable routingTable;
    void rePrepareAndBindGroupMatrix();

    inline float envelope_rate_linear_nowrap(float f)
    {
        return blockSize * sampleRateInv * dsp::twoToTheXTable.twoToThe(-f);
    }

    std::array<dsp::processor::Processor *, engine::processorCount> processors{};
    uint8_t processorPlacementStorage alignas(
        16)[engine::processorCount][dsp::processor::processorMemoryBufferSize];
    int32_t processorIntParams alignas(
        16)[engine::processorCount][dsp::processor::maxProcessorIntParams];
    lipol processorMix[engine::processorCount];
    lipolOS processorMixOS[engine::processorCount];
    lipol processorLevel[engine::processorCount];
    lipolOS processorLevelOS[engine::processorCount];

    sst::basic_blocks::dsp::UIComponentLagHandler mUILag;

    void onProcessorTypeChanged(int w, dsp::processor::ProcessorType t);

    uint32_t activeZones{0};

    typedef std::vector<std::unique_ptr<Zone>> zoneContainer_t;

    const zoneContainer_t &getZones() const { return zones; }

    zoneContainer_t::iterator begin() noexcept { return zones.begin(); }
    zoneContainer_t::const_iterator cbegin() const noexcept { return zones.cbegin(); }

    zoneContainer_t::iterator end() noexcept { return zones.end(); }
    zoneContainer_t::const_iterator cend() const noexcept { return zones.cend(); }

  private:
    zoneContainer_t zones;
};
} // namespace scxt::engine

SC_DESCRIBE(scxt::engine::Group::GroupOutputInfo,
            SC_FIELD(amplitude, pmd().asCubicDecibelAttenuation().withName("Amplitude"));
            SC_FIELD(pan, pmd().asPercentBipolar().withName("Pan"));
            SC_FIELD(procRouting, pmd().asInt().withRange(0, 1));
            SC_FIELD(oversample, pmd().asBool().withName("Oversample"));
            SC_FIELD(velocitySensitivity,
                     pmd().asPercent().withName("Velocity Sensitivity").withDefault(0.6f));)

#endif