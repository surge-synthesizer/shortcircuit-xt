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

#include <stdexcept>

#include "sst/plugininfra/strnatcmp.h"
#include "group_matrix.h"
#include "engine/group.h"
#include "engine/engine.h"

namespace scxt::modulation
{

std::unordered_map<GroupMatrixConfig::SourceIdentifier, int32_t> GroupMatrixConfig::defaultLags;

namespace shmo = scxt::modulation::shared;

GroupMatrixEndpoints::ProcessorTarget::ProcessorTarget(engine::Engine *e, uint32_t p)
    : scxt::modulation::shared::ProcessorTargetEndpointData<TG, 'gprc'>(p)
{
    if (e)
    {
        auto ptFn = [](const engine::Group &z,
                       const GroupMatrixConfig::TargetIdentifier &t) -> std::string {
            auto &d = z.processorDescription[t.index];
            if (d.type == dsp::processor::proct_none)
                return "";
            return std::string("P") + std::to_string(t.index + 1) + " " + d.typeDisplayName;
        };

        auto mixFn = [](const engine::Group &z,
                        const GroupMatrixConfig::TargetIdentifier &t) -> std::string {
            auto &d = z.processorDescription[t.index];
            if (d.type == dsp::processor::proct_none)
                return "";
            return "Mix";
        };

        auto levFn = [](const engine::Group &z,
                        const GroupMatrixConfig::TargetIdentifier &t) -> std::string {
            auto &d = z.processorDescription[t.index];
            if (d.type == dsp::processor::proct_none)
                return "";
            return "Output Level";
        };

        registerGroupModTarget(e, mixT, ptFn, mixFn);
        registerGroupModTarget(e, outputLevelDbT, ptFn, levFn, true);

        for (int i = 0; i < scxt::maxProcessorFloatParams; ++i)
        {
            auto elFn = [icopy = i](const engine::Group &z,
                                    const GroupMatrixConfig::TargetIdentifier &t) -> std::string {
                auto &d = z.processorDescription[t.index];
                if (d.type == dsp::processor::proct_none)
                    return "";
                return d.floatControlDescriptions[icopy].name;
            };

            auto adFn = [icopy = i](const engine::Group &z,
                                    const GroupMatrixConfig::TargetIdentifier &t) -> int32_t {
                auto &d = z.processorDescription[t.index];
                if (d.type == dsp::processor::proct_none)
                    return false;
                auto can = d.floatControlDescriptions[icopy].hasSupportsMultiplicativeModulation();
                if (!can)
                    return false;
                auto should =
                    !d.floatControlDescriptions[icopy].hasMultiplicativeModulationOffByDefault();
                return can * 1 + should * 2;
            };
            registerGroupModTarget(e, fpT[i], ptFn, elFn, adFn);
        }
    }
}

void GroupMatrixEndpoints::bindTargetBaseValues(scxt::modulation::GroupMatrix &m, engine::Group &g)
{
    outputTarget.bind(m, g);
    for (auto &p : processorTarget)
        p.bind(m, g);
    for (auto &l : lfo)
        l.bind(m, g);
    for (auto &e : egTarget)
        e.bind(m, g);
}

void GroupMatrixEndpoints::EGTarget::bind(scxt::modulation::GroupMatrix &m, engine::Group &g)
{
    baseBind(m, g.gegStorage[index]);
}

void GroupMatrixEndpoints::ProcessorTarget::bind(scxt::modulation::GroupMatrix &m, engine::Group &g)
{
    auto &p = g.processorStorage[index];
    auto &d = g.processorDescription[index];
    shmo::bindEl(m, p, mixT, p.mix, mixP);
    shmo::bindEl(m, p, outputLevelDbT, p.outputCubAmp, outputLevelDbP);

    for (int i = 0; i < scxt::maxProcessorFloatParams; ++i)
    {
        shmo::bindEl(m, p, fpT[i], p.floatParams[i], floatP[i], d.floatControlDescriptions[i]);
    }
}

GroupMatrixEndpoints::LFOTarget::LFOTarget(engine::Engine *e, uint32_t p)
    : shared::LFOTargetEndpointData<TG, 'glfo'>(p)
{
    if (e)
    {
        auto ptFn = [](const engine::Group &g, const auto &t) -> std::string {
            return "GLFO " + std::to_string(t.index + 1);
        };

        auto conditionLabel =
            [](const std::string &lab,
               std::function<bool(const scxt::modulation::ModulatorStorage &)> op) {
                return [o = op, l = lab](const engine::Group &g, const auto &t) -> std::string {
                    auto &ms = g.modulatorStorage[t.index];
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

        registerGroupModTarget(e, rateT, ptFn, notEnvLabel(nm(ms.rate)));
        registerGroupModTarget(e, amplitudeT, ptFn, allLabel(nm(ms.amplitude)), true);
        registerGroupModTarget(e, retriggerT, ptFn, allLabel("Retrigger"));
        registerGroupModTarget(e, curve.deformT, ptFn, curveLabel(nm(ms.curveLfoStorage.deform)));
        registerGroupModTarget(e, curve.angleT, ptFn, curveLabel(nm(ms.curveLfoStorage.angle)));
        registerGroupModTarget(e, curve.delayT, ptFn, curveLabel(nm(ms.curveLfoStorage.delay)));
        registerGroupModTarget(e, curve.attackT, ptFn, curveLabel(nm(ms.curveLfoStorage.attack)));
        registerGroupModTarget(e, curve.releaseT, ptFn, curveLabel(nm(ms.curveLfoStorage.release)));
        registerGroupModTarget(e, step.smoothT, ptFn, stepLabel(nm(ms.stepLfoStorage.smooth)));
        registerGroupModTarget(e, env.delayT, ptFn, envLabel(nm(ms.envLfoStorage.delay)));
        registerGroupModTarget(e, env.attackT, ptFn, envLabel(nm(ms.envLfoStorage.attack)));
        registerGroupModTarget(e, env.holdT, ptFn, envLabel(nm(ms.envLfoStorage.hold)));
        registerGroupModTarget(e, env.decayT, ptFn, envLabel(nm(ms.envLfoStorage.decay)));
        registerGroupModTarget(e, env.sustainT, ptFn, envLabel(nm(ms.envLfoStorage.sustain)));
        registerGroupModTarget(e, env.releaseT, ptFn, envLabel(nm(ms.envLfoStorage.release)));
        registerGroupModTarget(e, env.aShapeT, ptFn, envLabel(nm(ms.envLfoStorage.aShape)));
        registerGroupModTarget(e, env.dShapeT, ptFn, envLabel(nm(ms.envLfoStorage.dShape)));
        registerGroupModTarget(e, env.rShapeT, ptFn, envLabel(nm(ms.envLfoStorage.rShape)));
        registerGroupModTarget(e, env.rateMulT, ptFn, envLabel(nm(ms.envLfoStorage.rateMul)));
    }
}

void GroupMatrixEndpoints::LFOTarget::bind(scxt::modulation::GroupMatrix &m, engine::Group &g)
{
    baseBind(m, g);
}

void GroupMatrixEndpoints::OutputTarget::bind(scxt::modulation::GroupMatrix &m, engine::Group &g)
{
    shmo::bindEl(m, g.outputInfo, ampT, g.outputInfo.amplitude, ampP);
    shmo::bindEl(m, g.outputInfo, panT, g.outputInfo.pan, panP);
}

GroupMatrixEndpoints::Sources::Sources(engine::Engine *e)
    : lfoSources(e, "GLFO", "GLFO"), midiCCSources(e),
      egSource{{'greg', 'eg1 ', 0}, {'greg', 'eg2 ', 0}}, transportSources(e), rngSources(e),
      macroSources(e), subordinateVoiceSources(e), midiSources(e), keyAndPitchSources(e)
{
    registerGroupModSource(e, egSource[0], "EG", "EG1");
    registerGroupModSource(e, egSource[1], "EG", "EG2");

    for (int i = 0; i < randomsPerGroupOrZone; ++i)
        registerGroupModSource(
            e, rngSources.randoms[i], [](auto &a, auto &b) { return "Random"; },
            [i](auto &a, auto &b) {
                return "Rnd " + std::string(1, (char)('A' + i)) + " " +
                       a.miscSourceStorage.randomDisplayName(i);
            });
}

void GroupMatrixEndpoints::Sources::bind(scxt::modulation::GroupMatrix &m, engine::Group &g)
{
    lfoSources.bind(m, g, zeroSource);
    midiCCSources.bind(m, *(g.parentPart));
    keyAndPitchSources.bind(m, g);

    int idx{0};
    auto *part = g.parentPart;
    for (int i = 0; i < macrosPerPart; ++i)
    {
        m.bindSourceValue(macroSources.macros[i], part->macros[i].value);
    }

    m.bindSourceValue(egSource[0], g.eg[0].outBlock0);
    m.bindSourceValue(egSource[1], g.eg[1].outBlock0);

    for (int i = 0; i < scxt::randomsPerGroupOrZone; ++i)
    {
        m.bindSourceValue(rngSources.randoms[i], g.randomEvaluator.outputs[i]);
    }

    for (int i = 0; i < scxt::phasorsPerGroupOrZone; ++i)
    {
        m.bindSourceValue(transportSources.phasors[i], g.phasorEvaluator.outputs[i]);
    }

    m.bindSourceValue(midiSources.modWheelSource, g.parentPart->midiCCValues[1]);
    m.bindSourceValue(midiSources.chanATSource, g.parentPart->channelAT);
    m.bindSourceValue(midiSources.pbpm1Source, g.parentPart->pitchBendValue);

    m.bindSourceValue(subordinateVoiceSources.anyVoiceGated, g.fAnyGated);
    m.bindSourceValue(subordinateVoiceSources.anyVoiceSounding, g.fAnySounding);
    m.bindSourceValue(subordinateVoiceSources.voiceCount, g.fVoiceCount);
    m.bindSourceValue(subordinateVoiceSources.gatedVoiceCount, g.fGatedCount);
}

void GroupMatrixEndpoints::Sources::KeyAndPitchSources::bind(GroupMatrix &m, engine::Group &g)
{
    m.bindSourceValue(lowPitch, g.pitchTrack.low);
    m.bindSourceValue(highPitch, g.pitchTrack.high);
    m.bindSourceValue(lastPitch, g.pitchTrack.last);
    m.bindSourceValue(lowKey, g.keyTrack.low);
    m.bindSourceValue(highKey, g.keyTrack.high);
    m.bindSourceValue(lastKey, g.keyTrack.last);

    m.bindSourceValue(voiceCount, g.voiceCount);
}

void GroupMatrixEndpoints::registerGroupModTarget(
    engine::Engine *e, const GroupMatrixConfig::TargetIdentifier &t,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
        pathFn,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
        nameFn,
    std::function<int32_t(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
        additiveFn)
{
    if (!e)
        return;

    e->registerGroupModTarget(t, pathFn, nameFn, additiveFn);
}

void GroupMatrixEndpoints::registerGroupModSource(
    engine::Engine *e, const GroupMatrixConfig::SourceIdentifier &t,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::SourceIdentifier &)>
        pathFn,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::SourceIdentifier &)>
        nameFn)
{
    if (!e)
        return;

    e->registerGroupModSource(t, pathFn, nameFn);
}

groupMatrixMetadata_t getGroupMatrixMetadata(const engine::Group &g)
{
    // somewhat dissatisfied with amount of copying here also
    auto e = g.getEngine();

    namedTargetVector_t tg;
    namedSourceVector_t sr;
    namedCurveVector_t cr;

    auto identCmp = [](const auto &a, const auto &b) {
        const auto &srca = std::get<0>(a);
        const auto &srcb = std::get<0>(b);
        const auto &ida = std::get<1>(a);
        const auto &idb = std::get<1>(b);

        if (srca.gid == srcb.gid && (srca.gid == 'gmac' || srca.gid == 'gelf'))
        {
            // Renaming makes macros a special case. Rows are just obvious
            return srca.index < srcb.index;
        }

        if (ida.first == idb.first)
        {
            if (ida.second == idb.second)
            {
                return std::hash<
                           std::remove_cv_t<std::remove_reference_t<decltype(std::get<0>(a))>>>{}(
                           std::get<0>(a)) <
                       std::hash<
                           std::remove_cv_t<std::remove_reference_t<decltype(std::get<0>(b))>>>{}(
                           std::get<0>(b));
            }
            else
            {
                return strnatcasecmp(ida.second.c_str(), idb.second.c_str()) < 0;
            }
        }
        return strnatcasecmp(ida.first.c_str(), idb.first.c_str()) < 0;
    };

    auto tgtCmp = [identCmp](const auto &a, const auto &b) {
        const auto &ta = std::get<0>(a);
        const auto &tb = std::get<0>(b);
        if (ta.gid == 'gprc' && tb.gid == ta.gid && tb.index == ta.index)
        {
            return ta.tid < tb.tid;
        }
        return identCmp(a, b);
    };

    for (const auto &[t, fns] : e->groupModTargets)
    {
        tg.emplace_back(t, identifierDisplayName_t{std::get<0>(fns)(g, t), std::get<1>(fns)(g, t)},
                        std::get<2>(fns)(g, t),
                        std::get<3>(fns)(g, t)); // GroupMatrixConfig::getIsMultiplicative(t));
    }
    std::sort(tg.begin(), tg.end(), tgtCmp);

    for (const auto &[s, fns] : e->groupModSources)
    {
        sr.emplace_back(s, identifierDisplayName_t{fns.first(g, s), fns.second(g, s)});
    }
    std::sort(sr.begin(), sr.end(), identCmp);

    for (const auto &c : scxt::modulation::ModulationCurves::allCurves)
    {
        auto n = scxt::modulation::ModulationCurves::curveNames.find(c);
        assert(n != scxt::modulation::ModulationCurves::curveNames.end());
        cr.emplace_back(c, identifierDisplayName_t{n->second.first, n->second.second});
    }

    return groupMatrixMetadata_t{true, sr, tg, cr};
}
GroupMatrixEndpoints::Sources::MacroSources::MacroSources(engine::Engine *e)
{
    for (auto i = 0U; i < macrosPerPart; ++i)
    {
        macros[i] = SR{'gmac', 'mcro', i};
        registerGroupModSource(
            e, macros[i], [](auto &a, auto &b) { return "Macro"; },
            [i](auto &grp, auto &s) { return grp.parentPart->macros[i].name; });
        GroupMatrixConfig::setDefaultLagFor(macros[i], 25);
    }
}

GroupMatrixEndpoints::Sources::SubordinateVoiceSources::SubordinateVoiceSources(engine::Engine *e)
    : anyVoiceGated{'gvoc', 'avgt', 0}, anyVoiceSounding{'gvoc', 'avsd', 0},
      voiceCount{'gvoc', 'vcnt', 0}, gatedVoiceCount{'gvoc', 'vgct', 0}
{
    registerGroupModSource(
        e, anyVoiceGated, [](auto &, auto &) { return "Voices"; },
        [](auto &, auto &) { return "Any Gated"; });
    registerGroupModSource(
        e, anyVoiceSounding, [](auto &, auto &) { return "Voices"; },
        [](auto &, auto &) { return "Any Sounding"; });
    registerGroupModSource(
        e, voiceCount, [](auto &, auto &) { return "Voices"; },
        [](auto &, auto &) { return "Voices"; });
    registerGroupModSource(
        e, gatedVoiceCount, [](auto &, auto &) { return "Voices"; },
        [](auto &, auto &) { return "Gated Voices"; });
}

} // namespace scxt::modulation
