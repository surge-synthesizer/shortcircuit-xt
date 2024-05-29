//
// Created by Paul Walker on 5/29/24.
//

#include "sample/sample.h"

#if SCXT_USE_MP3
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include "minimp3_ex.h"
namespace scxt::sample
{
bool Sample::parseMP3(const fs::path &p)
{
    mp3dec_t mp3d;
    mp3dec_file_info_t info;
    if (mp3dec_load(&mp3d, p.u8string().c_str(), &info, nullptr, nullptr))
    {
        SCLOG("Failed to parse MP3");
        return false;
    }

    if (info.channels < 1 || info.channels > 2)
        return false;

    sample_rate = info.hz;
    channels = info.channels;
    sample_length = info.samples / info.channels;
    bitDepth = BD_I16;

    if (channels == 1)
    {
        allocateI16(0, sample_length);
        memcpy(sampleData[0], info.buffer, sample_length * sizeof(mp3d_sample_t));
    }
    else
    {
        allocateI16(0, sample_length);
        allocateI16(1, sample_length);

        // de-interleave by hand I guess
        for (int i = 0; i < sample_length; ++i)
        {
            ((int16_t *)sampleData[0])[i] = info.buffer[i * 2];
            ((int16_t *)sampleData[1])[i] = info.buffer[i * 2 + 1];
        }
    }

    return true;
}
} // namespace scxt::sample
#else
namespace scxt::sample
{
bool Sample::parseMP3(const fs::path &p) { return false; }
} // namespace scxt::sample
#endif
