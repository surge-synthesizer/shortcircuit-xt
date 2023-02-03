//
// Created by Paul Walker on 2/3/23.
//

#ifndef SHORTCIRCUIT_MODULATION_TRAITS_H
#define SHORTCIRCUIT_MODULATION_TRAITS_H

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
