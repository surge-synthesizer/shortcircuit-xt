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

#include <fstream>

#include <utils.h>

#include <filesystem/import.h>
#include "JSONLayoutConsumer.h"

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(scxtui_json_layouts);

namespace scxt::ui::connectors
{
std::string JSONLayoutLibrary::jsonForComponent(const std::string &nm)
{
    static bool checkedForLocal{false};
    static bool isLocal{false};
    static fs::path localPath{};

    if (!checkedForLocal)
    {
        checkedForLocal = true;
        auto se = getenv("SCXT_LAYOUT_SOURCE_DIR");
        if (se)
        {
            auto sep = fs::path{se};
            if (fs::exists(sep))
            {
                SCLOG("Setting JSON path from environment: " << sep.u8string());
                isLocal = true;
                localPath = sep;
            }
        }

        if (!isLocal)
        {
            auto rp = fs::path{"src-ui"} / "json-layout";
            if (fs::exists(rp))
            {
                SCLOG("Setting JSON path from working dir: " << rp.u8string());
                isLocal = true;
                localPath = rp;
            }
        }
    }

    if (isLocal)
    {
        auto rp = localPath / nm;
        if (fs::exists(rp))
        {
            std::ifstream ifj(rp);
            if (ifj.is_open())
            {
                std::stringstream buffer;
                buffer << ifj.rdbuf();
                ifj.close();
                return buffer.str();
            }
            else
            {
                SCLOG("Unable to open json file: '" << rp.u8string() << "'");
            }
        }
        else
        {
            SCLOG("isLocal is true and local json missing: '" << rp.u8string() << "'");
        }
    }

    try
    {
        auto fs = cmrc::scxtui_json_layouts::get_filesystem();
        auto fn = "json-layout/" + nm;
        auto jsnf = fs.open(fn);
        std::string json(jsnf.begin(), jsnf.end());

        return json;
    }
    catch (std::exception &e)
    {
        SCLOG("Exception with cmrc : " << e.what())
    }

    SCLOG("Unable to resolve JSON from library: '" << nm);
    return "{}";
}
} // namespace scxt::ui::connectors