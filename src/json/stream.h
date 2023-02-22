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

#ifndef SCXT_SRC_JSON_STREAM_H
#define SCXT_SRC_JSON_STREAM_H

#include "engine/patch.h"
#include "engine/engine.h"

namespace scxt::json
{

static constexpr uint64_t currentStreamingVersion{0x20230201};

std::string streamPatch(const engine::Patch &p, bool pretty = false);
std::string streamEngineState(const engine::Engine &e, bool pretty = false);
void unstreamEngineState(engine::Engine &e, const std::string &jsonData);
void unstreamEngineState(engine::Engine &e, const fs::path &path);

} // namespace scxt::json

#endif // SHORTCIRCUIT_STREAM_H
