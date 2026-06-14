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
    std::unordered_map<GroupMatrixConfig::TargetIdentifier, datamodel::pmd::FeatureState>
        activeTargetsToFeatureState;
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
                // Long path/name go in the menu; short path/name in the matrix row
                std::string longPath = "Group EG " + std::to_string(p + 1);
                std::string shortPath = "GEG" + std::to_string(p + 1);
                auto reg = [&](const auto &t, const std::string &nm) {
                    registerGroupModTarget(e, t, longPath, nm, false, shortPath, nm);
                };
                reg(dlyT, "Delay");
                reg(aT, "Attack");
                reg(hT, "Hold");
                reg(dT, "Decay");
                reg(sT, "Sustain");
                reg(rT, "Release");
                reg(asT, "Attack Shape");
                reg(dsT, "Decay Shape");
                reg(rsT, "Release Shape");
                reg(retriggerT, "Retrigger");
                reg(rateMulT, "Rate Multiplier");
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
        // This is out of line since it creates a calculation using zone
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
                                       bool supportsMul = false, const std::string &shortPath = "",
                                       const std::string &shortName = "")
    {
        auto pathFn = [p = path](const auto &a, const auto &b) -> std::string { return p; };
        auto nameFn = [n = name](const auto &a, const auto &b) -> std::string { return n; };
        std::function<std::string(const engine::Group &,
                                  const GroupMatrixConfig::TargetIdentifier &)>
            shortPathFn{};
        std::function<std::string(const engine::Group &,
                                  const GroupMatrixConfig::TargetIdentifier &)>
            shortNameFn{};
        if (!shortPath.empty())
            shortPathFn = [s = shortPath](const auto &a, const auto &b) -> std::string {
                return s;
            };
        if (!shortName.empty())
            shortNameFn = [s = shortName](const auto &a, const auto &b) -> std::string {
                return s;
            };
        registerGroupModTarget(
            e, t, pathFn, nameFn,
            [s = supportsMul](const auto &a, const auto &b) -> int32_t { return s; }, shortPathFn,
            shortNameFn);
    }

    static void
    registerGroupModTarget(engine::Engine *e, const GroupMatrixConfig::TargetIdentifier &t,
                           std::function<std::string(const engine::Group &,
                                                     const GroupMatrixConfig::TargetIdentifier &)>
                               pathFn,
                           std::function<std::string(const engine::Group &,
                                                     const GroupMatrixConfig::TargetIdentifier &)>
                               nameFn,
                           bool supportsMul = false,
                           std::function<std::string(const engine::Group &,
                                                     const GroupMatrixConfig::TargetIdentifier &)>
                               shortPathFn = {},
                           std::function<std::string(const engine::Group &,
                                                     const GroupMatrixConfig::TargetIdentifier &)>
                               shortNameFn = {})
    {
        registerGroupModTarget(
            e, t, pathFn, nameFn,
            [s = supportsMul](const auto &a, const auto &b) -> int32_t { return s; }, shortPathFn,
            shortNameFn);
    }

    static void registerGroupModTarget(
        engine::Engine *e, const GroupMatrixConfig::TargetIdentifier &,
        std::function<std::string(const engine::Group &,
                                  const GroupMatrixConfig::TargetIdentifier &)>
            pathFn,
        std::function<std::string(const engine::Group &,
                                  const GroupMatrixConfig::TargetIdentifier &)>
            nameFn,
        std::function<int32_t(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
            additiveFn,
        std::function<std::string(const engine::Group &,
                                  const GroupMatrixConfig::TargetIdentifier &)>
            shortPathFn = {},
        std::function<std::string(const engine::Group &,
                                  const GroupMatrixConfig::TargetIdentifier &)>
            shortNameFn = {});

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
        Sources(engine::Engine *e);

        scxt::modulation::shared::LFOSourceBase<SR, 'grlf', lfosPerGroup, registerGroupModSource>
            lfoSources;
        scxt::modulation::shared::MIDICCBase<GroupMatrixConfig, SR, 'gncc', registerGroupModSource>
            midiCCSources;

        struct KeyAndPitchSources
        {
            KeyAndPitchSources(engine::Engine *e)
                : lowPitch{'gkap', 'lpit'}, highPitch{'gkap', 'hpit'}, lastPitch{'gkap', 'apit'},
                  lowKey{'gkap', 'lkey'}, highKey{'gkap', 'hkey'}, lastKey{'gkap', 'akey'},
                  lowMidiKey{'gkap', 'lmky'}, highMidiKey{'gkap', 'hmky'},
                  lastMidiKey{'gkap', 'amky'}, voiceCount{'gkap', 'vcnt'}
            {
                registerGroupModSource(e, lowPitch, "KeyTracking", "Low Key+PB+Glide");
                registerGroupModSource(e, highPitch, "KeyTracking", "High Key+PB+Glide");
                registerGroupModSource(e, lastPitch, "KeyTracking", "Last Key+PB+Glide");
                registerGroupModSource(e, lowKey, "KeyTracking", "Low Key");
                registerGroupModSource(e, highKey, "KeyTracking", "High Key");
                registerGroupModSource(e, lastKey, "KeyTracking", "Last Key");
                // registerGroupModSource(e, lowMidiKey, "KeyTracking", "Low MIDI Key");
                // registerGroupModSource(e, highMidiKey, "KeyTracking", "High MIDI Key");
                // registerGroupModSource(e, lastMidiKey, "KeyTracking", "Last MIDI Key");
                registerGroupModSource(e, voiceCount, "KeyTracking", "Voice Count");
            }
            SR lowPitch, highPitch, lastPitch, lowKey, highKey, lastKey;
            SR lowMidiKey, highMidiKey, lastMidiKey, voiceCount;

            void bind(GroupMatrix &m, engine::Group &g);

            static bool isKeyAndPitchSource(const SR &sr) { return sr.gid == 'gkap'; }
        } keyAndPitchSources;

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
        scxt::modulation::shared::RNGSourceBase<SR, 'grng'> rngSources;
        scxt::modulation::shared::EnvFollowerSourceBase<SR, 'gef', registerGroupModSource>
            envFollowerSources;

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

// path, name, shortPath, shortName for matrix targets. Short variants are used in compact
// matrix-row display; long variants appear in the popup menu.
typedef std::tuple<std::string, std::string, std::string, std::string> targetDisplayName_t;
inline const std::string &displayPath(const targetDisplayName_t &d) { return std::get<0>(d); }
inline const std::string &displayName(const targetDisplayName_t &d) { return std::get<1>(d); }
inline const std::string &displayShortPath(const targetDisplayName_t &d) { return std::get<2>(d); }
inline const std::string &displayShortName(const targetDisplayName_t &d) { return std::get<3>(d); }

// The last two are "multiplicative" and "enabled"
// "multiplicative" uses first bit as can and second bit as should
typedef std::tuple<GroupMatrixConfig::TargetIdentifier, targetDisplayName_t, int32_t, bool>
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
