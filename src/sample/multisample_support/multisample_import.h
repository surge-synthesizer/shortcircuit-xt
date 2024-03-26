//
// Created by Paul Walker on 3/25/24.
//

#ifndef SHORTCIRCUITXT_MULTISAMPLE_IMPORT_H
#define SHORTCIRCUITXT_MULTISAMPLE_IMPORT_H

#include "filesystem/import.h"
#include <engine/engine.h>

namespace scxt::multisample_support
{
bool importMultisample(const fs::path &, engine::Engine &);
}
#endif // SHORTCIRCUITXT_MULTISAMPLE_IMPORT_H
