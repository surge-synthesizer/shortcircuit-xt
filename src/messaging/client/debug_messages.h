/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

/*
 * This is a set of wing-it-and-hope messages I have put
 * together so the UI can ask the serialization thread to
 * do debuggina and dumps. Ideally when we are done these
 * messages are not sent by most users most of the time.
 * Lets see!
 */
#ifndef SCXT_SRC_MESSAGING_CLIENT_DEBUG_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_DEBUG_MESSAGES_H

#include "client_macros.h"
#include "client_serial.h"
#include <map>
#include "engine/engine.h"

namespace scxt::messaging::client
{
using debugResponse_t = std::map<std::string, std::string>;
SERIAL_TO_CLIENT(DebugInfoGenerated, s2c_send_debug_info, debugResponse_t, onDebugInfoGenerated);

struct DebugActions
{
    static constexpr const char *pretty_json{"pretty_json"};
    static constexpr const char *pretty_json_daw{"pretty_json_daw"};
    static constexpr const char *pretty_json_multi{"pretty_json_multi"};
    static constexpr const char *pretty_json_part{"pretty_json_part"};
};

template <template <typename...> class... Transformers, template <typename...> class Traits>
inline void dbto_pretty_stream(std::ostream &os, const tao::json::basic_value<Traits> &v)
{
    tao::json::events::transformer<tao::json::events::to_pretty_stream, Transformers...> consumer(
        os, 3);
    tao::json::events::from_value(consumer, v);
}

inline void doDebugAction(const std::string &payload, const engine::Engine &engine,
                          MessageController &cont)
{
    if (payload == DebugActions::pretty_json || payload == DebugActions::pretty_json_daw ||
        payload == DebugActions::pretty_json_multi)
    {
        auto sr = scxt::engine::Engine::StreamReason::IN_PROCESS;
        if (payload == DebugActions::pretty_json_multi)
        {
            sr = engine::Engine::FOR_MULTI;
        }
        if (payload == DebugActions::pretty_json_daw)
        {
            sr = engine::Engine::FOR_DAW;
        }
        auto g = scxt::engine::Engine::StreamGuard(sr);
        auto res = scxt::json::streamEngineState(engine, true);
        SCLOG(res);

        auto nopres = scxt::json::streamEngineState(engine, false);
        SCLOG("Non-pretty engine state size " << nopres.size());
    }
    else if (payload == DebugActions::pretty_json_part)
    {
        auto pid = engine.getSelectionManager()->selectedPart;
        auto &p = engine.getPatch()->getPart(pid);

        std::ostringstream oss;
        dbto_pretty_stream(oss, json::scxt_value(p));
        SCLOG("Dumping json for part " << pid);
        SCLOG(oss.str());
    }
    else
    {
        SCLOG("Unknown debug action " << payload);
    }
}
CLIENT_TO_SERIAL(RequestDebugAction, c2s_request_debug_action, std::string,
                 doDebugAction(payload, engine, cont));
} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_DEBUG_MESSAGES_H
