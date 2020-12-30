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
#include <catch2/catch2.hpp>
#include <filesystem/import.h>

#include "globals.h"
#include "riff_memfile.h"
#include <iostream>
#include <map>

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
                std::cout << "LIST TAG is " << rl << std::endl;
            }

            rmf.RIFFDescend();
            while (rmf.RIFFPeekChunk(&tag, &chunksize))
            {
                char rfD[5];
                rmf.tagToFourCCStr(tag, rfD);
                std::cout << rfD << std::endl;
                if( std::string(rfD) == "LIST" )
                {
                    rmf.RIFFDescend();
                    while (rmf.RIFFPeekChunk(&tag, &chunksize))
                    {
                        rmf.tagToFourCCStr(tag, rfD);
                        std::cout << "  - " << rfD << std::endl;
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