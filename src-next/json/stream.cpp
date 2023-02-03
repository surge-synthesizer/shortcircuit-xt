//
// Created by Paul Walker on 2/2/23.
//

#include "stream.h"

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/from_file.hpp>
#include <tao/json/contrib/traits.hpp>

#include "engine_traits.h"

namespace scxt::json
{

template <template <typename...> class... Transformers, template <typename...> class Traits>
void to_pretty_stream(std::ostream &os, const tao::json::basic_value<Traits> &v)
{
    tao::json::events::transformer<tao::json::events::to_pretty_stream, Transformers...> consumer(os, 3);
    tao::json::events::from_value(consumer, v);
}

template<typename T>
std::string streamValue(const T& v, bool pretty)
{
    if (pretty)
    {
        std::ostringstream oss;
        to_pretty_stream(oss, v);
        return oss.str();
    }
    else
    {
        return tao::json::to_string(v);
    }
}

std::string streamPatch(const engine::Patch &p, bool pretty)
{
    return streamValue(tao::json::value(p), pretty);
}


std::string streamEngineState(const engine::Engine &e, bool pretty)
{
    return streamValue(tao::json::value(e), pretty);
}

void unstreamEngineState(engine::Engine &e, const fs::path &path)
{
    auto v = tao::json::from_file(path);
    v.to(e);
}
} // namespace scxt::json