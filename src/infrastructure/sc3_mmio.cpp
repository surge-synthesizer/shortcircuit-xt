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
#include "sc3_mmio.h"

#if WINDOWS
HMMIO mmioOpenFromPath(const fs::path &fn, void *, int mode)
{
    auto wideFn = fn.generic_wstring();
    return mmioOpenW(const_cast<LPWSTR>(wideFn.c_str()), nullptr, mode);
}
#else

#include <fstream>
#include <string>
#include <codecvt>
#include <locale>

HMMIO mmioOpenSTR(std::ifstream &ifs, int mode)
{
    if (!ifs.is_open())
    {
        return nullptr;
    }

    ifs.seekg(0, std::ios::end);
    auto length = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    auto data = new char[length];
    ifs.read(data, length);
    ifs.close();

    auto res = std::make_shared<sc3mmio_hand>();
    res->is_open = true;
    res->rawData = data;
    res->rawDataSize = length;
    res->memFile = SC3::Memfile::RIFFMemFile(
        res->rawData, res->rawDataSize); // Remember it doesn't take ownership

    return res;
}

HMMIO mmioOpenFromPath(const fs::path &fn, void *, int mode)
{
    auto ifs = std::ifstream(fn, std::ios::binary);
    return mmioOpenSTR(ifs, mode);
}
HMMIO mmioOpenW(const wchar_t *fn, void *, int mode)
{
#if MAC || LINUX
    auto converter = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{};
    auto fnut8 = converter.to_bytes(fn);

    auto ifs = std::ifstream(fnut8, std::ios::binary);
#else
    auto ifs = std::ifstream(fn, std::ios::binary);
#endif

    return mmioOpenSTR(ifs, mode);
}

int mmioDescend(HMMIO h, MMCKINFO *target, MMCKINFO *parent, int type)
{
    int tag;
    size_t ds;
    bool has;
    int ltag;

    switch (type)
    {
    case MMIO_FINDRIFF:
    {
        /*
         * A naieve implementation but assume that only first time through is a RIFF
         */
        if (h->memFile.RIFFPeekChunk(&tag, &ds, &has, &ltag))
        {
            if (vt_write_int32BE(tag) == mmioFOURCC('R', 'I', 'F', 'F'))
            {
                h->memFile.RIFFDescend();

                target->ckid = vt_write_int32BE(tag);
                target->fccType = vt_write_int32BE(ltag);
                target->cksize = ds;

                return 0;
            }
        }
        break;
    }
    case MMIO_FINDCHUNK:
    {
        /*
         * I ignore parent here. That's bad but it meets the limited pattern we have
         */
        auto lookFor = target->ckid;
        int ds;
        if (h->memFile.iff_descend(lookFor, &ds))
        {
            target->cksize = ds;
            return 0;
        }
        break;
    }
    case MMIO_FINDLIST:
    {
        size_t ds;
        // I am unsure why I need to do a BE switch here but don't want to change
        // the BE in the RIFFDescendSearch code.
        if (h->memFile.RIFFDescendSearch(vt_write_int32BE(target->fccType), &ds))
        {
            target->cksize = ds;
            return 0;
        }
        break;
    }
    }

    return 1;
}

int mmioRead(HMMIO h, HPSTR b, int s)
{
    if (!h->memFile.Read(b, s))
        return 0;
    return s;
}
int mmioSeek(HMMIO h, int t, int flag)
{
    int res = -1;
    switch (flag)
    {
    case SEEK_CUR:
        res = h->memFile.SeekI(t, SC3::Memfile::mf_FromCurrent);
        break;
    case SEEK_SET:
        res = h->memFile.SeekI(t, SC3::Memfile::mf_FromStart);
        break;
    case SEEK_END:
        res = h->memFile.SeekI(t, SC3::Memfile::mf_FromEnd);
        break;
    }
    return res;
}

#endif
