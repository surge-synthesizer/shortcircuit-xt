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

#include "group_matrix.h"
#include "engine/group.h"
#include <stdexcept>

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

void GroupMatrixEndpoints::Sources::bind(scxt::modulation::GroupMatrix &matrix,
                                         engine::Group &group)
{
    SCLOG_UNIMPL("Bind Sources");
}

void GroupMatrixEndpoints::registerGroupModTarget(
    engine::Engine *e, const GroupMatrixConfig::TargetIdentifier &,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
        pathFn,
    std::function<std::string(const engine::Group &, const GroupMatrixConfig::TargetIdentifier &)>
        nameFn)
{
}

} // namespace scxt::modulation
