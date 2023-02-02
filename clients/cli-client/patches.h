//
// Created by Paul Walker on 1/30/23.
//

#ifndef SCXT_MAIN_TEST__PATCHES_H
#define SCXT_MAIN_TEST__PATCHES_H

#include "engine/engine.h"

namespace scxt::cli_client
{

bool basicTestPatch(engine::Engine &engine)
{
    auto dir = fs::path{"resources/test_samples/next"};
    for (const auto &[s, k0, k1] : {std::make_tuple(std::string("Kick.wav"), 60, 64),
                                    std::make_tuple(std::string("Beep.wav"), 60, 62),
                                    {"Hat.wav", 65, 67},
                                    {"PulseSaw.wav", 48, 59},
                                    {"Beep.wav", 48, 52}})
    {
        auto sid = engine.getSampleManager()->loadSampleByPath(dir / s);

        if (!sid.has_value())
        {
            std::cout << "Unable to load sample " << s << std::endl;
            return false;
        }
        auto zptr = std::make_unique<scxt::engine::Zone>(*sid);
        zptr->keyboardRange = {k0, k1};
        zptr->rootKey = k0;

        zptr->filterType[0] = dsp::filter::ft_osc_pulse_sync;
        zptr->filterMix[0] = 0.5;
        zptr->routingTable[0].src = modulation::vms_LFO1;
        zptr->routingTable[0].dst = modulation::vmd_Filter1_Mix;
        zptr->routingTable[0].depth = 0.5;

        zptr->filterType[1] = dsp::filter::ft_SuperSVF;
        zptr->filterMix[1] = 1.0;

        modulation::modulators::load_lfo_preset(modulation::modulators::lp_sine,
                                                &zptr->lfoStorage[0]);

        zptr->attachToSample(*engine.getSampleManager());

        engine.getPatch()->getPart(0)->getGroup(0)->addZone(zptr);
    }
    return true;
}

} // namespace scxt::cli_client

#endif // SCXT_MAIN_TEST__PATCHES_H
