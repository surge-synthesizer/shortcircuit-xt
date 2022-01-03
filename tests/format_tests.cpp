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

#include "test_main.h"

#include <iostream>
#include <map>
#include <vector>

#include "globals.h"
#include "sampler.h"

TEST_CASE("Simple SF2 Load", "[formats]")
{
    SECTION("Simple Load")
    {
        auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr, gLogger);
        REQUIRE(sc3);

        sc3->set_samplerate(48000);
#if WINDOWS
        REQUIRE(sc3->load_file("resources\\test_samples\\harpsi.sf2"));
#else
        REQUIRE(sc3->load_file(string_to_path("resources/test_samples/harpsi.sf2")));
#endif

        auto notes = {60, 64, 66};
        std::vector<float> rmses;
        for (auto n : notes)
        {
            double rms = 0;
            for (int i = 0; i < 300; ++i)
            {
                if (i == 60)
                    sc3->PlayNote(0, n, 120);
                if (i == 140)
                    sc3->ReleaseNote(0, n, 0);

                sc3->process_audio();
                for (int k = 0; k < block_size; ++k)
                {
                    rms += sc3->output[0][k] * sc3->output[0][k] +
                           sc3->output[1][k] * sc3->output[1][k];
                }
            }
            rms = sqrt(rms);
            rmses.push_back(rms);
        }
        REQUIRE(rmses[0] == Approx(19.86253).margin(1e-3));
        REQUIRE(rmses[1] == Approx(21.01420).margin(1e-3));
        REQUIRE(rmses[2] == Approx(20.87521).margin(1e-3));
    }
}

TEST_CASE("Simple WAV Load", "[formats]")
{
    SECTION("Simple Load")
    {
        auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
        REQUIRE(sc3);

        sc3->set_samplerate(48000);
#if WINDOWS
        REQUIRE(sc3->load_file("resources\\test_samples\\BadPluckSample.wav"));
#else
        REQUIRE(sc3->load_file(string_to_path("resources/test_samples/BadPluckSample.wav")));
#endif

        double rms = 0;
        int n = 36;
        for (int i = 0; i < 200; ++i)
        {
            if (i == 60)
                sc3->PlayNote(0, n, 120);
            if (i == 140)
                sc3->ReleaseNote(0, n, 0);

            sc3->process_audio();
            for (int k = 0; k < block_size; ++k)
            {
                rms +=
                    sc3->output[0][k] * sc3->output[0][k] + sc3->output[1][k] * sc3->output[1][k];
            }
        }
        rms = sqrt(rms);
        REQUIRE(rms == Approx(5.9624592896).margin(1e-4));
    }

    SECTION("Wide string filename load")
    {
        auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
        REQUIRE(sc3);

        sc3->set_samplerate(48000);
        REQUIRE(sc3->load_file(
            "resources/test_samples/\xe8\x81\xb2\xe9\x9f\xb3\xe4\xb8\x8d\xe5\xa5\xbd.wav"));
    }
}

TEST_CASE("Simple SFZ+WAV Load", "[formats]")
{
    SECTION("Load SFZ - Single Sample, simplest case")
    {
        auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
        REQUIRE(sc3);

        sc3->set_samplerate(48000);
        REQUIRE(sc3->load_file("resources/test_samples/malicex_sfz/miniguitar_octavelike.sfz"));

        // play single note
        double rms = 0;
        int n = 60;
        for (int i = 0; i < 200; ++i)
        {
            if (i == 60)
                sc3->PlayNote(0, n, 120);
            if (i == 140)
                sc3->ReleaseNote(0, n, 0);

            sc3->process_audio();
            for (int k = 0; k < block_size; ++k)
            {
                rms +=
                    sc3->output[0][k] * sc3->output[0][k] + sc3->output[1][k] * sc3->output[1][k];
            }
        }
        rms = sqrt(rms);
        REQUIRE(rms == Approx(35.68221).margin(1e-4));
    }

    SECTION("Load SFZ - Single-velocity Drumkit")
    {
        auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
        REQUIRE(sc3);

        sc3->set_samplerate(48000);
        REQUIRE(sc3->load_file("resources/test_samples/malicex_sfz/YM-FM_Font FM Drums.sfz"));

        double rms = 0;
        int n_lo = 35, n_hi = 60; // play several notes at once
        for (int i = 0; i < 2000; ++i)
        {
            if (i == 60)
                for (int n = n_lo; n <= n_hi; ++n)
                    sc3->PlayNote(0, n, 127);

            if (i == 1400)
                for (int n = n_lo; n <= n_hi; ++n)
                    sc3->ReleaseNote(0, n, 0);

            sc3->process_audio();
            for (int k = 0; k < block_size; ++k)
            {
                rms +=
                    sc3->output[0][k] * sc3->output[0][k] + sc3->output[1][k] * sc3->output[1][k];
            }
        }
        rms = sqrt(rms);
        // TODO this one fluctuates a lot, also risk of segfault due to looping bug
        REQUIRE(rms == Approx(232.55386).margin(1e-4));
    }

    SECTION("Load SFZ - Multi-velocity zones")
    {
        auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
        REQUIRE(sc3);

        sc3->set_samplerate(48000);
        REQUIRE(sc3->load_file("resources/test_samples/malicex_sfz/YM-FM_Font Music Box.sfz"));

        // play notes (same key, different velocities) at once.
        double rms = 0;
        int notes[] = {24};
        int vels[] = {40, 41, 79, 80, 89, 90, 100, 127};

        for (int i = 0; i < 2000; ++i)
        {
            if (i == 60)
                for (auto n : notes)
                    for (auto v : vels)
                        sc3->PlayNote(0, n, v);

            if (i == 1400)
                for (auto n : notes)
                    for (auto v : vels) // just getting the counts
                        sc3->ReleaseNote(0, n, 0);

            sc3->process_audio();
            for (int k = 0; k < block_size; ++k)
            {
                rms +=
                    sc3->output[0][k] * sc3->output[0][k] + sc3->output[1][k] * sc3->output[1][k];
            }
        }
        rms = sqrt(rms);
        REQUIRE(rms == Approx(588.04064).margin(1e-4));
    }
}

TEST_CASE("Load two SF2s", "[formats]")
{
    SECTION("TwoSF2s")
    {
        auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
        REQUIRE(sc3);

        sc3->set_samplerate(48000);
#if WINDOWS
        REQUIRE(sc3->load_file("resources\\test_samples\\harpsi.sf2"));
        REQUIRE(sc3->load_file("resources\\test_samples\\Crysrhod.sf2"));
#else
        REQUIRE(sc3->load_file(string_to_path("resources/test_samples/harpsi.sf2")));
        REQUIRE(sc3->load_file(string_to_path("resources/test_samples/Crysrhod.sf2")));
#endif
    }
}

TEST_CASE("Akai S6k patch load", "[formats]")
{
    gTestLevel = SC3::Log::Level::Debug;

    SECTION("Simple Load")
    {
        auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr, gLogger);
        REQUIRE(sc3);

        sc3->set_samplerate(48000);
#if WINDOWS
        REQUIRE(sc3->load_file("resources\\test_samples\\akai_s6k\\POWER SECT S.AKP"));
#else
        REQUIRE(sc3->load_file(string_to_path("resources/test_samples/akai_s6k/POWER SECT S.AKP")));
#endif
        /*
                double rms = 0;
                int n = 36;
                for (int i = 0; i < 100; ++i)
                {
                    if (i == 30)
                        sc3->PlayNote(0, n, 120);
                    if (i == 70)
                        sc3->ReleaseNote(0, n, 0);

                    sc3->process_audio();
                    for (int k = 0; k < block_size; ++k)
                    {
                        rms +=
                            sc3->output[0][k] * sc3->output[0][k] + sc3->output[1][k] *
           sc3->output[1][k];
                    }
                }
                rms = sqrt(rms);
                REQUIRE(rms == Approx(6.0266351586).margin(1e-4));
                */
    }

    gTestLevel = SC3::Log::Level::None;
}
