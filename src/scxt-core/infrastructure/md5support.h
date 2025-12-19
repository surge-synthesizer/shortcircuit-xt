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

    // Calculate the MD5 hash
    CC_MD5(fmp.data(), fmp.dataSize(), digest);

    return hexString(digest, CC_MD5_DIGEST_LENGTH);
#else
    return md5::MD5::Hash(fmp.data(), fmp.dataSize());
#endif
}

} // namespace scxt::infrastructure
#endif // SHORTCIRCUITXT_MD5SUPPORT_H
