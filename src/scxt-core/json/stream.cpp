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

#include "stream.h"

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "scxt_traits.h"
#include "engine_traits.h"
#include "messaging/messaging.h"

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

void unstreamEngineState(engine::Engine &e, const std::string &data, bool msgPack)
{
    e.clearAll(false);
    if (msgPack)
    {
        tao::json::events::transformer<tao::json::events::to_basic_value<scxt_traits>> consumer;
        tao::json::msgpack::events::from_string(consumer, data);
        auto jv = std::move(consumer.value);
        jv.to(e);
    }
    else
    {
        tao::json::events::transformer<tao::json::events::to_basic_value<scxt_traits>> consumer;
        tao::json::events::from_string(consumer, data);
        auto jv = std::move(consumer.value);
        jv.to(e);
    }
    e.getSampleManager()->purgeUnreferencedSamples();
    e.sendFullRefreshToClient();
}

void unstreamPartState(engine::Engine &e, int part, const std::string &data, bool msgPack,
                       bool setStreamGuard)
{
    e.getPatch()->getPart(part)->clearGroups();
    {
        std::unique_ptr<engine::Engine::StreamGuard> sg;
        if (setStreamGuard)
        {
            SCLOG_IF(streaming, "Activating stream guard for part unstream");
            sg = std::make_unique<engine::Engine::StreamGuard>(engine::Engine::FOR_PART);
        }
        if (msgPack)
        {
            tao::json::events::transformer<tao::json::events::to_basic_value<scxt_traits>> consumer;
            tao::json::msgpack::events::from_string(consumer, data);
            auto jv = std::move(consumer.value);
            jv.to(*(e.getPatch()->getPart(part)));
        }
        else
        {
            tao::json::events::transformer<tao::json::events::to_basic_value<scxt_traits>> consumer;
            tao::json::events::from_string(consumer, data);
            auto jv = std::move(consumer.value);
            jv.to(*(e.getPatch()->getPart(part)));
        }
    }

    e.sendFullRefreshToClient();
}
} // namespace scxt::json