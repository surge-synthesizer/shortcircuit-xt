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

namespace scxt::json
{

template <> struct scxt_traits<modulation::VoiceModMatrixDestinationAddress>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const modulation::VoiceModMatrixDestinationAddress &t)
    {
        auto dn = scxt::modulation::getVoiceModMatrixDestStreamingName(t.type);
        v = {{"type", dn}, {"index", t.index}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v,
                   modulation::VoiceModMatrixDestinationAddress &t)
    {
        std::string tsn;
        findIf(v, "type", tsn);
        t.type = scxt::modulation::fromVoiceModMatrixDestStreamingName(tsn).value_or(
            scxt::modulation::vmd_none);
        findIf(v, "index", t.index);
    }
};

template <> struct scxt_traits<modulation::VoiceModMatrixSource>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const modulation::VoiceModMatrixSource &t)
    {
        auto sn = scxt::modulation::getVoiceModMatrixSourceStreamingName(t);
        v = {{"vms_name", sn}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, modulation::VoiceModMatrixSource &t)
    {
        std::string tsn;
        findIf(v, "vms_name", tsn);
        t = scxt::modulation::fromVoiceModMatrixSourceStreamingName(tsn).value_or(
            scxt::modulation::vms_none);
    }
};

template <> struct scxt_traits<modulation::VoiceModMatrixCurve>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const modulation::VoiceModMatrixCurve &t)
    {
        auto sn = scxt::modulation::getVoiceModMatrixCurveStreamingName(t);
        v = {{"vmc_name", sn}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, modulation::VoiceModMatrixCurve &t)
    {
        std::string tsn;
        findIf(v, "vmc_name", tsn);
        t = scxt::modulation::fromVoiceModMatrixCurveStreamingName(tsn).value_or(
            scxt::modulation::vmc_none);
    }
};

template <> struct scxt_traits<scxt::modulation::VoiceModMatrix::Routing>
{
    typedef scxt::modulation::VoiceModMatrix::Routing rt_t;
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const rt_t &t)
    {
        v = {{"active", t.active}, {"selConsistent", t.selConsistent},
             {"src", t.src},       {"srcVia", t.srcVia},
             {"dst", t.dst},       {"curve", t.curve},
             {"depth", t.depth}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, rt_t &result)
    {
        findOrSet(v, "active", true, result.active);
        findOrSet(v, "selConsistent", true, result.selConsistent);
        findIf(v, "src", result.src);
        findIf(v, "srcVia", result.srcVia);
        findIf(v, "dst", result.dst);
        findOrSet(v, "curve", modulation::vmc_none, result.curve);
        findIf(v, "depth", result.depth);
    }
};

template <> struct scxt_traits<scxt::modulation::modulators::StepLFOStorage>
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
             {"triggermode", (int32_t)t.triggermode},
             {"cyclemode", t.cyclemode},
             {"onlyonce", t.onlyonce}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, rt_t &result)
    {
        const auto &object = v.get_object();
        findIf(v, "data", result.data);
        findIf(v, "repeat", result.repeat);
        findIf(v, "rate", result.rate);
        findIf(v, "smooth", result.smooth);
        findIf(v, "shuffle", result.shuffle);
        findIf(v, "temposync", result.temposync);
        int32_t tm;
        findIf(v, "triggermode", tm);
        result.triggermode = (rt_t::TriggerModes)tm;
        findIf(v, "cyclemode", result.cyclemode);
        findIf(v, "onlyonce", result.onlyonce);
    }
};
} // namespace scxt::json

#endif // SHORTCIRCUIT_MODULATION_TRAITS_H
