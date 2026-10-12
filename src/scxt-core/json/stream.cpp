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
#include <tao/json/msgpack/from_string.hpp>

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

static scxt_value parsePayload(const std::string &data, bool msgPack)
{
    tao::json::events::transformer<tao::json::events::to_basic_value<scxt_traits>> consumer;
    if (msgPack)
        tao::json::msgpack::events::from_string(consumer, data);
    else
        tao::json::events::from_string(consumer, data);
    return std::move(consumer.value);
}

// the version an engine state was written with, as the engine's own unstream reads it
static uint64_t engineStreamingVersion(const scxt_value &v)
{
    uint64_t sv{0};
    findIf(v, "streamingVersion", sv);
    return sv;
}

void unstreamEngineState(engine::Engine &e, const std::string &data, bool msgPack)
{
    e.clearAll(false);
    parsePayload(data, msgPack).to(e);
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
        parsePayload(data, msgPack).to(*(e.getPatch()->getPart(part)));
    }

    e.sendFullRefreshToClient();
}

void unstreamPartFromMultiState(engine::Engine &e, int part, int fromPart, const std::string &data,
                                bool msgPack)
{
    auto v = parsePayload(data, msgPack);
    const auto &parts = v.at("patch").at("parts").get_array();
    if (fromPart < 0 || fromPart >= (int)parts.size())
        throw std::runtime_error("The multi has no part " + std::to_string(fromPart + 1));

    engine::Engine::UnstreamGuard ug(engineStreamingVersion(v));
    auto sg = engine::Engine::StreamGuard(engine::Engine::FOR_PART);

    // samples before the part, as the engine unstream orders them, so its zones find them
    findIf(v, "sampleManager", *(e.getSampleManager()));

    auto &pt = e.getPatch()->getPart(part);
    pt->clearGroups();
    parts[fromPart].to(*pt);
    e.onPartConfigurationUpdated();

    e.sendFullRefreshToClient();
}

void unstreamMultiStructure(engine::Patch &p, const std::string &data, bool msgPack)
{
    auto v = parsePayload(data, msgPack);
    engine::Engine::UnstreamGuard ug(engineStreamingVersion(v));
    auto sg = engine::Engine::StreamGuard(engine::Engine::STRUCTURE_ONLY);
    v.at("patch").to(p);
}

void unstreamPartStructure(engine::Part &p, const std::string &data, bool msgPack)
{
    auto sg = engine::Engine::StreamGuard(engine::Engine::STRUCTURE_ONLY);
    parsePayload(data, msgPack).to(p);
}
} // namespace scxt::json