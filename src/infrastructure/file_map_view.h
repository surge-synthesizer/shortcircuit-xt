/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#ifndef SCXT_SRC_INFRASTRUCTURE_FILE_MAP_VIEW_H
#define SCXT_SRC_INFRASTRUCTURE_FILE_MAP_VIEW_H

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
