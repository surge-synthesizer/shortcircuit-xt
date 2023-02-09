/*
 * ShortCircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortCircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortCircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortCircuitXT is inspired by, and shares some code with, the
 * commercial product ShortCircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for ShortCircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortCircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_JSON_SAMPLE_TRAITS_H
#define SCXT_SRC_JSON_SAMPLE_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "stream.h"

#include "sample/sample_manager.h"
#include "scxt_traits.h"

namespace scxt::json
{
template <> struct scxt_traits<scxt::sample::SampleManager>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::sample::SampleManager &e)
    {
        // TODO: Probably stream samples better than this
        v = {{"samplePaths", e.getPathsAndIDs()}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::sample::SampleManager &mgr)
    {
        mgr.reset();
        auto arr = v.at("samplePaths").get_array();
        for (const auto el : arr)
        {
            scxt::SampleID id;
            std::string str;
            el.get_array()[0].to(id);
            el.get_array()[1].to(str);
            mgr.loadSampleByPathToID(fs::path{str}, id);
        }
    }
};
} // namespace scxt::json
#endif // SHORTCIRCUIT_SAMPLE_TRAITS_H
