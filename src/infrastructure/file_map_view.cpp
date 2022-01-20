/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include "infrastructure/file_map_view.h"
#include "util/scxtstring.h"
#include <cstdio>
#if WINDOWS
#include <windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#endif

namespace scxt
{

#if WINDOWS
struct WinImpl : FileMapView::Impl
{
    WinImpl(const fs::path &fname) { init(fname.wstring()); }
    ~WinImpl()
    {
        if (isMapped)
            UnmapViewOfFile(data);
        if (hmf)
            CloseHandle(hmf);
        if (hf)
            CloseHandle(hf);
        data = 0;
        dataSize = 0;
    }
    void init(std::wstring fname)
    {
        isMapped = 0;
        data = nullptr;
        dataSize = 0;

        hf = CreateFileW(fname.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                         FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (!hf)
            return;
        dataSize = GetFileSize(hf, NULL);

        hmf = CreateFileMappingW(hf, 0, PAGE_READONLY, 0, 0, 0);
        if (!hmf)
        {
            dataSize = 0;
            CloseHandle(hf);
            return;
        }

        data = MapViewOfFile(hmf, FILE_MAP_READ, 0, 0, 0);

        if (!data)
        {
            dataSize = 0;
            return;
        }
        isMapped = true;
    }
    void *data = nullptr;
    size_t dataSize = 0;
    bool isMapped = false;

    HANDLE hf = 0, hmf = 0;

    int fd = 0;
};

WinImpl *as(FileMapView::Impl *imp) { return reinterpret_cast<WinImpl *>(imp); }

#else
struct posixImpl : FileMapView::Impl
{
    posixImpl(const fs::path &fname) { init(fname); }
    ~posixImpl()
    {
        if (isMapped)
        {
            munmap(data, dataSize);
            close(fd);
        }
    }
    void init(const fs::path &fname)
    {
        struct stat sb;
        // TODO AS does open assume utf8?
        fd = open(path_to_string(fname).c_str(), O_RDONLY);
        if (!fd)
        {
            isMapped = false;
            return;
        }
        fstat(fd, &sb);
        data = mmap(nullptr, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED)
        {
            isMapped = false;
            close(fd);
            data = nullptr;
            dataSize = 0;
            return;
        }
        isMapped = true;
        dataSize = sb.st_size;
    }
    void *data = nullptr;
    size_t dataSize = 0;
    bool isMapped = false;

    int fd = 0;
};

posixImpl *as(FileMapView::Impl *imp) { return reinterpret_cast<posixImpl *>(imp); }
#endif

FileMapView::FileMapView(const fs::path &fname)
{
#if WIN32
    impl = std::make_unique<WinImpl>(fname);
#else
    impl = std::make_unique<posixImpl>(fname);
#endif
}

FileMapView::~FileMapView() {}

void *FileMapView::data() { return as(impl.get())->data; }

size_t FileMapView::dataSize() { return as(impl.get())->dataSize; }

bool FileMapView::isMapped() { return as(impl.get())->isMapped; }

} // namespace scxt