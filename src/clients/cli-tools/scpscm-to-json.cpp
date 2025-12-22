/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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