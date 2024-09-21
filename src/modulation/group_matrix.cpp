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

#include <stdexcept>

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
                                    const GroupMatrixConfig::TargetIdentifier &t) -> bool {
                auto &d = z.processorDescription[t.index];
                if (d.type == dsp::processor::proct_none)
                    return "";
                return d.floatControlDescriptions[icopy].hasSupportsMultiplicativeModulation();
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
    for (auto &e : eg)
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
void GroupMatrixEndpoints::LFOTarget::bind(scxt::modulation::GroupMatrix &m, engine::Group &g)
{
    baseBind(m, g);
}

void GroupMatrixEndpoints::OutputTarget::bind(scxt::modulation::GroupMatrix &m, engine::Group &g)
{
    shmo::bindEl(m, g.outputInfo, ampT, g.outputInfo.amplitude, ampP);
    shmo::bindEl(m, g.outputInfo, panT, g.outputInfo.pan, panP);
}

void GroupMatrixEndpoints::Sources::bind(scxt::modulation::GroupMatrix &m, engine::Group &g)
{
    lfoSources.bind(m, g, zeroSource);

    int idx{0};

    for (int i = 0; i < scxt::numTransportPhasors; ++i)
    {
        assert(g.getEngine());
        m.bindSourceValue(transportSources.phasors[i], g.getEngine()->transportPhasors[i]);
    }

    auto *part = g.parentPart;
    for (int i = 0; i < macrosPerPart; ++i)
    {
        m.bindSourceValue(macroSources.macros[i], part->macros[i].value);
    }

    m.bindSourceValue(egSource[0], g.eg[0].outBlock0);
    m.bindSourceValue(egSource[1], g.eg[1].outBlock0);
}

void GroupMatrixEndpoints::registerGroupModTarget(
    engine::Engine *e, const GroupMatrixConfig::TargetIdentifier &t,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
        pathFn,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
        nameFn,
    std::function<bool(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
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
        if (srca.gid == 'gmac' && srcb.gid == 'gmac')
        {
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
                return ida.second < idb.second;
            }
        }
        return ida.first < idb.first;
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
                        std::get<2>(fns)(g, t)); // GroupMatrixConfig::getIsMultiplicative(t));
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
} // namespace scxt::modulation
