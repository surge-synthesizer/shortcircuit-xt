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

#ifndef SCXT_SRC_SCXT_CORE_INFRASTRUCTURE_FILE_MAP_VIEW_H
#define SCXT_SRC_SCXT_CORE_INFRASTRUCTURE_FILE_MAP_VIEW_H

#include <string>
#include <memory>

#include "filesystem_import.h"

namespace scxt::infrastructure
{

/**
 * This class provides a mechanism to do a MapViewOfFile/mmap in a portable
 * way. The intent is that you would use it with a unique_ptr where you needed
 * the map scope.
 *
 * ```cpp
 * fs::path p = string_to_path("foo.bin");
 * auto mapper = std::make_unique<scxt::FileMapView>(p);
 * auto d = mapper->data; // valid until mapper is destroyed
 * auto s = mapper->dataSize;
 * ```
 */
class FileMapView
{
  public:
    /**
     * Construct a view of a file
     * @param filename the path to the file to be mapped. If unavabiable, isMapped will return false
     */
    FileMapView(const fs::path &filename);
    ~FileMapView();

    bool isMapped();
    void *data();
    size_t dataSize();

    struct Impl
    {
        virtual ~Impl() = default;
    };
    std::unique_ptr<Impl> impl;
};
} // namespace scxt::infrastructure

#endif // SHORTCIRCUIT_FILE_MAP_VIEW_H
