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
#include <cctype>
#include <stdio.h>

#include <catch2/catch2.hpp>
#include "infrastructure/import_fs.h"

#include "globals.h"
#include "riff_memfile.h"
#include "infrastructure/file_map_view.h"

TEST_CASE("RIFF_MemFile", "[io]")
{
    SECTION("Read a WAV with an RMF")
    {
        auto p = string_to_path("resources/test_samples/WavStereo48k.wav");
        auto ifs = std::ifstream(p, std::ios::binary);

        REQUIRE(ifs.is_open());

        ifs.seekg(0, std::ios::end);
        auto length = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        auto data = new char[length];
        ifs.read(data, length);
        ifs.close();

        SC3::Memfile::RIFFMemFile rmf(data, length);

        size_t chunksize;
        int tag, LISTtag;
        bool IsLIST;

        std::map<std::string,int> knownSizes;
        knownSizes["fmt "] = 16;
        knownSizes["data"] = 2835688;
        knownSizes["LGWV"] = 2778;
        knownSizes["cue "] = 28;

        bool foundData = false;
        while (rmf.RIFFPeekChunk(&tag, &chunksize))
        {
            char rf[5];
            rmf.tagToFourCCStr(tag, rf);
            REQUIRE(strcmp(rf, "RIFF") == 0);
            REQUIRE(((size_t)length - chunksize) == 8);
            rmf.RIFFDescend();
            while (rmf.RIFFPeekChunk(&tag, &chunksize))
            {
                char rfD[5];
                rmf.tagToFourCCStr(tag, rfD);
                if( knownSizes.find(rfD) != knownSizes.end() )
                    REQUIRE( chunksize == knownSizes[rfD]);
                if (strcmp(rfD, "data") == 0)
                    foundData = true;
                rmf.RIFFSkipChunk();
            }
        }
        REQUIRE(foundData);
        delete[] data;
    }

    SECTION( "Read an SF2 with an RMF" )
    {
        auto p = string_to_path("resources/test_samples/harpsi.sf2");
        auto ifs = std::ifstream(p, std::ios::binary);

        REQUIRE(ifs.is_open());

        ifs.seekg(0, std::ios::end);
        auto length = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        auto data = new char[length];
        ifs.read(data, length);
        ifs.close();

        SC3::Memfile::RIFFMemFile rmf(data, length);

        size_t chunksize;
        int tag, listTag;
        bool isList;

        while (rmf.RIFFPeekChunk(&tag, &chunksize, &isList, &listTag))
        {
            char rf[5];
            rmf.tagToFourCCStr(tag, rf);
            REQUIRE(strcmp(rf, "RIFF") == 0);
            REQUIRE(((size_t)length - chunksize) == 8);

            if( isList )
            {
                char rl[5];
                rmf.tagToFourCCStr(listTag, rl );
            }

            rmf.RIFFDescend();
            while (rmf.RIFFPeekChunk(&tag, &chunksize))
            {
                char rfD[5];
                rmf.tagToFourCCStr(tag, rfD);
                if( std::string(rfD) == "LIST" )
                {
                    rmf.RIFFDescend();
                    while (rmf.RIFFPeekChunk(&tag, &chunksize))
                    {
                        rmf.tagToFourCCStr(tag, rfD);
                        rmf.RIFFSkipChunk();
                    }
                    rmf.RIFFAscend();
                }
            }
        }
        delete[] data;
    }

    SECTION( "riff descend search test" )
    {
        auto p = string_to_path("resources/test_samples/harpsi.sf2");
        auto ifs = std::ifstream(p, std::ios::binary);

        REQUIRE(ifs.is_open());

        ifs.seekg(0, std::ios::end);
        auto length = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        auto data = new char[length];
        ifs.read(data, length);
        ifs.close();

        SC3::Memfile::RIFFMemFile rmf(data, length);

        size_t datasize;
        REQUIRE( rmf.riff_descend_RIFF_or_LIST('sfbk', &datasize));

        delete[] data;
    }
}

TEST_CASE("File Mapper", "[io]")
{
    int testSize = 52310;
    int testSkip = 13;
#define MAKE_TEST_DATA_FILES 0
#if MAKE_TEST_DATA_FILES
    auto f = fopen("resources/test_samples/not_audio.bin", "wb");
    REQUIRE(f);
    if (f)
    {
        unsigned char d = 0;
        for (auto i = 0; i < testSize; ++i)
        {
            fwrite(&d, 1, 1, f);
            d += testSkip;
        }
    }
    fclose(f);
#endif

    SECTION("Mapper reads Test Data")
    {
        auto mapper = std::make_unique<SC3::FileMapView>("resources/test_samples/not_audio.bin");
        REQUIRE(mapper);
        REQUIRE(mapper->isMapped());

        auto data = (const unsigned char *)mapper->data();
        REQUIRE(data);
        auto dataSize = mapper->dataSize();
        REQUIRE(dataSize == testSize);

        unsigned char d = 0;
        for (int i = 0; i < dataSize; ++i)
        {
            INFO("Testing at position " << i)
            REQUIRE(data[i] == d);
            d += testSkip;
        }
    }
}