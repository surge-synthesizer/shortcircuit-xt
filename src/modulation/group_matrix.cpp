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
            return "mix";
        };

        registerGroupModTarget(e, mixT, ptFn, mixFn);
        for (int i = 0; i < scxt::maxProcessorFloatParams; ++i)
        {
            auto elFn = [icopy = i](const engine::Group &z,
                                    const GroupMatrixConfig::TargetIdentifier &t) -> std::string {
                auto &d = z.processorDescription[t.index];
                if (d.type == dsp::processor::proct_none)
                    return "";
                return d.floatControlDescriptions[icopy].name;
            };
            registerGroupModTarget(e, fpT[i], ptFn, elFn);
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
    int idx{0};
    for (int i = 0; i < lfosPerGroup; ++i)
    {
        switch (g.lfoEvaluator[i])
        {
        case engine::Group::CURVE:
        {
            m.bindSourceValue(lfoSources.sources[i], g.curveLfos[i].output);
        }
        break;
        case engine::Group::STEP:
        {
            m.bindSourceValue(lfoSources.sources[i], g.stepLfos[i].output);
        }
        break;
        case engine::Group::ENV:
            m.bindSourceValue(lfoSources.sources[i], g.envLfos[i].output);
            break;
        case engine::Group::MSEG:
            m.bindSourceValue(lfoSources.sources[i], zeroSource);
            break;
        }
    }

    for (int i = 0; i < scxt::numTransportPhasors; ++i)
    {
        assert(g.getEngine());
        m.bindSourceValue(transportSources.phasors[i], g.getEngine()->transportPhasors[i]);
    }
}

void GroupMatrixEndpoints::registerGroupModTarget(
    engine::Engine *e, const GroupMatrixConfig::TargetIdentifier &t,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
        pathFn,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
        nameFn)
{
    if (!e)
        return;

    e->registerGroupModTarget(t, pathFn, nameFn);
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

groupMatrixMetadata_t getGroupMatrixMetadata(engine::Group &g)
{
    // somewhat dissatisfied with amount of copying here also
    auto e = g.getEngine();

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
        if (ta.gid == 'gprc' && tb.gid == ta.gid && tb.index == ta.index)
        {
            return ta.tid < tb.tid;
        }
        return identCmp(a, b);
    };

    for (const auto &[t, fns] : e->groupModTargets)
    {
        tg.emplace_back(t, identifierDisplayName_t{fns.first(g, t), fns.second(g, t)});
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
} // namespace scxt::modulation
