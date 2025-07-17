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

#include "part.h"
#include "bus.h"
#include "patch.h"
#include "engine.h"
#include "feature_enums.h"

#include "selection/selection_manager.h"

#include "sst/basic-blocks/simd/setup.h"
#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/PanLaws.h"
#include "messaging/messaging.h"

namespace scxt::engine
{
void Part::process(Engine &e)
{
    namespace blk = sst::basic_blocks::mechanics;

    float lcp alignas(16)[2][blockSize];
    float defOut alignas(16)[2][blockSize];

    macroLagHandler.process();
    externalSignalLag.processAll();

    auto lev = configuration.level;
    lev = lev * lev * lev;

    bool defaultAssigned{false};

    namespace pl = sst::basic_blocks::dsp::pan_laws;
    auto pan = configuration.pan;
    pl::panmatrix_t pmat{1, 1, 0, 0};
    if (pan != 0)
        pl::stereoEqualPower(0.5 * (pan + 1), pmat);

    for (const auto &g : groups)
    {
        if (g->isActive())
        {
            g->process(e);

            auto bi = g->outputInfo.routeTo;
            if (bi == DEFAULT_BUS || bi == configuration.routeTo)
            {
                blk::mul_block<blockSize>(g->output[0], lev, lcp[0]);
                blk::mul_block<blockSize>(g->output[1], lev, lcp[1]);

                if (pan != 0.0)
                {
                    for (int i = 0; i < blockSize; ++i)
                    {
                        auto il = lcp[0][i];
                        auto ir = lcp[1][i];
                        lcp[0][i] = pmat[0] * il + pmat[2] * ir;
                        lcp[1][i] = pmat[1] * ir + pmat[3] * il;
                    }
                }

                if (defaultAssigned)
                {
                    blk::accumulate_from_to<blockSize>(lcp[0], defOut[0]);
                    blk::accumulate_from_to<blockSize>(lcp[1], defOut[1]);
                }
                else
                {
                    blk::copy_from_to<blockSize>(lcp[0], defOut[0]);
                    blk::copy_from_to<blockSize>(lcp[1], defOut[1]);
                    defaultAssigned = true;
                }
            }
            else
            {
                auto &obus = e.getPatch()->getBusForOutput(bi);

                blk::accumulate_from_to<blockSize>(g->output[0], obus.output[0]);
                blk::accumulate_from_to<blockSize>(g->output[1], obus.output[1]);
            }
        }
    }

    if (defaultAssigned)
    {
        // this should be the route to point
        auto bi = configuration.routeTo;
        if (configuration.routeTo == DEFAULT_BUS)
            bi = (BusAddress)(PART_0 + partNumber);

        for (auto &p : partEffects)
        {
            if (p)
            {
                p->process(defOut[0], defOut[1]);
            }
        }

        auto &obus = e.getPatch()->getBusForOutput(bi);

        blk::accumulate_from_to<blockSize>(defOut[0], obus.output[0]);
        blk::accumulate_from_to<blockSize>(defOut[1], obus.output[1]);
    }
}

Part::zoneMappingSummary_t Part::getZoneMappingSummary()
{
    zoneMappingSummary_t res;

    int pidx{partNumber};
    int gidx{0};
    for (const auto &g : groups)
    {
        int zidx{0};
        for (const auto &z : *g)
        {
            // get address for zone
            auto addr = selection::SelectionManager::ZoneAddress(pidx, gidx, zidx);
            int32_t features{0};
            if (z->missingSampleCount() > 0)
            {
                features |= ZoneFeatures::MISSING_SAMPLE;
            }
            auto data = zoneMappingItem_t{addr, z->mapping.keyboardRange, z->mapping.velocityRange,
                                          z->getName(), features};
            // res[addr] = data;
            res.emplace_back(data);
            zidx++;
        }
        gidx++;
    }
    return res;
}

std::vector<SampleID> Part::getSamplesUsedByPart() const
{
    std::unordered_set<SampleID> resSet;
    for (const auto &g : groups)
    {
        for (const auto &z : g->getZones())
        {
            for (const auto &var : z->variantData.variants)
            {
                if (var.sampleID.isValid())
                {
                    resSet.insert(var.sampleID);
                }
            }
        }
    }
    return std::vector<SampleID>(resSet.begin(), resSet.end());
}

void Part::moveGroupToAfter(size_t whichGroup, size_t toAfter)
{
    if (whichGroup < 0 || whichGroup >= groups.size() || toAfter < 0 || toAfter >= groups.size() ||
        whichGroup == toAfter)
    {
        return;
    }
    if (whichGroup < toAfter)
    {
        auto og = std::move(groups[whichGroup]);
        for (int i = whichGroup; i < toAfter; ++i)
        {
            groups[i] = std::move(groups[i + 1]);
        }
        groups[toAfter] = std::move(og);
    }
    else
    {
        // so move whichgroup=4 to after toAfter=1
        // 1 2 3 4 becomes 1 4 2 3
        // so grab 4 (1 2 3 x)
        // 3-> 4, 2-> 3 (1 x 2 3)
        // position
        auto og = std::move(groups[whichGroup]);
        for (int i = whichGroup - 1; i > toAfter; --i)
        {
            groups[i + 1] = std::move(groups[i]);
        }
        groups[toAfter + 1] = std::move(og);
    }
}

void Part::initializeAfterUnstream(Engine &e)
{
    for (int idx = 0; idx < maxEffectsPerPart; ++idx)
    {
        partEffects[idx] = createEffect(partEffectStorage[idx].type, &e, &partEffectStorage[idx]);
        if (partEffects[idx])
        {
            partEffects[idx]->init(false);
            sendBusEffectInfoToClient(e, idx);
        }
    }
}

void Part::setBusEffectType(Engine &e, int idx, AvailableBusEffects t)
{
    assert(idx >= 0 && idx < maxEffectsPerPart);
    partEffects[idx] = createEffect(t, &e, &partEffectStorage[idx]);
    if (partEffects[idx])
        partEffects[idx]->init(true);
}

void Part::sendBusEffectInfoToClient(const Engine &e, int slot)
{
    std::array<datamodel::pmd, BusEffectStorage::maxBusEffectParams> pmds;

    int saz{0};
    if (partEffects[slot])
    {
        for (int i = 0; i < partEffects[slot]->numParams(); ++i)
        {
            pmds[i] = partEffects[slot]->paramAt(i);
        }
        saz = partEffects[slot]->numParams();
    }
    for (int i = saz; i < BusEffectStorage::maxBusEffectParams; ++i)
        pmds[i].type = sst::basic_blocks::params::ParamMetaData::NONE;

    messaging::client::serializationSendToClient(
        messaging::client::s2c_bus_effect_full_data,
        messaging::client::busEffectFullData_t{
            (int)-1, partNumber, slot, {pmds, partEffectStorage[slot]}},
        *(e.getMessageController()));
}

} // namespace scxt::engine