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

#ifndef SCXT_SRC_SCXT_CORE_INFRASTRUCTURE_MD5SUPPORT_H
#define SCXT_SRC_SCXT_CORE_INFRASTRUCTURE_MD5SUPPORT_H

#include <string>
#include "filesystem_import.h"
#include "file_map_view.h"
#include "md5.h"

#if MAC
#include <CommonCrypto/CommonDigest.h>
#endif

namespace scxt::infrastructure
{

inline std::string hexString(unsigned char *digest, int length)
{
    char buf[3];
    std::ostringstream s;
    for (int i = 0; i < length; ++i)
    {
        snprintf(buf, 3, "%02x", digest[i]);
        buf[2] = 0;
        s << buf;
    }
    return s.str();
}

inline std::string createMD5SumFromFile(const fs::path &path)
{
    auto fmp = infrastructure::FileMapView(path);
    if (!fmp.isMapped())
        return {};

#if MAC
    unsigned char digest[CC_MD5_DIGEST_LENGTH]; // CC_MD5_DIGEST_LENGTH is 16 bytes

    // macos has wisely deprecated CC_MD5 as cryptographically insecure but we are just using it
    // for identity so turn off the warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

    // Calculate the MD5 hash
    CC_MD5(fmp.data(), fmp.dataSize(), digest);

#pragma clang diagnostic pop

    return hexString(digest, CC_MD5_DIGEST_LENGTH);
#else
    return md5::MD5::Hash(fmp.data(), fmp.dataSize());
#endif
}

} // namespace scxt::infrastructure
#endif // SHORTCIRCUITXT_MD5SUPPORT_H
