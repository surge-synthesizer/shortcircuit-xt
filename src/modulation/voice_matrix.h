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

#ifndef SCXT_SRC_MODULATION_VOICE_MATRIX_H
#define SCXT_SRC_MODULATION_VOICE_MATRIX_H

#include <string>
#include <array>
#include <vector>

#include "sst/cpputils/constructors.h"
#include "sst/basic-blocks/mod-matrix/ModMatrix.h"

#include "utils.h"
#include "dsp/processor/processor.h"

#include "mod_curves.h"
#include "matrix_shared.h"

namespace scxt::engine
{
struct Zone;
struct Engine;
} // namespace scxt::engine
namespace scxt::voice
{
struct Voice;
}

namespace scxt::voice::modulation
{
struct MatrixConfig
{
    using SourceIdentifier = scxt::modulation::shared::SourceIdentifier;
    using TargetIdentifier = scxt::modulation::shared::TargetIdentifier;

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

    using RoutingExtraPayload = scxt::modulation::shared::RoutingExtraPayload;

    static bool isTargetModMatrixDepth(const TargetIdentifier &t) { return t.gid == 'self'; }
    static size_t getTargetModMatrixElement(const TargetIdentifier &t)
    {
        assert(isTargetModMatrixDepth(t));
        return (size_t)t.index;
    }
};
} // namespace scxt::voice::modulation

namespace scxt::voice::modulation
{
struct Matrix : sst::basic_blocks::mod_matrix::FixedMatrix<MatrixConfig>
{
    bool forUIMode{false};
    std::unordered_map<MatrixConfig::TargetIdentifier, datamodel::pmd> activeTargetsToPMD;
    std::unordered_map<MatrixConfig::TargetIdentifier, float> activeTargetsToBaseValue;
};

struct MatrixEndpoints
{
    using TG = MatrixConfig::TargetIdentifier;
    using SR = MatrixConfig::SourceIdentifier;

    MatrixEndpoints(engine::Engine *e)
        : aeg(e, 0), eg2(e, 1),
          lfo{sst::cpputils::make_array_bind_last_index<LFOTarget, scxt::lfosPerZone>(e)},
          selfModulation(e),
          processorTarget{
              sst::cpputils::make_array_bind_last_index<ProcessorTarget,
                                                        scxt::processorsPerZoneAndGroup>(e)},
          mappingTarget(e), outputTarget(e), sources(e)
    {
    }

    // We will need to refactor this when we do group. One step at a time
    struct LFOTarget : scxt::modulation::shared::LFOTargetEndpointData<TG, 'lfo '>
    {
        LFOTarget(engine::Engine *e, uint32_t p);

        void bind(Matrix &m, engine::Zone &z);
    };
    std::array<LFOTarget, scxt::lfosPerZone> lfo;

    struct EGTarget : scxt::modulation::shared::EGTargetEndpointData<TG, 'envg'>
    {
        EGTarget(engine::Engine *e, uint32_t p)
            : scxt::modulation::shared::EGTargetEndpointData<TG, 'envg'>(p)
        {
            std::string group = (index == 0 ? "AEG" : "EG2");
            registerVoiceModTarget(e, aT, group, "Attack");
            registerVoiceModTarget(e, hT, group, "Hold");
            registerVoiceModTarget(e, dT, group, "Decay");
            registerVoiceModTarget(e, sT, group, "Sustain");
            registerVoiceModTarget(e, rT, group, "Release");
            registerVoiceModTarget(e, asT, group, "Attack Shape");
            registerVoiceModTarget(e, dsT, group, "Decay Shape");
            registerVoiceModTarget(e, rsT, group, "Release Shape");
        }

        void bind(Matrix &m, engine::Zone &z);
    } aeg, eg2;

    struct MappingTarget
    {
        MappingTarget(engine::Engine *e)
            : pitchOffsetT{'zmap', 'ptof', 0}, panT{'zmap', 'pan ', 0}, ampT{'zmap', 'ampl', 0},
              playbackRatioT{'zmap', 'pbrt', 0}
        {
            if (e)
            {
                registerVoiceModTarget(e, pitchOffsetT, "Mapping", "Pitch Offset");
                registerVoiceModTarget(e, panT, "Mapping", "Pan");
                registerVoiceModTarget(e, ampT, "Mapping", "Amplitude");
                registerVoiceModTarget(e, playbackRatioT, "Mapping", "Playback Ratio");

                MatrixConfig::setIsMultiplicative(ampT);
            }
        }
        TG pitchOffsetT, panT, ampT, playbackRatioT;

        const float *pitchOffsetP{nullptr}, *panP{nullptr}, *ampP{nullptr},
            *playbackRatioP{nullptr};

        void bind(Matrix &m, engine::Zone &z);

        float zeroBase{0.f}; // this is a base zero value for things which are not in the mod map
    } mappingTarget;

    struct OutputTarget
    {
        OutputTarget(engine::Engine *e) : panT{'zout', 'pan ', 0}, ampT{'zout', 'ampl', 0}
        {
            if (e)
            {
                registerVoiceModTarget(e, panT, "Output", "Pan");
                registerVoiceModTarget(e, ampT, "Output", "Amplitude");

                MatrixConfig::setIsMultiplicative(ampT);
            }
        }
        TG panT, ampT;

        const float *panP{nullptr}, *ampP{nullptr};

        void bind(Matrix &m, engine::Zone &z);
    } outputTarget;

    struct ProcessorTarget : scxt::modulation::shared::ProcessorTargetEndpointData<TG, 'proc'>
    {
        // This is out of line since it creates a caluclation using zone
        // innards and we can't have an include cycle
        ProcessorTarget(engine::Engine *e, uint32_t p);

        void bind(Matrix &m, engine::Zone &z);
    };
    std::array<ProcessorTarget, scxt::processorsPerZoneAndGroup> processorTarget;

    struct SelfModulationTarget
    {
        SelfModulationTarget(engine::Engine *e)
        {
            for (uint32_t i = 0; i < MatrixConfig::FixedMatrixSize; ++i)
            {
                selfT[i] = TG{'self', 'dpth', i};
                registerVoiceModTarget(e, selfT[i], "Mod Depth", "Row " + std::to_string(i + 1));
            }
        }
        std::array<TG, MatrixConfig::FixedMatrixSize> selfT;
    } selfModulation;

    struct Sources
    {
        Sources(engine::Engine *e)
            : lfoSources(e), midiSources(e), aegSource{'zneg', 'aeg ', 0},
              eg2Source{'zneg', 'eg2 ', 0}, transportSources(e)
        {
            registerVoiceModSource(e, aegSource, "", "AEG");
            registerVoiceModSource(e, eg2Source, "", "EG2");
        }
        struct LFOSources
        {
            LFOSources(engine::Engine *e)
            {
                for (uint32_t i = 0; i < lfosPerZone; ++i)
                {
                    sources[i] = SR{'znlf', 'outp', i};
                    registerVoiceModSource(e, sources[i], "", "LFO " + std::to_string(i + 1));
                }
            }
            std::array<SR, scxt::lfosPerZone> sources;
        } lfoSources;

        struct MIDISources
        {
            MIDISources(engine::Engine *e)
                : modWheelSource{'zmid', 'modw'}, velocitySource{'zmid', 'velo'}
            {
                registerVoiceModSource(e, modWheelSource, "MIDI", "Mod Wheel");
                registerVoiceModSource(e, velocitySource, "MIDI", "Velocity");
            }
            SR modWheelSource, velocitySource;
        } midiSources;

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

                    phasors[i] = SR{'ztsp', 'phsr', i};
                    registerVoiceModSource(e, phasors[i], "Transport", name);

                    voicePhasors[i] = SR{'ztsp', 'vphr', i};
                    registerVoiceModSource(e, voicePhasors[i], "Transport", "Voice " + name);
                }
            }
            SR phasors[scxt::numTransportPhasors];
            SR voicePhasors[scxt::numTransportPhasors];
        } transportSources;

        SR aegSource, eg2Source;

        void bind(Matrix &m, engine::Zone &z, voice::Voice &v);

        float zeroSource{0.f};
    } sources;

    void bindTargetBaseValues(Matrix &m, engine::Zone &z);

    static void registerVoiceModTarget(engine::Engine *e, const MatrixConfig::TargetIdentifier &t,
                                       const std::string &path, const std::string &name)
    {
        registerVoiceModTarget(
            e, t, [p = path](const auto &a, const auto &b) -> std::string { return p; },
            [n = name](const auto &a, const auto &b) -> std::string { return n; });
    }

    static void registerVoiceModTarget(
        engine::Engine *e, const MatrixConfig::TargetIdentifier &,
        std::function<std::string(const engine::Zone &, const MatrixConfig::TargetIdentifier &)>
            pathFn,
        std::function<std::string(const engine::Zone &, const MatrixConfig::TargetIdentifier &)>
            nameFn);

    static void registerVoiceModSource(engine::Engine *e, const MatrixConfig::SourceIdentifier &t,
                                       const std::string &path, const std::string &name)
    {
        registerVoiceModSource(
            e, t, [p = path](const auto &a, const auto &b) -> std::string { return p; },
            [n = name](const auto &a, const auto &b) -> std::string { return n; });
    }

    static void registerVoiceModSource(
        engine::Engine *e, const MatrixConfig::SourceIdentifier &,
        std::function<std::string(const engine::Zone &, const MatrixConfig::SourceIdentifier &)>
            pathFn,
        std::function<std::string(const engine::Zone &, const MatrixConfig::SourceIdentifier &)>
            nameFn);
};

/*
 * Metadata functions return a path (or empty for top level) and name.
 * These are bad implementations for now
 */
typedef std::pair<std::string, std::string> identifierDisplayName_t;

typedef std::pair<MatrixConfig::TargetIdentifier, identifierDisplayName_t> namedTarget_t;
typedef std::vector<namedTarget_t> namedTargetVector_t;
typedef std::pair<MatrixConfig::SourceIdentifier, identifierDisplayName_t> namedSource_t;
typedef std::vector<namedSource_t> namedSourceVector_t;
typedef std::pair<MatrixConfig::CurveIdentifier, identifierDisplayName_t> namedCurve_t;
typedef std::vector<namedCurve_t> namedCurveVector_t;

typedef std::tuple<bool, namedSourceVector_t, namedTargetVector_t, namedCurveVector_t>
    voiceMatrixMetadata_t;

voiceMatrixMetadata_t getVoiceMatrixMetadata(engine::Zone &z);
} // namespace scxt::voice::modulation

#endif // __SCXT_VOICE_MATRIX_H
