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

#ifndef SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_ENV_FOLLOWER_H
#define SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_ENV_FOLLOWER_H
#include "configuration.h"
#include "modulation/modulator_storage.h"
#include "sst/basic-blocks/dsp/Ballistics.h"
#include "sst/basic-blocks/mechanics/block-ops.h"
#include "dsp/data_tables.h"

#include <array>

namespace scxt::modulation::modulators
{
struct EnvFollower
{
    std::array<float, 2> outputs{0.f, 0.f};
    sst::basic_blocks::dsp::Ballistics ballistics{};

    template <bool OS> void process_block(const float *const inputL, const float *const inputR)
    {
        namespace mech = sst::basic_blocks::mechanics;

        set_coeffs();
        auto gain = dsp::dbTable.dbToLinear(settings->gain);

        if constexpr (OS)
        {
            if (settings->stereoLink > 0)
            {
                float res alignas(16)[blockSize << 1];
                for (int s = 0; s < blockSize << 1; ++s)
                {
                    ballistics.process_sample(gain * M_SQRT1_2 * std::fabs(inputL[s] + inputR[s]),
                                              res[s]);
                }
                outputs[0] = mech::blockMax<blockSize << 1>(res);
            }
            else
            {
                float resL alignas(16)[blockSize << 1];
                float resR alignas(16)[blockSize << 1];
                for (int s = 0; s < blockSize << 1; ++s)
                {
                    ballistics.process_sample(gain * std::fabs(inputL[s]),
                                              gain * std::fabs(inputR[s]), resL[s], resR[s]);
                }
                outputs[0] = mech::blockMax<blockSize << 1>(resL);
                outputs[1] = mech::blockMax<blockSize << 1>(resR);
            }
        }
        else
        {
            if (settings->stereoLink > 0)
            {
                float res alignas(16)[blockSize];
                for (int s = 0; s < blockSize; ++s)
                {
                    ballistics.process_sample(gain * M_SQRT1_2 * std::fabs(inputL[s] + inputR[s]),
                                              res[s]);
                }
                outputs[0] = mech::blockMax<blockSize>(res);
            }
            else
            {
                float resL alignas(16)[blockSize];
                float resR alignas(16)[blockSize];
                for (int s = 0; s < blockSize; ++s)
                {
                    ballistics.process_sample(gain * std::fabs(inputL[s]),
                                              gain * std::fabs(inputR[s]), resL[s], resR[s]);
                }
                outputs[0] = mech::blockMax<blockSize>(resL);
                outputs[1] = mech::blockMax<blockSize>(resR);
            }
        }
    }

    template <bool OS> void process_block(const float *const inputL)
    {
        namespace mech = sst::basic_blocks::mechanics;

        set_coeffs();
        auto gain = dsp::dbTable.dbToLinear(settings->gain);

        if constexpr (OS)
        {
            float res alignas(16)[blockSize << 1];
            for (int s = 0; s < blockSize << 1; ++s)
            {
                ballistics.process_sample(gain * std::fabs(inputL[s]), res[s]);
            }
            outputs[0] = mech::blockMax<blockSize << 1>(res);
            outputs[1] = 0.f;
        }
        else
        {
            float res alignas(16)[blockSize];
            for (int s = 0; s < blockSize; ++s)
            {
                ballistics.process_sample(gain * std::fabs(inputL[s]), res[s]);
            }
            outputs[0] = mech::blockMax<blockSize>(res);
            outputs[1] = 0.f;
        }
    }

    void assign(EnvFollowerStorage *s) { settings = s; }
    void setSampleRate(float sr) { ballistics.setSampleRate(sr); }

  private:
    EnvFollowerStorage *settings{nullptr};
    float attack_prior{123456789.f}, release_prior{123456789.f};

    void set_coeffs()
    {
        if (settings->attack != attack_prior)
        {
            ballistics.set_attack(settings->attack);
            attack_prior = settings->attack;
        }
        if (settings->release != release_prior)
        {
            ballistics.set_release(settings->release);
            release_prior = settings->release;
        }
    }
};
} // namespace scxt::modulation::modulators

#endif // SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_ENV_FOLLOWER_H
