//
// Created by Paul Walker on 2/2/23.
//

#ifndef SCXT_SRC_JSON_STREAM_H
#define SCXT_SRC_JSON_STREAM_H

#include "engine/patch.h"
#include "engine/engine.h"

namespace scxt::json
{

static constexpr uint64_t currentStreamingVersion{0x20230201};

std::string streamPatch(const engine::Patch &p, bool pretty = false);
std::string streamEngineState(const engine::Engine &e, bool pretty = false);
void unstreamEngineState(engine::Engine &e, const std::string jsonData);
void unstreamEngineState(engine::Engine &e, const fs::path &path);

} // namespace scxt::json

#endif // SHORTCIRCUIT_STREAM_H
