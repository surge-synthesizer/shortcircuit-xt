/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_FILE_MAP_VIEW_H
#define SHORTCIRCUIT_FILE_MAP_VIEW_H

#include <string>
#include <memory>
#include "infrastructure/import_fs.h"

/*
 * This class provides a mechanism to do a MapViewOfFile/mmap in a portable
 * way. The intent is that you would use it with a unique_ptr where you needed
 * the map scope. So, for instance
 *
 * fs::path p = string_to_path("foo.bin");
 * auto mapper = std::make_unique<SC3::FileMapView>(p);
 * auto d = mapper->data; // valid until mapper is destroyed
 * auto s = mapper->dataSize;
 */

namespace SC3
{
class FileMapView
{
  public:
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
} // namespace SC3

#endif // SHORTCIRCUIT_FILE_MAP_VIEW_H
