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

    static bool supportsLag(const SourceIdentifier &s) { return true; }

    static constexpr bool IsFixedMatrix{true};
    static constexpr size_t FixedMatrixSize{12};
    static constexpr bool providesTargetRanges{true};

    using TI = TargetIdentifier;
    using SI = SourceIdentifier;

    using RoutingExtraPayload = scxt::modulation::shared::RoutingExtraPayload;

    static bool isTargetModMatrixDepth(const TargetIdentifier &t) { return t.gid == 'self'; }
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
        : egTarget{sst::cpputils::make_array_bind_last_index<EGTarget, scxt::egsPerZone>(e)},
          lfo{sst::cpputils::make_array_bind_last_index<LFOTarget, scxt::lfosPerZone>(e)},
          selfModulation(e),
          processorTarget{
              sst::cpputils::make_array_bind_last_index<ProcessorTarget,
                                                        scxt::processorsPerZoneAndGroup>(e)},
          mappingTarget(e), outputTarget(e), sources(e)
    {
    }

    static void registerVoiceModSource(engine::Engine *e, const MatrixConfig::SourceIdentifier &t,
                                       const std::string &path, const std::string &name)
    {
        registerVoiceModSource(
            e, t, [p = path](const auto &a, const auto &b) -> std::string { return p; },
            [n = name](const auto &a, const auto &b) -> std::string { return n; });
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
            std::string group = (index == 0 ? "AEG" : std::string("EG") + std::to_string(p + 1));
            registerVoiceModTarget(e, dlyT, group, "Delay");
            registerVoiceModTarget(e, aT, group, "Attack");
            registerVoiceModTarget(e, hT, group, "Hold");
            registerVoiceModTarget(e, dT, group, "Decay");
            registerVoiceModTarget(e, sT, group, "Sustain");
            registerVoiceModTarget(e, rT, group, "Release");
            registerVoiceModTarget(e, asT, group, "Attack Shape");
            registerVoiceModTarget(e, dsT, group, "Decay Shape");
            registerVoiceModTarget(e, rsT, group, "Release Shape");
            registerVoiceModTarget(e, retriggerT, group, "Retrigger");
        }

        void bind(Matrix &m, engine::Zone &z);
    };
    std::array<EGTarget, scxt::egsPerZone> egTarget;

    struct MappingTarget
    {
        // so we can reach them from external multisamples
        static constexpr TG pitchOffsetA{'zmap', 'ptof', 0};
        MappingTarget(engine::Engine *e)
            : pitchOffsetT(pitchOffsetA), panT{'zmap', 'pan ', 0}, ampT{'zmap', 'ampl', 0},
              playbackRatioT{'zmap', 'pbrt', 0}
        {
            if (e)
            {
                registerVoiceModTarget(e, pitchOffsetT, "Mapping", "Pitch Offset");
                registerVoiceModTarget(e, panT, "Mapping", "Pan");
                registerVoiceModTarget(e, ampT, "Mapping", "Amplitude", true);
                registerVoiceModTarget(e, playbackRatioT, "Mapping", "Playback Ratio");
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
                registerVoiceModTarget(e, ampT, "Output", "Amplitude", true);
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
        static_assert(scxt::egsPerZone == 5, "Clean up the egSources constructor if this fails");
        // so we can reach them from external multisamples
        static constexpr SR eg2A{'zneg', 'eg2 ', 0};
        Sources(engine::Engine *e)
            : lfoSources(e), midiCCSources(e), midiSources(e), noteExpressions(e),
              egSources{{{'zneg', 'aeg ', 0},
                         eg2A,
                         {'zneg', 'eg3 ', 0},
                         {'zneg', 'eg4 ', 0},
                         {'zneg', 'eg5 ', 0}}},
              transportSources(e), rngSources(e), macroSources(e), mpeSources(e), voiceSources(e)
        {
            registerVoiceModSource(e, egSources[0], "", "AEG");
            for (int i = 1; i < egsPerZone; ++i)
                registerVoiceModSource(e, egSources[i], "", "EG" + std::to_string(i + 1));
        }

        LFOSourceBase<SR, 'znlf', lfosPerZone, registerVoiceModSource> lfoSources;
        MIDICCBase<MatrixConfig, SR, 'zncc', registerVoiceModSource> midiCCSources;

        struct MIDISources
        {
            MIDISources(engine::Engine *e)
                : modWheelSource{'zmid', 'modw'}, velocitySource{'zmid', 'velo'},
                  releaseVelocitySource{'zmid', 'rvel'}, keytrackSource{'zmid', 'ktrk'}
            {
                registerVoiceModSource(e, modWheelSource, "MIDI", "Mod Wheel");
                MatrixConfig::setDefaultLagFor(modWheelSource, 25);
                registerVoiceModSource(e, velocitySource, "MIDI", "Velocity");
                registerVoiceModSource(e, releaseVelocitySource, "MIDI", "Release Vel");
                registerVoiceModSource(e, keytrackSource, "MIDI", "KeyTrack");
                registerVoiceModSource(e, polyATSource, "MIDI", "Poly AT");
                MatrixConfig::setDefaultLagFor(polyATSource, 100);
            }
            SR modWheelSource, velocitySource, releaseVelocitySource, keytrackSource, polyATSource;
        } midiSources;

        struct MPESources
        {
            MPESources(engine::Engine *e)
                : mpeBend{'zmpe', 'bend'}, mpeTimbre{'zmpe', 'timb'}, mpePressure{'zmpe', 'pres'}
            {
                registerVoiceModSource(e, mpeBend, "MPE", "Voice Pitch Bend");
                registerVoiceModSource(e, mpeTimbre, "MPE", "Timbre");
                MatrixConfig::setDefaultLagFor(mpeTimbre, 50);
                registerVoiceModSource(e, mpePressure, "MPE", "Pressure");
                MatrixConfig::setDefaultLagFor(mpePressure, 50);
            }
            SR mpeBend, mpeTimbre, mpePressure;
        } mpeSources;

        struct NoteExpressionSources
        {
            NoteExpressionSources(engine::Engine *e)
                : volume{'znte', 'volu'}, pan{'znte', 'pan '}, tuning{'znte', 'tuni'},
                  vibrato{'znte', 'vibr'}, expression{'znte', 'expr'}, brightness{'znte', 'brit'},
                  pressure{'znte', 'pres'}
            {
                registerVoiceModSource(e, volume, "Note Expressions", "Volume");
                registerVoiceModSource(e, pan, "Note Expressions", "Pan");
                registerVoiceModSource(e, tuning, "Note Expressions", "Tuning");
                registerVoiceModSource(e, vibrato, "Note Expressions", "Vibrato");
                registerVoiceModSource(e, expression, "Note Expressions", "Expression");
                registerVoiceModSource(e, brightness, "Note Expressions", "Brightness");
                registerVoiceModSource(e, pressure, "Note Expressions", "Pressure");

                MatrixConfig::setDefaultLagFor(volume, 10);
                MatrixConfig::setDefaultLagFor(pan, 10);
                MatrixConfig::setDefaultLagFor(tuning, 10);
                MatrixConfig::setDefaultLagFor(vibrato, 10);
                MatrixConfig::setDefaultLagFor(expression, 10);
                MatrixConfig::setDefaultLagFor(brightness, 10);
                MatrixConfig::setDefaultLagFor(pressure, 10);
            }
            SR volume, pan, tuning, vibrato, expression, brightness, pressure;
        } noteExpressions;

        struct VoiceSources
        {
            VoiceSources(engine::Engine *e)
                : isGated{'zvsr', 'gate'}, isReleased{'zvsr', 'reld'},
                  variantCount{'zvsr', 'vcnt', 0}, variantCountFraction{'zvsr', 'vcfr', 0},
                  loopPercentage{'zvsr', 'lppc', 0}, isLooping{'zvsr', 'islp', 0},
                  samplePercentage{'zvsr', 'sppc', 0}
            {
                registerVoiceModSource(e, isGated, "Voice", "Is Gated");
                registerVoiceModSource(e, isReleased, "Voice", "Is Released");
                registerVoiceModSource(e, variantCount, "Voice", "Variant Idx");
                registerVoiceModSource(e, variantCountFraction, "Voice", "Variant %");
                registerVoiceModSource(e, isLooping, "Voice", "Is Looping");
                registerVoiceModSource(e, samplePercentage, "Voice", "Sample %");
                registerVoiceModSource(e, loopPercentage, "Voice", "Loop %");
            }
            SR isGated, isReleased;
            SR variantCount, variantCountFraction;
            SR isLooping, loopPercentage, samplePercentage;
        } voiceSources;

        TransportSourceBase<SR, 'ztsp', true, registerVoiceModSource> transportSources;

        struct RNGSources
        {
            RNGSources(engine::Engine *e)
            {
                for (uint32_t i = 0; i < 8; ++i)
                {
                    std::string name = "";
                    name = (i % 4 > 1) ? "Unipolar " : "Bipolar ";
                    name += (i < 4) ? "Even " : "Gaussian ";
                    name += std::to_string(i % 2 + 1);

                    randoms[i] = SR{'zrng', 'rnds', i};
                    registerVoiceModSource(e, randoms[i], "Random", name);
                }
            }
            SR randoms[8];
        } rngSources;

        struct MacroSources
        {
            MacroSources(engine::Engine *e);

            SR macros[macrosPerPart];
        } macroSources;

        std::array<SR, egsPerZone> egSources;

        void bind(Matrix &m, engine::Zone &z, voice::Voice &v);

        float zeroSource{0.f};
    } sources;

    void bindTargetBaseValues(Matrix &m, engine::Zone &z);

    static void registerVoiceModTarget(engine::Engine *e, const MatrixConfig::TargetIdentifier &t,
                                       const std::string &path, const std::string &name,
                                       bool supportsMultiplicative = false)
    {
        registerVoiceModTarget(
            e, t, [p = path](const auto &a, const auto &b) -> std::string { return p; },
            [n = name](const auto &a, const auto &b) -> std::string { return n; },
            [s = supportsMultiplicative](const auto &a, const auto &b) -> bool { return s; });
    }

    static void registerVoiceModTarget(
        engine::Engine *e, const MatrixConfig::TargetIdentifier &t,
        std::function<std::string(const engine::Zone &, const MatrixConfig::TargetIdentifier &)>
            pathFn,
        std::function<std::string(const engine::Zone &, const MatrixConfig::TargetIdentifier &)>
            nameFn,
        bool supportsMultiplicative = false)
    {
        registerVoiceModTarget(
            e, t, pathFn, nameFn,
            [s = supportsMultiplicative](const auto &a, const auto &b) { return s; });
    }

    static void registerVoiceModTarget(
        engine::Engine *e, const MatrixConfig::TargetIdentifier &,
        std::function<std::string(const engine::Zone &, const MatrixConfig::TargetIdentifier &)>
            pathFn,
        std::function<std::string(const engine::Zone &, const MatrixConfig::TargetIdentifier &)>
            nameFn,
        std::function<bool(const engine::Zone &, const MatrixConfig::TargetIdentifier &)>
            additiveFn,
        std::function<bool(const engine::Zone &, const MatrixConfig::TargetIdentifier &)>
            enabledFn =
                [](const engine::Zone &, const MatrixConfig::TargetIdentifier &) { return true; });

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

// The last bool is "allows multiplicative"
typedef std::tuple<MatrixConfig::TargetIdentifier, identifierDisplayName_t, bool, bool>
    namedTarget_t;
typedef std::vector<namedTarget_t> namedTargetVector_t;
typedef std::pair<MatrixConfig::SourceIdentifier, identifierDisplayName_t> namedSource_t;
typedef std::vector<namedSource_t> namedSourceVector_t;
typedef std::pair<MatrixConfig::CurveIdentifier, identifierDisplayName_t> namedCurve_t;
typedef std::vector<namedCurve_t> namedCurveVector_t;

typedef std::tuple<bool, namedSourceVector_t, namedTargetVector_t, namedCurveVector_t>
    voiceMatrixMetadata_t;

voiceMatrixMetadata_t getVoiceMatrixMetadata(const engine::Zone &z);
} // namespace scxt::voice::modulation

#endif // __SCXT_VOICE_MATRIX_H
