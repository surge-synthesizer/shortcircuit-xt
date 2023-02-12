/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#ifndef SCXT_SRC_JSON_MODULATION_TRAITS_H
#define SCXT_SRC_JSON_MODULATION_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "modulation/voice_matrix.h"
#include "modulation/modulators/steplfo.h"

template <> struct tao::json::traits<scxt::modulation::VoiceModMatrix::Routing>
{
    typedef scxt::modulation::VoiceModMatrix::Routing rt_t;
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const rt_t &t)
    {
        auto dn = scxt::modulation::getVoiceModMatrixDestStreamingName(t.dst);
        auto sn = scxt::modulation::getVoiceModMatrixSourceStreamingName(t.src);
        v = {{"src", sn}, {"dst", dn}, {"depth", t.depth}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, rt_t &result)
    {
        const auto &object = v.get_object();
        auto ss = v.at("src").template as<std::string>();
        auto ds = v.at("dst").template as<std::string>();
        result.src = scxt::modulation::fromVoiceModMatrixSourceStreamingName(ss).value_or(
            scxt::modulation::vms_none);
        result.dst = scxt::modulation::fromVoiceModMatrixDestStreamingName(ds).value_or(
            scxt::modulation::vmd_none);
        v.at("depth").to(result.depth);
    }
};

template <> struct tao::json::traits<scxt::modulation::modulators::StepLFOStorage>
{
    typedef scxt::modulation::modulators::StepLFOStorage rt_t;
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const rt_t &t)
    {
        v = {{"data", t.data},
             {"repeat", t.repeat},
             {"rate", t.rate},
             {"smooth", t.smooth},
             {"shuffle", t.shuffle},
             {"temposync", t.temposync},
             {"triggermode", t.triggermode},
             {"cyclemode", t.cyclemode},
             {"onlyonce", t.onlyonce}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, rt_t &result)
    {
        const auto &object = v.get_object();
        v.at("data").to(result.data);
        v.at("repeat").to(result.repeat);
        v.at("rate").to(result.rate);
        v.at("smooth").to(result.smooth);
        v.at("shuffle").to(result.shuffle);
        v.at("temposync").to(result.temposync);
        v.at("triggermode").to(result.triggermode);
        v.at("cyclemode").to(result.cyclemode);
        v.at("onlyonce").to(result.onlyonce);
    }
};

#endif // SHORTCIRCUIT_MODULATION_TRAITS_H
