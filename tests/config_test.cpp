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

#include "test_main.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <iostream>
#include <map>
#include <vector>

#include "configuration.h"
#include "sampler.h"

TEST_CASE("Decode Path", "[config]")
{
    std::string nameOnly, ext;
    int progid, sampleid;
    fs::path out, pathOnly;
#ifdef _WIN32
    SECTION("Windows basic")
    {
        fs::path p("c:\\my\\path\\filename.WAV>100|200");

        decode_path(p, &out, &ext, &nameOnly, &pathOnly, &progid, &sampleid);
        REQUIRE(out.compare("c:\\my\\path\\filename.WAV") == 0);
        REQUIRE(ext.compare("wav") == 0);
        REQUIRE(nameOnly.compare("filename") == 0);
        REQUIRE(pathOnly.compare("c:\\my\\path") == 0);
        REQUIRE(progid == 100);
        REQUIRE(sampleid == 200);
    }
    SECTION("Windows Sample id only")
    {
        fs::path p("c:\\my\\path\\filename.WAV|200");

        decode_path(p, &out, &ext, &nameOnly, &pathOnly, &progid, &sampleid);
        REQUIRE(out.compare("c:\\my\\path\\filename.WAV") == 0);
        REQUIRE(ext.compare("wav") == 0);
        REQUIRE(nameOnly.compare("filename") == 0);
        REQUIRE(pathOnly.compare("c:\\my\\path") == 0);
        REQUIRE(progid == -1);
        REQUIRE(sampleid == 200);
    }
    SECTION("Windows Prog id only")
    {
        fs::path p("c:\\my\\path\\filename.WAV>100");

        decode_path(p, &out, &ext, &nameOnly, &pathOnly, &progid, &sampleid);
        REQUIRE(out.compare("c:\\my\\path\\filename.WAV") == 0);
        REQUIRE(ext.compare("wav") == 0);
        REQUIRE(nameOnly.compare("filename") == 0);
        REQUIRE(pathOnly.compare("c:\\my\\path") == 0);
        REQUIRE(progid == 100);
        REQUIRE(sampleid == -1);
    }
    SECTION("Windows Path only with progid")
    {
        fs::path p("c:\\my\\path\\>100");

        decode_path(p, &out, &ext, &nameOnly, &pathOnly, &progid, &sampleid);
        REQUIRE(out.compare("c:\\my\\path\\") == 0);
        REQUIRE(ext.empty());
        REQUIRE(nameOnly.empty());
        REQUIRE(pathOnly.compare("c:\\my\\path") == 0);
        REQUIRE(progid == 100);
        REQUIRE(sampleid == -1);
    }
    SECTION("Windows Path only with sampleid")
    {
        fs::path p("c:\\my\\path\\|200");

        decode_path(p, &out, &ext, &nameOnly, &pathOnly, &progid, &sampleid);
        REQUIRE(out.compare("c:\\my\\path\\") == 0);
        REQUIRE(ext.empty());
        REQUIRE(nameOnly.empty());
        REQUIRE(pathOnly.compare("c:\\my\\path") == 0);
        REQUIRE(progid == -1);
        REQUIRE(sampleid == 200);
    }
    SECTION("Windows wide name")
    {
        fs::path p(L"resources\\test_samples\\\u8072\u97f3\u4e0d\u597d.wav>100|200");
        char name[256];
        WideCharToMultiByte(CP_UTF8, 0, L"\u8072\u97f3\u4e0d\u597d", -1, name, 256, 0, 0);
        decode_path(p, &out, &ext, &nameOnly, &pathOnly, &progid, &sampleid);
        REQUIRE(out.compare(L"resources\\test_samples\\\u8072\u97f3\u4e0d\u597d.wav") == 0);
        REQUIRE(ext.compare("wav") == 0);
        REQUIRE(nameOnly.compare(name) == 0);
        REQUIRE(pathOnly.compare("resources\\test_samples") == 0);
        REQUIRE(progid == 100);
        REQUIRE(sampleid == 200);
    }
    
#else
    SECTION("Unix/mac basic") 
    {
        fs::path p("/my/path/filename.WAV>100|200");

        decode_path(p, &out, &ext, &nameOnly, &pathOnly, &progid, &sampleid);
        REQUIRE(out.string().compare(std::string("/my/path/filename.WAV")) == 0);
        REQUIRE(ext.compare(std::string("wav")) == 0);
        REQUIRE(nameOnly.compare(std::string("filename")) == 0);
        REQUIRE(pathOnly.string().compare(std::string("/my/path")) == 0);
        REQUIRE(progid == 100);
        REQUIRE(sampleid == 200);

    }        

    SECTION("Unix/mac path only")
    {        
        fs::path p("/my/path/>100");

        decode_path(p, &out, &ext, &nameOnly, &pathOnly, &progid, &sampleid);
        REQUIRE(out.string().compare(std::string("/my/path/")) == 0);
        REQUIRE(ext.empty());
        REQUIRE(nameOnly.empty());
        REQUIRE(pathOnly.string().compare(std::string("/my/path")) == 0);
        REQUIRE(progid == 100);
        REQUIRE(sampleid == -1);
    }        
 #endif
}

TEST_CASE("Build Path", "[config]")
{
    std::string nameOnly, ext;
    fs::path out, pathOnly;
#ifdef _WIN32
    SECTION("Windows build")
    {
        fs::path p("c:\\my\\path");
        auto out=build_path(p, "filename", "WAV");
        REQUIRE(out.compare("c:\\my\\path\\filename.WAV") == 0);        
    }

    SECTION("Windows wide") 
    { 
        char name[256];
        WideCharToMultiByte(CP_UTF8, 0, L"\u8072\u97f3\u4e0d\u597d", -1, name, 256, 0, 0);
        fs::path p("resources\\test_samples");        
        auto out = build_path(p, name, "wav");
        auto wideFn = out.generic_wstring();
        REQUIRE(out.compare(L"resources\\test_samples\\\u8072\u97f3\u4e0d\u597d.wav") == 0); 

    }
#endif
}

TEST_CASE("Resolve Path", "[config]") {
    SC3::Log::StreamLogger sl(gLogger);
    configuration c(sl);

    SECTION("relative") {
        c.set_relative_path("/foo");
        auto p=c.resolve_path(string_to_path("<relative>/bar"));
        REQUIRE(p.string()==std::string("/foo/bar"));
    }

// unix only is case sensitive with paths
#if !defined(_WIN32) && !defined(__APPLE__)
    SECTION("Unix case sensitivity") {
        auto p=c.resolve_path(string_to_path("resources/test_samples/akai_s6k/POWER SECT S.akp"));
        REQUIRE(p.string()==std::string("resources/test_samples/akai_s6k/POWER SECT S.AKP"));
    }
#endif
}
