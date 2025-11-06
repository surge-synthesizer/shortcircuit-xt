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

#ifndef SCXT_SRC_SCXT_CORE_MODULATION_GROUP_MATRIX_H
#define SCXT_SRC_SCXT_CORE_MODULATION_GROUP_MATRIX_H

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

    static bool supportsLag(const SourceIdentifier &s) { return true; }

    static constexpr bool IsFixedMatrix{true};
    static constexpr bool ProvidesNonZeroTargetBases{true};
    static constexpr size_t FixedMatrixSize{scxt::modMatrixRowsPerGroup};
    static constexpr bool providesTargetRanges{true};

    using TI = TargetIdentifier;
    using SI = SourceIdentifier;

    static bool isTargetModMatrixDepth(const TargetIdentifier &t) { return t.gid == 'gelf'; }
    static size_t getTargetModMatrixElement(const TargetIdentifier &t)
    {
        assert(isTargetModMatrixDepth(t));
        return (size_t)t.index;
    }

    static std::unordered_map<SourceIdentifier, int32_t> defaultLags;
    static void setDefaultLagFor(const SourceIdentifier &s, int32_t v) { defaultLags[s] = v; }
    static int32_t defaultLagFor(const SourceIdentifier &s)
    {
        auto r = defaultLags.find(s);
        if (r != defaultLags.end())
        {
            return r->second;
        }
        return 0;
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
          egTarget{sst::cpputils::make_array_bind_last_index<EGTarget, scxt::egsPerGroup>(e)},
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
                registerGroupModTarget(e, dlyT, envnm, "Delay");
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
    std::array<EGTarget, scxt::egsPerGroup> egTarget;

    struct LFOTarget : shared::LFOTargetEndpointData<TG, 'glfo'>
    {
        LFOTarget(engine::Engine *e, uint32_t p);
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
                registerGroupModTarget(e, ampT, "Output", "Amplitude", true);
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
                                       const std::string &path, const std::string &name,
                                       bool supportsMul = false)
    {
        registerGroupModTarget(
            e, t, [p = path](const auto &a, const auto &b) -> std::string { return p; },
            [n = name](const auto &a, const auto &b) -> std::string { return n; },
            [s = supportsMul](const auto &a, const auto &b) -> bool { return s; });
    }

    static void
    registerGroupModTarget(engine::Engine *e, const GroupMatrixConfig::TargetIdentifier &t,
                           std::function<std::string(const engine::Group &,
                                                     const GroupMatrixConfig::TargetIdentifier &)>
                               pathFn,
                           std::function<std::string(const engine::Group &,
                                                     const GroupMatrixConfig::TargetIdentifier &)>
                               nameFn,
                           bool supportsMul = false)
    {
        registerGroupModTarget(
            e, t, pathFn, nameFn,
            [s = supportsMul](const auto &a, const auto &b) -> bool { return s; });
    }

    static void registerGroupModTarget(
        engine::Engine *e, const GroupMatrixConfig::TargetIdentifier &,
        std::function<std::string(const engine::Group &,
                                  const GroupMatrixConfig::TargetIdentifier &)>
            pathFn,
        std::function<std::string(const engine::Group &,
                                  const GroupMatrixConfig::TargetIdentifier &)>
            nameFn,
        std::function<bool(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
            additiveFn);

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
            : lfoSources(e, "GLFO", "GLFO"), midiCCSources(e),
              egSource{{'greg', 'eg1 ', 0}, {'greg', 'eg2 ', 0}}, transportSources(e),
              rngSources(e), macroSources(e), subordinateVoiceSources(e), midiSources(e)
        {
            registerGroupModSource(e, egSource[0], "EG", "EG1");
            registerGroupModSource(e, egSource[1], "EG", "EG2");
        }

        scxt::modulation::shared::LFOSourceBase<SR, 'grlf', lfosPerGroup, registerGroupModSource>
            lfoSources;
        scxt::modulation::shared::MIDICCBase<GroupMatrixConfig, SR, 'gncc', registerGroupModSource>
            midiCCSources;

        struct MIDISources
        {
            MIDISources(engine::Engine *e)
                : modWheelSource{'gmid', 'modw'}, chanATSource{'gmid', 'chat'},
                  pbpm1Source{'gmid', 'pb11'}
            {
                registerGroupModSource(e, modWheelSource, "MIDI", "Mod Wheel");
                GroupMatrixConfig::setDefaultLagFor(modWheelSource, 25);
                registerGroupModSource(e, chanATSource, "MIDI", "Channel AT");
                registerGroupModSource(e, pbpm1Source, "MIDI", "Pitch Bend");
                GroupMatrixConfig::setDefaultLagFor(pbpm1Source, 10);
            }
            SR modWheelSource, chanATSource, pbpm1Source;
        } midiSources;

        SR egSource[2];
        scxt::modulation::shared::TransportSourceBase<SR, 'gtsp', registerGroupModSource>
            transportSources;
        scxt::modulation::shared::RNGSourceBase<SR, 'grng', registerGroupModSource> rngSources;

        struct MacroSources
        {
            MacroSources(engine::Engine *e);

            SR macros[macrosPerPart];
        } macroSources;

        struct SubordinateVoiceSources
        {
            SubordinateVoiceSources(engine::Engine *e);

            SR anyVoiceGated, anyVoiceSounding, voiceCount, gatedVoiceCount;
        } subordinateVoiceSources;

        float zeroSource{0.f};

        void bind(GroupMatrix &matrix, engine::Group &group);
    } sources;
};

typedef std::pair<std::string, std::string> identifierDisplayName_t;

// The last bool is "allows multiplicative"
typedef std::tuple<GroupMatrixConfig::TargetIdentifier, identifierDisplayName_t, bool, bool>
    namedTarget_t;
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
