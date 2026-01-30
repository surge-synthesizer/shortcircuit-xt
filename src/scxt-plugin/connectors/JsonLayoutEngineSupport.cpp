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
#include <sstream>

#include "JsonLayoutEngineSupport.h"

#include <cmrc/cmrc.hpp>
#include "configuration.h"

CMRC_DECLARE(scxtui_json_layouts);

namespace scxt::ui::connectors::jsonlayout
{
std::optional<std::string> resolveJsonFile(const std::string &nm)
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
                SCLOG_IF(jsonUI, "Setting JSON path from environment: " << sep.u8string());
                isLocal = true;
                localPath = sep;
            }
        }

        if (!isLocal)
        {
            auto rp = fs::path{"src/scxt-plugin"} / "json-assets";
            if (fs::exists(rp))
            {
                SCLOG_IF(jsonUI, "Setting JSON path from working dir: " << rp.u8string());
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
                SCLOG_IF(jsonUI, "Unable to open json file: '" << rp.u8string() << "'");
            }
        }
        else
        {
            SCLOG_IF(jsonUI, "isLocal is true and local json missing: '" << rp.u8string() << "'");
        }
    }

    try
    {
        auto fs = cmrc::scxtui_json_layouts::get_filesystem();
        auto fn = "json-assets/" + nm;
        auto jsnf = fs.open(fn);
        std::string json(jsnf.begin(), jsnf.end());

        return json;
    }
    catch (std::exception &e)
    {
        SCLOG_IF(jsonUI, "Exception with cmrc : " << e.what())
    }

    SCLOG_IF(jsonUI, "Unable to resolve JSON from library: '" << nm);
    return std::nullopt;
}
} // namespace scxt::ui::connectors::jsonlayout