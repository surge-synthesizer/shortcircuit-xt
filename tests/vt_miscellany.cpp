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
#include <catch2/catch2.hpp>
#include "vt_util/vt_string.h"
#include <cstring>

TEST_CASE( "vtString", "[vt]" )
{
    SECTION( "vtCopyString" )
    {
        char s[256], dest[256];
        s[0] = 0;
        vtCopyString(dest, s, 256 );
        REQUIRE( strcmp( s, dest ) == 0 );

        strcpy( s, "I Am VT String" );

        vtCopyString(dest, s, 256 );
        REQUIRE( strcmp( s, dest ) == 0 );

        INFO( "Null Terminate Dest no matter what" );
        for( int i=0; i<256; ++i )
            s[i] = 1;
        vtCopyString( dest, s, 256 );
        REQUIRE( strlen(dest) == 255 );
    }

    SECTION( "vtCopyStringW" )
    {
        wchar_t s[256], dest[256];
        s[0] = 0;
        vtCopyStringW(dest, s, 256 );
        REQUIRE( wcscmp( s, dest ) == 0 );

        wcscpy( s, L"I Am VT String" );

        vtCopyStringW(dest, s, 256 );
        REQUIRE( wcscmp( s, dest ) == 0 );

        INFO( "Null Terminate Dest no matter what" );
        for( int i=0; i<256; ++i )
            s[i] = 1;
        vtCopyStringW( dest, s, 256 );
        REQUIRE( wcslen(dest) == 255 );
    }

    SECTION( "vtToAndFrom" )
    {
        char c[256];
        wchar_t w[256];

        vtWStringToString(c, L"Hi There", 256 );
        REQUIRE( strcmp(c,"Hi There") == 0 );

        vtStringToWString(w, "Howdy Pardner", 256 );
        REQUIRE( wcscmp(w, L"Howdy Pardner" ) == 0 );
    }
}
