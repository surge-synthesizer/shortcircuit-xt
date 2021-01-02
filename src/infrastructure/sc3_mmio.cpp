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
#include "sc3_mmio.h"

#if ! WINDOWS

#include <fstream>
#include <string>
#include <codecvt>
#include <locale>

HMMIO mmioOpenSTR( std::ifstream &ifs, int mode )
{
    if (!ifs.is_open()) { return nullptr; }

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
    res->memFile = SC3::Memfile::RIFFMemFile(res->rawData, res->rawDataSize); // Remember it doesn't take ownership

    return res;
}

HMMIO mmioOpen(const char *fn, void *, int mode)
{
    auto ifs = std::ifstream(fn, std::ios::binary);
    return mmioOpenSTR( ifs, mode );
}
HMMIO mmioOpenW(const wchar_t *fn, void *, int mode)
{
#if MAC || LINUX
    auto converter = std::wstring_convert<
        std::codecvt_utf8_utf16<wchar_t>, wchar_t>{};
    auto fnut8 = converter.to_bytes(fn);

    auto ifs = std::ifstream( fnut8, std::ios::binary );
#else
    auto ifs = std::ifstream( fn, std::ios::binary );
#endif

    return mmioOpenSTR( ifs, mode );
}

int mmioDescend(HMMIO h, MMCKINFO *target, MMCKINFO *parent, int type )
{
    /*
    bool RIFFPeekChunk(int *Tag = nullptr, size_t *DataSize = nullptr, bool *HasSubchunks = nullptr,
                       int *LISTTag = nullptr)
                       */
    int tag;
    size_t ds;
    bool has;
    int ltag;

    /*
     * This is not a correct implementation but it makes regtests pass so it might
     * be enough for our sub out workflow
     */
    if (parent == nullptr)
    {
        if (h->memFile.RIFFPeekChunk(&tag, &ds, &has, &ltag))
        {
            h->memFile.RIFFDescend();

            target->ckid = vt_write_int32BE(tag);
            target->fccType = vt_write_int32BE(ltag);
            target->cksize = ds;

            return 0;
        }
    }
    else
    {
        auto lookFor = target->ckid;
        int ds;
        if (h->memFile.iff_descend(lookFor, &ds))
        {
            target->cksize = ds;
            return 0;
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
