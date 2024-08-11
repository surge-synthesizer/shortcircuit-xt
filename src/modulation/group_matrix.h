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

#ifndef SCXT_SRC_MODULATION_GROUP_MATRIX_H
#define SCXT_SRC_MODULATION_GROUP_MATRIX_H

#include <string>
#include <array>

#include "configuration.h"
#include "utils.h"

#include "sst/cpputils/constructors.h"
#include "sst/basic-blocks/mod-matrix/ModMatrix.h"

#include "datamodel/metadata.h"

#include "matrix_shared.h"
#include "mod_curves.h"

namespace scxt::engine
{
struct Group;
struct Engine;
} // namespace scxt::engine

namespace scxt::modulation
{
struct GroupMatrixConfig
{
    using SourceIdentifier = scxt::modulation::shared::SourceIdentifier;
    using TargetIdentifier = scxt::modulation::shared::TargetIdentifier;
    using RoutingExtraPayload = scxt::modulation::shared::RoutingExtraPayload;

    using CurveIdentifier = scxt::modulation::ModulationCurves::CurveIdentifier;
    static std::function<float(float)> getCurveOperator(CurveIdentifier id)
    {
        return scxt::modulation::ModulationCurves::getCurveOperator(id);
    }

    static std::unordered_set<TargetIdentifier> multiplicativeTargets;
    static void setIsMultiplicative(const TargetIdentifier &ti)
    {
        multiplicativeTargets.insert(ti);
    }
    static bool getIsMultiplicative(const TargetIdentifier &ti)
    {
        auto res = multiplicativeTargets.find(ti) != multiplicativeTargets.end();
        return res;
    }

    static constexpr bool IsFixedMatrix{true};
    static constexpr size_t FixedMatrixSize{12};

    using TI = TargetIdentifier;
    using SI = SourceIdentifier;

    static bool isTargetModMatrixDepth(const TargetIdentifier &t) { return t.gid == 'gelf'; }
    static size_t getTargetModMatrixElement(const TargetIdentifier &t)
    {
        assert(isTargetModMatrixDepth(t));
        return (size_t)t.index;
    }
};

struct GroupMatrix : sst::basic_blocks::mod_matrix::FixedMatrix<GroupMatrixConfig>
{
    bool forUIMode{false};
    std::unordered_map<GroupMatrixConfig::TargetIdentifier, datamodel::pmd> activeTargetsToPMD;
    std::unordered_map<GroupMatrixConfig::TargetIdentifier, float> activeTargetsToBaseValue;
};

struct GroupMatrixEndpoints
{
    using TG = GroupMatrixConfig::TargetIdentifier;
    using SR = GroupMatrixConfig::SourceIdentifier;

    GroupMatrixEndpoints(scxt::engine::Engine *e)
        : lfo{sst::cpputils::make_array_bind_last_index<LFOTarget, scxt::lfosPerGroup>(e)},
          processorTarget{
              sst::cpputils::make_array_bind_last_index<ProcessorTarget,
                                                        scxt::processorsPerZoneAndGroup>(e)},
          outputTarget(e),
          eg{sst::cpputils::make_array_bind_last_index<EGTarget, scxt::egPerGroup>(e)},
          selfModulation(e), sources(e)
    {
        // If there are group endpoints, we can merge them here
    }

    struct EGTarget : shared::EGTargetEndpointData<TG, 'genv'>
    {
        EGTarget(engine::Engine *e, uint32_t p) : shared::EGTargetEndpointData<TG, 'genv'>(p)
        {
            if (e)
            {
                std::string envnm = (p == 0 ? "EG1" : "EG2");
                registerGroupModTarget(e, aT, envnm, "Attack");
                registerGroupModTarget(e, hT, envnm, "Hold");
                registerGroupModTarget(e, dT, envnm, "Decay");
                registerGroupModTarget(e, sT, envnm, "Sustain");
                registerGroupModTarget(e, rT, envnm, "Release");
                registerGroupModTarget(e, asT, envnm, "Attack Shape");
                registerGroupModTarget(e, dsT, envnm, "Decay Shape");
                registerGroupModTarget(e, rsT, envnm, "Release Shape");
            }
        }
        void bind(GroupMatrix &m, engine::Group &g);
    };
    std::array<EGTarget, scxt::egPerGroup> eg;

    struct LFOTarget : shared::LFOTargetEndpointData<TG, 'glfo'>
    {
        LFOTarget(engine::Engine *e, uint32_t p) : shared::LFOTargetEndpointData<TG, 'glfo'>(p)
        {
            if (e)
                SCLOG_UNIMPL("Engine Attach Group LFO");
        }
        void bind(GroupMatrix &m, engine::Group &g);
    };
    std::array<LFOTarget, scxt::lfosPerGroup> lfo;

    struct ProcessorTarget : scxt::modulation::shared::ProcessorTargetEndpointData<TG, 'gprc'>
    {
        // This is out of line since it creates a caluclation using zone
        // innards and we can't have an include cycle
        ProcessorTarget(engine::Engine *e, uint32_t p);

        void bind(GroupMatrix &m, engine::Group &z);
    };
    std::array<ProcessorTarget, scxt::processorsPerZoneAndGroup> processorTarget;

    struct OutputTarget
    {
        OutputTarget(engine::Engine *e) : panT{'gout', 'pan ', 0}, ampT{'gout', 'ampl', 0}
        {
            if (e)
            {
                registerGroupModTarget(e, panT, "Output", "Pan");
                registerGroupModTarget(e, ampT, "Output", "Amplitude");
            }
        }
        TG panT, ampT;

        const float *panP{nullptr}, *ampP{nullptr};

        void bind(GroupMatrix &m, engine::Group &z);
    } outputTarget;

    struct SelfModulationTarget
    {
        SelfModulationTarget(engine::Engine *e)
        {
            for (uint32_t i = 0; i < GroupMatrixConfig::FixedMatrixSize; ++i)
            {
                selfT[i] = TG{'gelf', 'dpth', i};
                registerGroupModTarget(e, selfT[i], "Mod Depth", "Row " + std::to_string(i + 1));
            }
        }
        std::array<TG, GroupMatrixConfig::FixedMatrixSize> selfT;
    } selfModulation;

    void bindTargetBaseValues(GroupMatrix &m, engine::Group &g);

    static void registerGroupModTarget(engine::Engine *e,
                                       const GroupMatrixConfig::TargetIdentifier &t,
                                       const std::string &path, const std::string &name)
    {
        registerGroupModTarget(
            e, t, [p = path](const auto &a, const auto &b) -> std::string { return p; },
            [n = name](const auto &a, const auto &b) -> std::string { return n; });
    }

    static void
    registerGroupModTarget(engine::Engine *e, const GroupMatrixConfig::TargetIdentifier &,
                           std::function<std::string(const engine::Group &,
                                                     const GroupMatrixConfig::TargetIdentifier &)>
                               pathFn,
                           std::function<std::string(const engine::Group &,
                                                     const GroupMatrixConfig::TargetIdentifier &)>
                               nameFn);

    static void registerGroupModSource(engine::Engine *e,
                                       const GroupMatrixConfig::SourceIdentifier &t,
                                       const std::string &path, const std::string &name)
    {
        registerGroupModSource(
            e, t, [p = path](const auto &a, const auto &b) -> std::string { return p; },
            [n = name](const auto &a, const auto &b) -> std::string { return n; });
    }

    static void
    registerGroupModSource(engine::Engine *e, const GroupMatrixConfig::SourceIdentifier &,
                           std::function<std::string(const engine::Group &,
                                                     const GroupMatrixConfig::SourceIdentifier &)>
                               pathFn,
                           std::function<std::string(const engine::Group &,
                                                     const GroupMatrixConfig::SourceIdentifier &)>
                               nameFn);

    struct Sources
    {
        Sources(engine::Engine *e)
            : lfoSources(e), egSource{{'greg', 'eg1 ', 0}, {'greg', 'eg2 ', 0}},
              transportSources(e), macroSources(e)
        {
            registerGroupModSource(e, egSource[0], "", "EG1");
            registerGroupModSource(e, egSource[1], "", "EG2");
        }
        struct LFOSources
        {
            LFOSources(engine::Engine *e)
            {
                for (uint32_t i = 0; i < lfosPerZone; ++i)
                {
                    sources[i] = SR{'grlf', 'outp', i};
                    registerGroupModSource(e, sources[i], "", "LFO " + std::to_string(i + 1));
                }
            }
            std::array<SR, scxt::lfosPerZone> sources;
        } lfoSources;

        SR egSource[2];

        struct TransportSources
        {
            TransportSources(engine::Engine *e)
            {
                auto ctr = (scxt::numTransportPhasors - 1) / 2;
                for (uint32_t i = 0; i < scxt::numTransportPhasors; ++i)
                {
                    std::string name = "Beat";
                    if (i < ctr)
                        name = std::string("Beat / ") + std::to_string(1 << (ctr - i));
                    if (i > ctr)
                        name = std::string("Beat x ") + std::to_string(1 << (i - ctr));

                    phasors[i] = SR{'gtsp', 'phsr', i};
                    registerGroupModSource(e, phasors[i], "Transport", name);
                }
            }
            SR phasors[scxt::numTransportPhasors];
        } transportSources;

        struct MacroSources
        {
            MacroSources(engine::Engine *e);

            SR macros[macrosPerPart];
        } macroSources;

        float zeroSource{0.f};

        void bind(GroupMatrix &matrix, engine::Group &group);
    } sources;
};

typedef std::pair<std::string, std::string> identifierDisplayName_t;

typedef std::pair<GroupMatrixConfig::TargetIdentifier, identifierDisplayName_t> namedTarget_t;
typedef std::vector<namedTarget_t> namedTargetVector_t;
typedef std::pair<GroupMatrixConfig::SourceIdentifier, identifierDisplayName_t> namedSource_t;
typedef std::vector<namedSource_t> namedSourceVector_t;
typedef std::pair<GroupMatrixConfig::CurveIdentifier, identifierDisplayName_t> namedCurve_t;
typedef std::vector<namedCurve_t> namedCurveVector_t;

typedef std::tuple<bool, namedSourceVector_t, namedTargetVector_t, namedCurveVector_t>
    groupMatrixMetadata_t;

groupMatrixMetadata_t getGroupMatrixMetadata(const engine::Group &z);
} // namespace scxt::modulation
#endif // SHORTCIRCUITXT_GROUP_MATRIX_H
