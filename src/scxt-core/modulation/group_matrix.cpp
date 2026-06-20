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

void GroupMatrix::warmup(engine::Engine *e)
{
    // See Matrix::warmup; same approach for the (monophonic) group matrix.
    static float warmupSink{0.f};
    for (const auto &sp : e->groupModSources)
        bindSourceValue(sp.first, warmupSink);
    for (const auto &tp : e->groupModTargets)
        bindTargetBaseValue(tp.first, warmupSink);
}

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

        auto ptShortFn = [](const engine::Group &z,
                            const GroupMatrixConfig::TargetIdentifier &t) -> std::string {
            auto &d = z.processorDescription[t.index];
            if (d.type == dsp::processor::proct_none)
                return "";
            return std::string("P") + std::to_string(t.index + 1) + "." + d.typeShortName;
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

        auto levShortFn = [](const engine::Group &z,
                             const GroupMatrixConfig::TargetIdentifier &t) -> std::string {
            auto &d = z.processorDescription[t.index];
            if (d.type == dsp::processor::proct_none)
                return "";
            return "Out Lvl";
        };

        auto order = scxt::modulation::shared::ExplicitMenuOrder(e);
        for (int i = 0; i < scxt::maxProcessorFloatParams; ++i)
        {
            auto elFn = [icopy = i](const engine::Group &z,
                                    const GroupMatrixConfig::TargetIdentifier &t) -> std::string {
                auto &d = z.processorDescription[t.index];
                if (d.type == dsp::processor::proct_none)
                    return "";
                return d.floatControlDescriptions[icopy].name;
            };
            auto elShortFn = [icopy =
                                  i](const engine::Group &z,
                                     const GroupMatrixConfig::TargetIdentifier &t) -> std::string {
                auto &d = z.processorDescription[t.index];
                if (d.type == dsp::processor::proct_none)
                    return "";
                return d.floatControlDescriptions[icopy].shortName;
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
            registerGroupModTarget(e, fpT[i], ptFn, elFn, adFn, ptShortFn, elShortFn);
        }

        order.separator();
        registerGroupModTarget(e, mixT, ptFn, mixFn, false, ptShortFn, mixFn);
        registerGroupModTarget(e, outputLevelDbT, ptFn, levFn, true, ptShortFn, levShortFn);
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
        // Long path goes in the menu; short path in the matrix row
        auto ptFn = [](const engine::Group &g, const auto &t) -> std::string {
            return "Group LFO " + std::to_string(t.index + 1);
        };

        auto ptShortFn = [](const engine::Group &g, const auto &t) -> std::string {
            return "GLFO" + std::to_string(t.index + 1);
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

        // Short name matches long name; only the path differs between menu and row
        auto reg = [&](const auto &t, auto nameFn, bool mul = false) {
            registerGroupModTarget(e, t, ptFn, nameFn, mul, ptShortFn, nameFn);
        };

        auto orderGuard = scxt::modulation::shared::ExplicitMenuOrder(e);
        reg(rateT, notEnvLabel(nm(ms.rate)));
        reg(amplitudeT, allLabel(nm(ms.amplitude)), true);
        reg(retriggerT, allLabel("Retrigger"));
        orderGuard.separator();
        reg(curve.deformT, curveLabel(nm(ms.curveLfoStorage.deform)));
        reg(curve.angleT, curveLabel(nm(ms.curveLfoStorage.angle)));
        reg(curve.delayT, curveLabel(nm(ms.curveLfoStorage.delay)));
        reg(curve.attackT, curveLabel(nm(ms.curveLfoStorage.attack)));
        reg(curve.releaseT, curveLabel(nm(ms.curveLfoStorage.release)));
        orderGuard.separator();
        reg(step.smoothT, stepLabel(nm(ms.stepLfoStorage.smooth)));
        orderGuard.separator();
        reg(env.delayT, envLabel(nm(ms.envLfoStorage.delay)));
        reg(env.attackT, envLabel(nm(ms.envLfoStorage.attack)));
        reg(env.holdT, envLabel(nm(ms.envLfoStorage.hold)));
        reg(env.decayT, envLabel(nm(ms.envLfoStorage.decay)));
        reg(env.sustainT, envLabel(nm(ms.envLfoStorage.sustain)));
        reg(env.releaseT, envLabel(nm(ms.envLfoStorage.release)));
        orderGuard.separator();
        reg(env.aShapeT, envLabel(nm(ms.envLfoStorage.aShape)));
        reg(env.dShapeT, envLabel(nm(ms.envLfoStorage.dShape)));
        reg(env.rShapeT, envLabel(nm(ms.envLfoStorage.rShape)));
        orderGuard.separator();
        reg(env.rateMulT, envLabel(nm(ms.envLfoStorage.rateMul)));
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
      envFollowerSources(e), macroSources(e), subordinateVoiceSources(e), midiSources(e),
      keyAndPitchSources(e)
{
    registerGroupModSource(e, egSource[0], "Group EG", "GEG1");
    registerGroupModSource(e, egSource[1], "Group EG", "GEG2");

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

    for (int i = 0; i < scxt::envFollowersPerGroupOrZone; ++i)
    {
        m.bindSourceValue(envFollowerSources.envFollowersL[i], g.envelopeFollowers[i].outputs[0]);
        m.bindSourceValue(envFollowerSources.envFollowersR[i], g.envelopeFollowers[i].outputs[1]);
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
        additiveFn,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
        shortPathFn,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
        shortNameFn)
{
    if (!e)
        return;

    e->registerGroupModTarget(
        t, pathFn, nameFn, additiveFn, [](const auto &, const auto &) { return true; }, shortPathFn,
        shortNameFn);
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

    auto identCmp = [e](const auto &a, const auto &b) {
        const auto &srca = std::get<0>(a);
        const auto &srcb = std::get<0>(b);
        const auto &ida = std::get<1>(a);
        const auto &idb = std::get<1>(b);

        if (srca.gid == srcb.gid && (srca.gid == 'gmac' || srca.gid == 'gelf'))
        {
            // Renaming makes macros a special case. Rows are just obvious
            return srca.index < srcb.index;
        }

        // categories (paths) sort alphabetically, keeping same-path items contiguous
        if (ida.first != idb.first)
            return strnatcasecmp(ida.first.c_str(), idb.first.c_str()) < 0;

        // within a category: explicit regOrder (>= 0) wins and precedes alphabetical items
        auto ordOf = [e](const auto &s) -> int32_t {
            auto it = e->groupModSources.find(s);
            return it == e->groupModSources.end() ? -1 : it->second.regOrder;
        };
        auto oa = ordOf(srca);
        auto ob = ordOf(srcb);
        if (oa >= 0 || ob >= 0)
        {
            if (oa >= 0 && ob >= 0)
                return oa < ob;
            return oa >= 0;
        }

        if (ida.second != idb.second)
            return strnatcasecmp(ida.second.c_str(), idb.second.c_str()) < 0;
        return std::hash<std::remove_cv_t<std::remove_reference_t<decltype(srca)>>>{}(srca) <
               std::hash<std::remove_cv_t<std::remove_reference_t<decltype(srcb)>>>{}(srcb);
    };

    // Targets carry a 4-tuple displayName (path, name, shortPath, shortName); categories sort
    // alphabetically by long path, items within a category by explicit regOrder then long name.
    auto tgtCmp = [e](const namedTarget_t &a, const namedTarget_t &b) {
        const auto &ta = std::get<0>(a);
        const auto &tb = std::get<0>(b);
        if (ta.gid == 'gprc' && tb.gid == ta.gid && tb.index == ta.index)
        {
            return ta.tid < tb.tid;
        }
        const auto &dna = std::get<1>(a);
        const auto &dnb = std::get<1>(b);
        if (displayPath(dna) != displayPath(dnb))
            return strnatcasecmp(displayPath(dna).c_str(), displayPath(dnb).c_str()) < 0;

        auto ordOf = [e](const GroupMatrixConfig::TargetIdentifier &t) -> int32_t {
            auto it = e->groupModTargets.find(t);
            return it == e->groupModTargets.end() ? -1 : it->second.regOrder;
        };
        auto oa = ordOf(ta);
        auto ob = ordOf(tb);
        if (oa >= 0 || ob >= 0)
        {
            if (oa >= 0 && ob >= 0)
                return oa < ob;
            return oa >= 0;
        }

        if (displayName(dna) != displayName(dnb))
            return strnatcasecmp(displayName(dna).c_str(), displayName(dnb).c_str()) < 0;
        return std::hash<GroupMatrixConfig::TargetIdentifier>{}(ta) <
               std::hash<GroupMatrixConfig::TargetIdentifier>{}(tb);
    };

    for (const auto &[t, fns] : e->groupModTargets)
    {
        const auto &pathFn = fns.pathFn;
        const auto &nameFn = fns.nameFn;
        const auto &shortPathFn = fns.shortPathFn;
        const auto &shortNameFn = fns.shortNameFn;
        const auto &additiveFn = fns.additiveFn;
        const auto &enabledFn = fns.enabledFn;
        auto p = pathFn(g, t);
        auto n = nameFn(g, t);
        auto sp = shortPathFn ? shortPathFn(g, t) : p;
        auto sn = shortNameFn ? shortNameFn(g, t) : n;
        tg.emplace_back(t, targetDisplayName_t{p, n, sp, sn}, additiveFn(g, t), enabledFn(g, t),
                        fns.separatorBefore);
    }
    std::sort(tg.begin(), tg.end(), tgtCmp);

    for (const auto &[s, fns] : e->groupModSources)
    {
        sr.emplace_back(s, identifierDisplayName_t{fns.pathFn(g, s), fns.nameFn(g, s)});
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
