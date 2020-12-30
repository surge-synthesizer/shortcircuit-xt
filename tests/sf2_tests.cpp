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

#include "globals.h"
#include "sampler.h"
#include "riff_memfile.h"
#include <iostream>
#include <map>


TEST_CASE( "Simple SF2 Load", "[formats]" )
{
    SECTION( "Simple Load" )
    {
        auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
        REQUIRE( sc3 );

        sc3->set_samplerate(48000);
#if WINDOWS
        std::cout << "Loading harpsi.sf2" << std::endl;
        REQUIRE( sc3->load_file("resources\\test_samples\\harpsi.sf2") );
#endif
    }

}