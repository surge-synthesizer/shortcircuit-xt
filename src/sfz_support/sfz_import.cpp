//
// Created by Paul Walker on 4/2/23.
//

#include "sfz_import.h"
#include "sfz_parse.h"

namespace scxt::sfz_support
{
bool importSFZ(const std::filesystem::path &f, engine::Engine &e)
{
    SFZParser parser;

    auto doc = parser.parse(f);
    auto rootDir = f.parent_path();
    SCDBGCOUT << SCD(rootDir.u8string()) << std::endl;

    for (const auto &[r, list] : doc)
    {
        SCDBGCOUT << "HEADER " << r.name << std::endl;
        for (const auto &oc : list)
        {
            SCDBGCOUT << "   " << oc.name << "  -> [" << oc.value << "]" << std::endl;
        }
    }

    return false;
}
} // namespace scxt::sfz_support