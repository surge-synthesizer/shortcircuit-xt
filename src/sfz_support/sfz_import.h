//
// Created by Paul Walker on 4/2/23.
//

#ifndef SHORTCIRCUITXT_SFZ_IMPORT_H
#define SHORTCIRCUITXT_SFZ_IMPORT_H

#include <filesystem>
#include <engine/engine.h>
namespace scxt::sfz_support
{
bool importSFZ(const std::filesystem::path &, engine::Engine &);
}

#endif // SHORTCIRCUITXT_SFZ_IMPORT_H
