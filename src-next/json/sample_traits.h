//
// Created by Paul Walker on 2/2/23.
//

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
