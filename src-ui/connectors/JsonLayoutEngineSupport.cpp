//
// Created by Paul Walker on 10/22/25.
//

#include "JsonLayoutEngineSupport.h"
#include "JSONAssetSupport.h" // for now

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(scxtui_json_layouts);

namespace scxt::ui::connectors::jsonlayout
{
std::string resolveJsonFile(const std::string &nm)
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
            auto rp = fs::path{"src-ui"} / "json-assets";
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
        auto fn = "json-assets/" + nm;
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
} // namespace scxt::ui::connectors::jsonlayout