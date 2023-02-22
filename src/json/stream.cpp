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

#include "stream.h"

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/from_file.hpp>
#include <tao/json/contrib/traits.hpp>

#include "scxt_traits.h"
#include "engine_traits.h"

namespace scxt::json
{

template <template <typename...> class... Transformers, template <typename...> class Traits>
void to_pretty_stream(std::ostream &os, const tao::json::basic_value<Traits> &v)
{
    tao::json::events::transformer<tao::json::events::to_pretty_stream, Transformers...> consumer(
        os, 3);
    tao::json::events::from_value(consumer, v);
}

template <typename T> std::string streamValue(const T &v, bool pretty)
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
    return streamValue(json::scxt_value(p), pretty);
}

std::string streamEngineState(const engine::Engine &e, bool pretty)
{
    return streamValue(json::scxt_value(e), pretty);
}

void unstreamEngineState(engine::Engine &e, const fs::path &path)
{
    tao::json::events::transformer<tao::json::events::to_basic_value<scxt_traits>> consumer;
    tao::json::events::from_file(consumer, path);
    auto jv = std::move(consumer.value);
    jv.to(e);
}

void unstreamEngineState(engine::Engine &e, const std::string &xml)
{
    tao::json::events::transformer<tao::json::events::to_basic_value<scxt_traits>> consumer;
    tao::json::events::from_string(consumer, xml);
    auto jv = std::move(consumer.value);
    jv.to(e);
}
} // namespace scxt::json