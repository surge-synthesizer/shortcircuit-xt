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

#include <iostream>
#include <map>
#include <vector>

#include "sampler.h"
#include "filesystem/import.h"
#include "sample.h"

TEST_CASE("Zones from 3 Wavs", "[zones]")
{
#if WINDOWS
    auto pbolpc = fs::path("resources\\test_samples\\OLPC");
#else
    auto pbolpc = string_to_path("resources/test_samples/OLPC");
#endif
    auto pbassDrum = pbolpc / string_to_path("drum-bass-lo-1.wav");
    auto pSnare = pbolpc / string_to_path("drum-snare-tap.wav");
    auto pHat = pbolpc / string_to_path("cymbal-hihat-foot-2.wav");

    auto kit = {pbassDrum, pSnare, pHat};
    std::vector<int> dsize = {169796, 127860, 35856};

    SECTION("Load Three Waves get three zones")
    {
        auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
        REQUIRE(sc3);

        int z = 0;
        for (auto item : kit)
        {
            int newG, newZ;
            bool isG;
            INFO(path_to_string(item));
            REQUIRE(sc3->load_file(item, &newG, &newZ, &isG));
            REQUIRE(!isG);
            REQUIRE(newZ == z++);
        }

        INFO("Checking zone existence is correct");
        for (int i = 0; i < max_zones; ++i)
            REQUIRE(sc3->zone_exist(i) == (i < z));
    }

    SECTION("Auto Zones are mapped Reasonably")
    {
        auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
        REQUIRE(sc3);

        int z = 0;
        for (auto item : kit)
        {
            int newG, newZ;
            INFO(path_to_string(item));
            REQUIRE(sc3->load_file(item, &newG, &newZ));
            z++;
        }

        for (auto i = 0; i < max_zones; ++i)
        {
            if (sc3->zone_exist(i))
            {
                REQUIRE(i < z);
                auto zone = sc3->zones[i];
                REQUIRE(zone.key_low == 36 + i);
                REQUIRE(zone.key_high == 36 + i);
                REQUIRE(zone.sample_id == i);
                // since we are a fresh instance. This is not generally true

                auto sample = sc3->samples[zone.sample_id];
                REQUIRE(sample->channels == 1);
                REQUIRE(sample->GetDataSize() == dsize[i]);
            }
            else
            {
                REQUIRE(i >= z);
            }
        }
    }

    for (int i = 0; i < 30; ++i)
        DYNAMIC_SECTION("Playback Three AutoZones " << i)
        {
            auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
            REQUIRE(sc3);
            sc3->set_samplerate(48000);

            auto oneSecInBlocks = (int)(48000 / block_size);

            for (auto item : kit)
            {
                int newG, newZ;
                REQUIRE(sc3->load_file(item, &newG, &newZ));
            }

            for (auto i = 0; i < max_zones; ++i)
            {
                if (sc3->zone_exist(i))
                {
                    auto zone = sc3->zones[i];
                    auto sample = sc3->samples[zone.sample_id];

                    REQUIRE(zone.key_low == 36 + i);
                    REQUIRE(zone.key_high == 36 + i);
                    REQUIRE(zone.sample_id == i);
                    // since we are a fresh instance. This is not generally true

                    REQUIRE(zone.sample_start == 0);
                    REQUIRE(zone.sample_stop == dsize[i] / 2);
                    REQUIRE(zone.playmode == pm_forwardRIFF);

                    REQUIRE(sample->channels == 1);
                    REQUIRE(sample->GetDataSize() == dsize[i]);
                    REQUIRE(sample->sample_rate == 44100);
                }
            }

            std::vector<float> rmses;
            for (auto n = 34; n < 42; ++n)
            {
                double rms = 0;
                for (int i = 0; i < oneSecInBlocks; ++i)
                {
                    if (i == 60)
                        sc3->PlayNote(0, n, 120);
                    if (i == oneSecInBlocks - 50)
                        sc3->ReleaseNote(0, n, 0);

                    sc3->process_audio();
                    for (int k = 0; k < block_size; ++k)
                    {
                        rms += sc3->output[0][k] * sc3->output[0][k] +
                               sc3->output[1][k] * sc3->output[1][k];
                    }
                }
                for (int i = 0; i < 2000; ++i)
                    sc3->process_audio();
                rms = sqrt(rms);
                if (n < 36 || n > 38)
                {
                    REQUIRE(rms == Approx(0).margin(1e-5));
                }
                else
                {
                    INFO("Checking with note " << n);
                    std::vector<float> vals = {16.964388, 14.899617, 8.906973};
                    REQUIRE(rms == Approx(vals[n - 36]).margin(1e-5));
                }
            }
        }
}