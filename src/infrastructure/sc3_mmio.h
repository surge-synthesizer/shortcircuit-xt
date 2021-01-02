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

#ifndef SHORTCIRCUIT_SC3_MMIO_H
#define SHORTCIRCUIT_SC3_MMIO_H

#if WINDOWS
#include <windows.h>
#include <mmiscapi.h>
#else
#include <windows_compat.h>
#include <iostream>
#include <memory>

#include "infrastructure/logfile.h"
#include "riff_memfile.h"

struct sc3mmio_hand {
    bool is_open = false;
    char *rawData = nullptr;
    size_t rawDataSize = 0;
    SC3::Memfile::RIFFMemFile memFile;

    void close()
    {
        if( is_open )
        {
            is_open = false;
            delete[] rawData;
            rawData = nullptr;
            rawDataSize = 0;
        }
    }

    sc3mmio_hand() = default;
    ~sc3mmio_hand() {
        if( is_open )
        {
            SC3::Log::logos() << "Leaked an sc3mmio_hand: Destroyed a non-closed handle" << std::endl;
            if( rawData ) delete[] rawData;
        }
    }
};
typedef std::shared_ptr<sc3mmio_hand> HMMIO;

struct MMCKINFO
{
    int fccType;
    int ckid;
    int cksize;
};

#define MMIO_READ 0
#define MMIO_ALLOCBUF 0
#define MMIO_FINDRIFF 0
#define MMIO_FINDCHUNK 0
#define MMIO_FINDLIST 0
#define LPMMCKINFO MMCKINFO *

typedef int LRESULT;

typedef const char* LPSTR;
typedef const wchar_t* LPWSTR;
HMMIO mmioOpen(LPSTR fn, void *defacto_unused, int mode);
HMMIO mmioOpenW(LPWSTR fn, void *defacto_unused, int mode);

inline uint32_t mmioFOURCC(char a, char b, char c, char d) {
    uint32_t res = ( a ) | ( b << 8 ) | ( c << 16 ) | (d << 24 );
    return res;
}
inline int mmioClose(HMMIO h, uint32_t) {
    if( h && h->is_open ) h->close();
    return 0;
}

int mmioDescend(HMMIO handle, MMCKINFO *target, MMCKINFO *parent, int style);
int mmioSeek(HMMIO a, int s, int f);
inline int mmioAscend(HMMIO a, MMCKINFO *b, int) { return 0; }
int mmioRead(HMMIO a, HPSTR b, int);

#endif

#endif // SHORTCIRCUIT_SC3_MMIO_H
