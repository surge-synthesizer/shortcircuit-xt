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

#include <fstream>

#include "configuration.h"
#include "engine/engine.h"
#include "patch_io/patch_io.h"
#include "json/scxt_traits.h"
#include "json/engine_traits.h"

#include "tao/json/msgpack/to_string.hpp"

template <template <typename...> class... Transformers, template <typename...> class Traits>
void to_pretty_stream(std::ostream &os, const tao::json::basic_value<Traits> &v)
{
    tao::json::events::transformer<tao::json::events::to_pretty_stream, Transformers...> consumer(
        os, 3);
    tao::json::events::from_value(consumer, v);
}

template <typename T> std::string streamValue(const T &v)
{
    std::ostringstream oss;
    to_pretty_stream(oss, v);
    return oss.str();
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        SCLOG_IF(cliTools, "Usage: " << argv[0] << " [filename]");
        return 1;
    }

    auto rs = scxt::patch_io::retrieveSCManifestAndPayload(fs::path(argv[1]));
    if (rs.has_value())
    {
        std::cout << "#  MANIFEST\n\n";
        std::cout << rs->first << std::endl;

        std::cout << "#  PAYLOAD\n\n";
        tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
            consumer;
        tao::json::msgpack::events::from_string(consumer, rs->second);
        auto jv = std::move(consumer.value);

        std::cout << streamValue(jv) << std::endl;
        ;
    }
}