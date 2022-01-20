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
#include <catch2/catch2.hpp>
#include "util/scxtstring.h"
#include <cstring>

TEST_CASE("vtString", "[vt]")
{
    SECTION("strncpy_0term")
    {
        char s[256], dest[256];
        s[0] = 0;
        strncpy_0term(dest, s, 256);
        REQUIRE(strcmp(s, dest) == 0);

        strcpy(s, "I Am VT String");

        strncpy_0term(dest, s, 256);
        REQUIRE(strcmp(s, dest) == 0);

        INFO("Null Terminate Dest no matter what");
        for (int i = 0; i < 256; ++i)
            s[i] = 1;
        strncpy_0term(dest, s, 256);
        REQUIRE(strlen(dest) == 255);
    }
}
