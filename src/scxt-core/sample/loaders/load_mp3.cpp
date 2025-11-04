/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "sample/sample.h"

#if SCXT_USE_MP3
#define MINIMP3_IMPLEMENTATION

#include <fstream>
#include "minimp3.h"
#include "minimp3_ex.h"
namespace scxt::sample
{
bool Sample::parseMP3(const uint8_t *data, size_t len)
{
    try
    {
        fs::path tf = fs::temp_directory_path();
        auto rf = rand() % 1000000;
        auto ff = tf / (std::string("scxt_mp3_temp_") + std::to_string(rf) + ".mp3");

        while (fs::exists(ff))
        {
            rf = rand() % 1000000;
            ff = tf / (std::string("scxt_mp3_temp_") + std::to_string(rf) + ".mpe");
        }

        std::ofstream outFile(ff, std::ios::binary);
        if (!outFile)
        {
            return false;
        }
        outFile.write((const char *)data, len);
        outFile.close();

        auto res = parseMP3(ff);

        fs::remove(ff);
        return res;
    }
    catch (fs::filesystem_error &e)
    {
        SCLOG("mp3 temp blew up " << e.what());
        return false;
    }
}

bool Sample::parseMP3(const fs::path &p)
{
    mp3dec_t mp3d;
    mp3dec_file_info_t info;
#if WIN32
    int count =
        MultiByteToWideChar(CP_UTF8, 0, p.u8string().c_str(), p.u8string().length(), NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, p.u8string().c_str(), p.u8string().length(), &wstr[0], count);
    if (mp3dec_load_w(&mp3d, &wstr[0], &info, nullptr, nullptr))
    {
        SCLOG("Failed to parse MP3");
        return false;
    }
#else
    if (mp3dec_load(&mp3d, p.u8string().c_str(), &info, nullptr, nullptr))
    {
        SCLOG("Failed to parse MP3");
        return false;
    }
#endif

    if (info.channels < 1 || info.channels > 2)
        return false;

    sample_rate = info.hz;
    channels = info.channels;
    sample_length = info.samples / info.channels;
    bitDepth = BD_I16;

    if (channels == 1)
    {
        allocateI16(0, sample_length);
        auto *dat = GetSamplePtrI16(0);
        memcpy(dat, info.buffer, sample_length * sizeof(mp3d_sample_t));
    }
    else
    {
        allocateI16(0, sample_length);
        allocateI16(1, sample_length);

        auto *dat0 = GetSamplePtrI16(0);
        auto *dat1 = GetSamplePtrI16(1);

        // de-interleave by hand I guess
        for (int i = 0; i < sample_length; ++i)
        {
            dat0[i] = info.buffer[i * 2];
            dat1[i] = info.buffer[i * 2 + 1];
        }
    }

    free(info.buffer);

    return true;
}
} // namespace scxt::sample
#else
namespace scxt::sample
{
bool Sample::parseMP3(const fs::path &p) { return false; }
bool Sample::parseMP3(const uint8_t *data, size_t len) { return false; }
} // namespace scxt::sample
#endif
