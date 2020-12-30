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

#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include <catch2/catch2.hpp>
#include <filesystem/import.h>

#include "globals.h"
#include "infrastructure/sc3_mmio.h"

TEST_CASE( "MMIO Layer", "[io]" )
{
    auto fccstr = [](uint32_t tag)
    {
        char fcc[5];
        for (auto i = 0; i < 4; ++i)
        {
            fcc[i] = (char)(tag & 0xFF);
            tag = tag >> 8;
        }
        fcc[4] = 0;
        return std::string(fcc);
    };

    SECTION("Open and Close" )
    {
        auto h = mmioOpen("resources/test_samples/WavStereo48k.wav",
                          NULL,
                          MMIO_READ | MMIO_ALLOCBUF);
        REQUIRE( h );
        REQUIRE( mmioClose( h, 0 ) == 0 );
    }
    SECTION( "OpenW and Close" )
    {
        auto h = mmioOpenW(L"resources/test_samples/WavStereo48k.wav",
                           NULL,
                           MMIO_READ | MMIO_ALLOCBUF);
        REQUIRE( h );
        REQUIRE( mmioClose( h, 0 ) == 0 );
    }
    SECTION( "mmioFourCC" )
    {
        REQUIRE( mmioFOURCC( 0, 0, 0, 0 ) == 0 );
        REQUIRE( mmioFOURCC( 'R', 'I', 'F', 'F' ) == 0x46464952);
        REQUIRE( mmioFOURCC( 'd', 'a', 't', 'a' ) == 0x61746164);
    }
#if WINDOWS
    SECTION( "Wave File Header Traversal" )
    {
        auto h =
            mmioOpen("resources/test_samples/WavStereo48k.wav", NULL, MMIO_READ | MMIO_ALLOCBUF);
        REQUIRE(h);

        MMCKINFO mmckinfoParent; /* for the Group Header */
        mmckinfoParent.fccType = 0;
        REQUIRE(mmioDescend(h, &mmckinfoParent, 0, MMIO_FINDRIFF) == 0);
        REQUIRE(fccstr(mmckinfoParent.ckid) == "RIFF");
        REQUIRE(fccstr(mmckinfoParent.fccType) == "WAVE");
        REQUIRE(mmioSeek(h, 0, SEEK_CUR) == 12);

        /*
         * fmt  2835688
         * data 2778
         * LGWV 365
         * cue  28
         */

        std::map<std::string,int> knownSizes;
        knownSizes["fmt "] = 16;
        knownSizes["data"] = 2835688;
        knownSizes["LGWV"] = 2778;
        knownSizes["cue "] = 28;

        for( auto p : knownSizes )
        {
            MMCKINFO mmckinfoSub;
            INFO( "Reading chunk " << p.first );
            mmckinfoSub.ckid = mmioFOURCC( p.first[0], p.first[1], p.first[2], p.first[3] );
            REQUIRE(mmioDescend(h, &mmckinfoSub, &mmckinfoParent, MMIO_FINDCHUNK) == 0);
            REQUIRE(p.second == mmckinfoSub.cksize);
            REQUIRE(mmioAscend(h, &mmckinfoSub, 0 ) == 0);
            REQUIRE(mmioSeek(h,12,SEEK_SET) == 12);
        }
        REQUIRE( mmioClose( h, 0 ) == 0 );
    }
    SECTION( "Open on an SF2" )
    {
        auto h = mmioOpen( "resources/test_samples/harpsi.sf2",
                          NULL,
                          MMIO_READ | MMIO_ALLOCBUF );
        REQUIRE( h );

        MMCKINFO mmckinfoParent; /* for the Group Header */
        mmckinfoParent.fccType = mmioFOURCC('s', 'f', 'b', 'k');

        REQUIRE( mmioDescend( h, &mmckinfoParent, 0, MMIO_FINDRIFF ) == 0 );
        REQUIRE( mmioSeek(h, 0, SEEK_CUR ) == 12 ); // skipped the header


    }
#endif
}