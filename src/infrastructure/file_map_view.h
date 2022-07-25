/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_FILE_MAP_VIEW_H
#define SHORTCIRCUIT_FILE_MAP_VIEW_H

#include <string>
#include <memory>
#include "filesystem/import.h"

namespace scxt
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
} // namespace scxt

#endif // SHORTCIRCUIT_FILE_MAP_VIEW_H
