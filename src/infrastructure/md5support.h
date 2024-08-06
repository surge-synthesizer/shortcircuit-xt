//
// Created by Paul Walker on 8/5/24.
//

#ifndef SHORTCIRCUITXT_MD5SUPPORT_H
#define SHORTCIRCUITXT_MD5SUPPORT_H

#include <string>
#include "filesystem_import.h"
#include "file_map_view.h"
#include "md5.h"

namespace scxt::infrastructure
{
inline std::string createMD5SumFromFile(const fs::path &path)
{
    auto fmp = infrastructure::FileMapView(path);
    if (!fmp.isMapped())
        return {};

    return md5::MD5::Hash(fmp.data(), fmp.dataSize());
}

} // namespace scxt::infrastructure
#endif // SHORTCIRCUITXT_MD5SUPPORT_H
