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

#include <optional>

#include "datamodel/metadata.h"
#include "voice_matrix.h"
#include "engine/zone.h"
#include "voice/voice.h"
#include "sst/basic-blocks/dsp/RNG.h"

#include "modulation/modulator_storage.h"

namespace scxt::voice::modulation
{

std::unordered_set<MatrixConfig::TargetIdentifier> MatrixConfig::multiplicativeTargets;

namespace shmo = scxt::modulation::shared;
sst::basic_blocks::dsp::RNG rng;

void MatrixEndpoints::bindTargetBaseValues(scxt::voice::modulation::Matrix &m, engine::Zone &z)
{
    for (auto &l : lfo)
        l.bind(m, z);
    aeg.bind(m, z);
    eg2.bind(m, z);

    mappingTarget.bind(m, z);
    outputTarget.bind(m, z);

    for (auto &p : processorTarget)
        p.bind(m, z);
}
void MatrixEndpoints::LFOTarget::bind(scxt::voice::modulation::Matrix &m, engine::Zone &z)
{
    baseBind(m, z);
}

void MatrixEndpoints::EGTarget::bind(scxt::voice::modulation::Matrix &m, engine::Zone &z)
{
    baseBind(m, z.egStorage[index]);
}

void MatrixEndpoints::MappingTarget::bind(scxt::voice::modulation::Matrix &m, engine::Zone &z)
{
    auto &mt = z.mapping;

    shmo::bindEl(m, mt, pitchOffsetT, mt.pitchOffset, pitchOffsetP);
    shmo::bindEl(m, mt, ampT, mt.amplitude, ampP);
    shmo::bindEl(m, mt, panT, mt.pan, panP);
    // This is a true oddity. We can modulate but not specify it
    shmo::bindEl(m, mt, playbackRatioT, zeroBase, playbackRatioP,
                 datamodel::pmd().asFloat().withRange(0, 2).withLinearScaleFormatting("x"));
}

void MatrixEndpoints::OutputTarget::bind(scxt::voice::modulation::Matrix &m, engine::Zone &z)
{
    auto &ot = z.outputInfo;
    shmo::bindEl(m, ot, panT, ot.pan, panP);
    shmo::bindEl(m, ot, ampT, ot.amplitude, ampP);
}

void MatrixEndpoints::ProcessorTarget::bind(scxt::voice::modulation::Matrix &m, engine::Zone &z)
{
    auto &p = z.processorStorage[index];
    auto &d = z.processorDescription[index];
    shmo::bindEl(m, p, mixT, p.mix, mixP);
    shmo::bindEl(m, p, outputLevelDbT, p.outputCubAmp, outputLevelDbP);

    for (int i = 0; i < scxt::maxProcessorFloatParams; ++i)
    {
        shmo::bindEl(m, p, fpT[i], p.floatParams[i], floatP[i], d.floatControlDescriptions[i]);
    }
}

float randomRoll(bool bipolar, int distribution)
{
    if (bipolar)
    {
        if (distribution == 0)
        {
            return rng.unifPM1();
        }
        else
        {
            return rng.normPM1();
        }
    }
    else
    {
        if (distribution == 0)
        {
            return rng.unif01();
        }
        else
        {
            return rng.half01();
        }
    }
}

void MatrixEndpoints::Sources::bind(scxt::voice::modulation::Matrix &m, engine::Zone &z,
                                    voice::Voice &v)
{
    lfoSources.bind(m, v, zeroSource);

    m.bindSourceValue(aegSource, v.aeg.outBlock0);
    m.bindSourceValue(eg2Source, v.eg2.outBlock0);

    m.bindSourceValue(midiSources.modWheelSource,
                      z.parentGroup->parentPart->midiCCSmoothers[1].output);
    m.bindSourceValue(midiSources.velocitySource, v.velocity);

    for (int i = 0; i < scxt::numTransportPhasors; ++i)
    {
        m.bindSourceValue(transportSources.phasors[i], z.getEngine()->transportPhasors[i]);
        m.bindSourceValue(transportSources.voicePhasors[i], v.transportPhasors[i]);
    }

    for (int i = 0; i < 8; ++i)
    {
        bool bip = (i % 4 > 1) ? false : true;
        int dist = (i < 4) ? 0 : 1;
        m.bindSourceConstantValue(rngSources.randoms[i], randomRoll(bip, dist));
    }

    auto *part = z.parentGroup->parentPart;
    for (int i = 0; i < macrosPerPart; ++i)
    {
        m.bindSourceValue(macroSources.macros[i], part->macros[i].value);
    }
}

void MatrixEndpoints::registerVoiceModTarget(
    engine::Engine *e, const MatrixConfig::TargetIdentifier &t,
    std::function<std::string(const engine::Zone &, const MatrixConfig::TargetIdentifier &)> pathFn,
    std::function<std::string(const engine::Zone &, const MatrixConfig::TargetIdentifier &)> nameFn)
{
    if (!e)
        return;

    e->registerVoiceModTarget(t, pathFn, nameFn);
}

void MatrixEndpoints::registerVoiceModSource(
    engine::Engine *e, const MatrixConfig::SourceIdentifier &t,
    std::function<std::string(const engine::Zone &, const MatrixConfig::SourceIdentifier &)> pathFn,
    std::function<std::string(const engine::Zone &, const MatrixConfig::SourceIdentifier &)> nameFn)
{
    if (!e)
        return;

    e->registerVoiceModSource(t, pathFn, nameFn);
}

voiceMatrixMetadata_t getVoiceMatrixMetadata(const engine::Zone &z)
{
    auto e = z.getEngine();

    namedTargetVector_t tg;
    namedSourceVector_t sr;
    namedCurveVector_t cr;

    auto identCmp = [](const auto &a, const auto &b) {
        const auto &srca = a.first;
        const auto &srcb = b.first;
        const auto &ida = a.second;
        const auto &idb = b.second;
        if (srca.gid == srcb.gid && (srca.gid == 'zmac' || srca.gid == 'self'))
        {
            return srca.index < srcb.index;
        }

        if (ida.first == idb.first)
        {
            if (ida.second == idb.second)
            {
                return std::hash<decltype(a.first)>{}(a.first) <
                       std::hash<decltype(b.first)>{}(b.first);
            }
            else
            {
                return ida.second < idb.second;
            }
        }
        return ida.first < idb.first;
    };

    auto tgtCmp = [identCmp](const auto &a, const auto &b) {
        const auto &ta = a.first;
        const auto &tb = b.first;
        if (ta.gid == 'proc' && tb.gid == ta.gid && tb.index == ta.index)
        {
            return ta.tid < tb.tid;
        }
        return identCmp(a, b);
    };

    for (const auto &[t, fns] : e->voiceModTargets)
    {
        tg.emplace_back(t, identifierDisplayName_t{fns.first(z, t), fns.second(z, t)});
    }
    std::sort(tg.begin(), tg.end(), tgtCmp);

    for (const auto &[s, fns] : e->voiceModSources)
    {
        sr.emplace_back(s, identifierDisplayName_t{fns.first(z, s), fns.second(z, s)});
    }
    std::sort(sr.begin(), sr.end(), identCmp);

    for (const auto &c : scxt::modulation::ModulationCurves::allCurves)
    {
        auto n = scxt::modulation::ModulationCurves::curveNames.find(c);
        assert(n != scxt::modulation::ModulationCurves::curveNames.end());
        cr.emplace_back(c, identifierDisplayName_t{n->second.first, n->second.second});
    }

    return voiceMatrixMetadata_t{true, sr, tg, cr};
}

MatrixEndpoints::ProcessorTarget::ProcessorTarget(engine::Engine *e, uint32_t p)
    : scxt::modulation::shared::ProcessorTargetEndpointData<TG, 'proc'>(p)
{
    auto ptFn = [](const engine::Zone &z, const MatrixConfig::TargetIdentifier &t) -> std::string {
        auto &d = z.processorDescription[t.index];
        if (d.type == dsp::processor::proct_none)
            return "";
        return std::string("P") + std::to_string(t.index + 1) + " " + d.typeDisplayName;
    };

    auto mixFn = [](const engine::Zone &z, const MatrixConfig::TargetIdentifier &t) -> std::string {
        auto &d = z.processorDescription[t.index];
        if (d.type == dsp::processor::proct_none)
            return "";
        return "Mix";
    };

    auto levFn = [](const engine::Zone &z, const MatrixConfig::TargetIdentifier &t) -> std::string {
        auto &d = z.processorDescription[t.index];
        if (d.type == dsp::processor::proct_none)
            return "";
        return "Output Level";
    };

    registerVoiceModTarget(e, mixT, ptFn, mixFn);
    registerVoiceModTarget(e, outputLevelDbT, ptFn, levFn);

    MatrixConfig::setIsMultiplicative(outputLevelDbT);
    for (int i = 0; i < scxt::maxProcessorFloatParams; ++i)
    {
        auto elFn = [icopy = i](const engine::Zone &z,
                                const MatrixConfig::TargetIdentifier &t) -> std::string {
            auto &d = z.processorDescription[t.index];
            if (d.type == dsp::processor::proct_none)
                return "";
            return d.floatControlDescriptions[icopy].name;
        };
        registerVoiceModTarget(e, fpT[i], ptFn, elFn);
    }
}

MatrixEndpoints::LFOTarget::LFOTarget(engine::Engine *e, uint32_t p)
    : scxt::modulation::shared::LFOTargetEndpointData<TG, 'lfo '>(p)
{
    if (e)
    {
        auto ptFn = [](const engine::Zone &z,
                       const MatrixConfig::TargetIdentifier &t) -> std::string {
            return "LFO " + std::to_string(t.index + 1);
        };

        auto conditionLabel =
            [](const std::string &lab,
               std::function<bool(const scxt::modulation::ModulatorStorage &)> op) {
                return [o = op, l = lab](const engine::Zone &z,
                                         const MatrixConfig::TargetIdentifier &t) -> std::string {
                    auto &ms = z.modulatorStorage[t.index];
                    if (o(ms))
                        return l;
                    return "";
                };
            };

        auto stepLabel = [conditionLabel](auto labl) {
            return conditionLabel(labl, [](auto &ms) { return ms.isStep(); });
        };

        auto curveLabel = [conditionLabel](auto labl) {
            return conditionLabel(labl, [](auto &ms) { return ms.isCurve(); });
        };

        auto envLabel = [conditionLabel](auto labl) {
            return conditionLabel(labl, [](auto &ms) { return ms.isEnv(); });
        };

        auto notEnvLabel = [conditionLabel](auto labl) {
            return conditionLabel(labl, [](auto &ms) { return !ms.isEnv(); });
        };

        auto allLabel = [conditionLabel](auto labl) {
            return conditionLabel(labl, [](auto &ms) { return true; });
        };

        auto ms = scxt::modulation::ModulatorStorage();
        auto nm = [&ms](auto &v) { return datamodel::describeValue(ms, v).name; };

        registerVoiceModTarget(e, rateT, ptFn, notEnvLabel(nm(ms.rate)));
        registerVoiceModTarget(e, curve.deformT, ptFn, curveLabel(nm(ms.curveLfoStorage.deform)));
        registerVoiceModTarget(e, curve.delayT, ptFn, curveLabel(nm(ms.curveLfoStorage.delay)));
        registerVoiceModTarget(e, curve.attackT, ptFn, curveLabel(nm(ms.curveLfoStorage.attack)));
        registerVoiceModTarget(e, curve.releaseT, ptFn, curveLabel(nm(ms.curveLfoStorage.release)));
        registerVoiceModTarget(e, step.smoothT, ptFn, stepLabel(nm(ms.stepLfoStorage.smooth)));
        registerVoiceModTarget(e, env.delayT, ptFn, envLabel(nm(ms.envLfoStorage.delay)));
        registerVoiceModTarget(e, env.attackT, ptFn, envLabel(nm(ms.envLfoStorage.attack)));
        registerVoiceModTarget(e, env.holdT, ptFn, envLabel(nm(ms.envLfoStorage.hold)));
        registerVoiceModTarget(e, env.decayT, ptFn, envLabel(nm(ms.envLfoStorage.decay)));
        registerVoiceModTarget(e, env.sustainT, ptFn, envLabel(nm(ms.envLfoStorage.sustain)));
        registerVoiceModTarget(e, env.releaseT, ptFn, envLabel(nm(ms.envLfoStorage.release)));
    }
}

MatrixEndpoints::Sources::MacroSources::MacroSources(engine::Engine *e)
{
    for (auto i = 0U; i < macrosPerPart; ++i)
    {
        macros[i] = SR{'zmac', 'mcro', i};
        registerVoiceModSource(
            e, macros[i], [](auto &a, auto &b) { return "Macro"; },
            [i](auto &zone, auto &s) { return zone.parentGroup->parentPart->macros[i].name; });
    }
}
} // namespace scxt::voice::modulation
