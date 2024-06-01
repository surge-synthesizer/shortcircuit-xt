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

namespace scxt::voice::modulation
{

namespace shmo = scxt::modulation::shared;

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

    for (int i = 0; i < scxt::maxProcessorFloatParams; ++i)
    {
        shmo::bindEl(m, p, fpT[i], p.floatParams[i], floatP[i], d.floatControlDescriptions[i]);
    }
}

void MatrixEndpoints::Sources::bind(scxt::voice::modulation::Matrix &m, engine::Zone &z,
                                    voice::Voice &v)
{
    for (int i = 0; i < 3; ++i)
    {
        switch (v.lfoEvaluator[i])
        {
        case Voice::CURVE:
            m.bindSourceValue(lfoSources.sources[i], v.curveLfos[i].output);
            break;
        case Voice::STEP:
            m.bindSourceValue(lfoSources.sources[i], v.stepLfos[i].output);
            break;
        case Voice::ENV:
            m.bindSourceValue(lfoSources.sources[i], v.envLfos[i].output);
            break;
        case Voice::MSEG:
            m.bindSourceValue(lfoSources.sources[i], zeroSource);
            break;
        }
    }
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

voiceMatrixMetadata_t getVoiceMatrixMetadata(engine::Zone &z)
{
    auto e = z.getEngine();

    namedTargetVector_t tg;
    namedSourceVector_t sr;
    namedCurveVector_t cr;

    auto identCmp = [](const auto &a, const auto &b) {
        const auto &ida = a.second;
        const auto &idb = b.second;
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

    registerVoiceModTarget(e, mixT, ptFn, mixFn);
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
        registerVoiceModTarget(e, curve.delayT, ptFn, curveLabel("Curve Delay"));
        registerVoiceModTarget(e, curve.attackT, ptFn, curveLabel("Curve Attack"));
        registerVoiceModTarget(e, curve.releaseT, ptFn, curveLabel("Curve Release"));
        registerVoiceModTarget(e, step.smoothT, ptFn, stepLabel("Step Smooth"));
        registerVoiceModTarget(e, env.delayT, ptFn, envLabel("Env Delay"));
        registerVoiceModTarget(e, env.attackT, ptFn, envLabel("Env Attack"));
        registerVoiceModTarget(e, env.holdT, ptFn, envLabel("Env Hold"));
        registerVoiceModTarget(e, env.decayT, ptFn, envLabel("Env Decay"));
        registerVoiceModTarget(e, env.sustainT, ptFn, envLabel("Env Sustain"));
        registerVoiceModTarget(e, env.releaseT, ptFn, envLabel("Env Release"));
    }
}

} // namespace scxt::voice::modulation
