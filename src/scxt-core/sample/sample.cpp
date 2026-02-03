/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include <sstream>
#include "sst/basic-blocks/mechanics/endian-ops.h"
#include "infrastructure/file_map_view.h"
#include "infrastructure/md5support.h"
#include "dsp/resampling.h"
#include "sample.h"
#include "patch_io/patch_io.h"

namespace scxt::sample
{

// Fine in a cpp
using namespace sst::basic_blocks::mechanics;

Sample::~Sample()
{
    if (sampleData[0])
        free(sampleData[0]);
    if (sampleData[1])
        free(sampleData[1]);
}

bool Sample::load(const fs::path &path)
{
    resetErrorString();
    if (!fs::exists(path))
    {
        addError("File " + path.u8string() + " does not exist.");
        return false;
    }

    md5Sum = infrastructure::createMD5SumFromFile(path);
    id.setPathHash(path.u8string().c_str());

    // If you add a type here add it in Browser::isLoadableFile also to stay in sync
    if (extensionMatches(path, ".wav"))
    {
        auto fmv = std::make_unique<infrastructure::FileMapView>(path);
        auto data = fmv->data();
        auto datasize = fmv->dataSize();

        clear_data(); // clear to a more predictable state

        bool r = parse_riff_wave(data, datasize);
        if (!r)
        {
            addError("Unable to parse RIFF");
            return false;
        }

        sample_loaded = true;
        mFileName = path;
        displayName = fmt::format("{}", path.filename().u8string());
        compoundSourceDetails = "";
        type = WAV_FILE;

        id.setAsMD5(md5Sum);

        return true;
    }
    else if (extensionMatches(path, ".flac"))
    {
        if (parseFlac(path))
        {
            sample_loaded = true;
            type = FLAC_FILE;
            mFileName = path;
            displayName = fmt::format("{}", path.filename().u8string());
            compoundSourceDetails = "";

            id.setAsMD5(md5Sum);

            return true;
        }
        else
        {
            addError("Unable to parse FLAC");
        }
    }
    else if (extensionMatches(path, ".mp3"))
    {
        if (parseMP3(path))
        {
            sample_loaded = true;
            type = MP3_FILE;
            mFileName = path;
            displayName = fmt::format("{}", path.filename().u8string());
            compoundSourceDetails = "";

            id.setAsMD5(md5Sum);

            return true;
        }
        else
        {
            addError("Unable to parse FLAC");
        }
    }
    else if (extensionMatches(path, ".aif") || extensionMatches(path, ".aiff"))
    {
        auto fmv = std::make_unique<infrastructure::FileMapView>(path);
        auto data = fmv->data();
        auto datasize = fmv->dataSize();

        clear_data(); // clear to a more predictable state

        bool r = parse_aiff(data, datasize);
        if (!r)
        {
            addError("Unable to parse AIF");
            return false;
        }

        type = AIFF_FILE;
        sample_loaded = true;
        mFileName = path;
        displayName = fmt::format("{}", path.filename().u8string());
        compoundSourceDetails = "";

        id.setAsMD5(md5Sum);

        return true;
    }

    return false;
}

bool Sample::loadFromSF2(const fs::path &p, sf2::File *f, int sampleIndex)
{
    mFileName = p;
    preset = -1;
    instrument = -1;
    region = sampleIndex;
    type = SF2_FILE;

    auto sfsample = f->GetSample(sampleIndex);
    if (!sfsample)
    {
        addError("Unable to load sample " + std::to_string(sampleIndex) + " of " + p.u8string());
        return false;
    }

    auto frameSize = sfsample->GetFrameSize();
    channels = sfsample->GetChannelCount();
    sampleLengthPerChannel = sfsample->GetTotalFrameCount();
    sample_rate = sfsample->SampleRate;

    auto s = sfsample;

    auto fnp = fs::path(fs::u8path(f->GetRiffFile()->GetFileName()));
    displayName = std::string(s->Name);
    compoundSourceDetails = fmt::format("{} @ {}", fnp.filename().u8string(), sampleIndex);

    if (frameSize == 2 && channels == 1 && sfsample->SampleType == sf2::Sample::MONO_SAMPLE)
    {
        bitDepth = BD_I16;
        auto buf = sfsample->LoadSampleData();
        // >> 1 here because void* -> int16_t is byte to two bytes
        load_data_i16(0, buf.pStart, buf.Size >> 1, sfsample->GetFrameSize());
        sfsample->ReleaseSampleData();
        return true;
    }
    else if (frameSize == 4 && sfsample->GetChannelCount() == 2 &&
             (sfsample->SampleType == sf2::Sample::LEFT_SAMPLE ||
              sfsample->SampleType == sf2::Sample::RIGHT_SAMPLE))
    {
        bitDepth = BD_I16;
        auto buf = sfsample->LoadSampleData();
        channels = 1;
        if (sfsample->SampleType == sf2::Sample::LEFT_SAMPLE)
        {
            // >> 2 here because void* -> int16_t is byte to two bytes, plus two channels with a
            // stride
            load_data_i16(0, (int16_t *)(buf.pStart), buf.Size >> 2, sfsample->GetFrameSize());
        }
        else if (sfsample->SampleType == sf2::Sample::RIGHT_SAMPLE)
        {
            load_data_i16(0, (int16_t *)(buf.pStart) + 1, buf.Size >> 2, sfsample->GetFrameSize());
        }
        sfsample->ReleaseSampleData();
        return true;
    }
    else if (sfsample->GetFrameSize() == 3 && sfsample->GetChannelCount() == 1)
    {
        bitDepth = BD_I16;
        channels = 1;
        auto buf = sfsample->LoadSampleData();
        load_data_i24(0, (void *)(buf.pStart), buf.Size, sfsample->GetFrameSize());
        return true;
    }

    std::ostringstream oss;
    oss << "Unable to load sample from SF2. " << SCD(sfsample->GetFrameSize())
        << SCD(sfsample->GetChannelCount());
    addError(oss.str());
    return false;
}

bool Sample::loadFromGIG(const fs::path &p, gig::File *f, int sampleIndex)
{
    mFileName = p;
    preset = -1;
    instrument = -1;
    region = sampleIndex;
    type = GIG_FILE;

    auto sfsample = f->GetSample(sampleIndex);
    if (!sfsample)
    {
        addError("Unable to load sample " + std::to_string(sampleIndex) + " of " + p.u8string());
        return false;
    }

    auto frameSize = sfsample->FrameSize;
    channels = sfsample->Channels;
    sampleLengthPerChannel = sfsample->SamplesTotal;
    sample_rate = sfsample->SamplesPerSecond;

    auto s = sfsample;

    auto fnp = fs::path(fs::u8path(f->GetRiffFile()->GetFileName()));
    std::string sname = "Gig Sample";
    if (s->pInfo)
        sname = s->pInfo->Name;
    displayName = sname;
    compoundSourceDetails = fmt::format("{} @ {}", fnp.filename().u8string(), sampleIndex);

    SCLOG_IF(sampleLoadAndPurge, "GIG " << SCD((int)channels) << SCD(sampleLengthPerChannel)
                                        << SCD(sample_rate) << SCD(frameSize) << " "
                                        << displayName);
    if (frameSize == 2 && channels == 1)
    {
        bitDepth = BD_I16;
        auto buf = sfsample->LoadSampleData();
        // >> 1 here because void* -> int16_t is byte to two bytes
        load_data_i16(0, buf.pStart, buf.Size >> 1, sfsample->FrameSize);
        sfsample->ReleaseSampleData();
        return true;
    }
    else if (frameSize == 4 && channels == 2)
    {
        bitDepth = BD_I16;
        auto buf = sfsample->LoadSampleData();
        // >> 2 here because void* -> int16_t is byte to two bytes, plus two channels with a
        // stride
        load_data_i16(0, (int16_t *)(buf.pStart), buf.Size >> 2, frameSize);
        load_data_i16(1, (int16_t *)(buf.pStart) + 1, buf.Size >> 2, frameSize);

        sfsample->ReleaseSampleData();
        return true;
    }

    addError("Cant load frameSize/channels from gig " + std::to_string(frameSize) + "/" +
             std::to_string(channels));
    return false;
}

bool Sample::loadFromSCXTMonolith(const fs::path &path, RIFF::File *f, int sampleIndex)
{
    mFileName = path;
    preset = -1;
    instrument = -1;
    region = sampleIndex;
    type = SCXT_FILE;

    SCLOG_IF(sampleLoadAndPurge, "Load from SCXT: " << SCD(path) << SCD(sampleIndex));
    auto reader = patch_io::SCMonolithSampleReader(f);

    patch_io::SCMonolithSampleReader::SampleData dat;
    if (!reader.getSampleData(sampleIndex, dat))
    {
        addError(reader.getErrorString());
        addError("Failed to get sample data from reader");
        return false;
    }

    displayName = dat.filename;
    compoundSourceDetails = fmt::format("{} @ {}", path.filename().u8string(), sampleIndex);

    auto fnP = fs::path(fs::u8path(dat.filename));

    if (extensionMatches(fnP, ".wav"))
    {
        auto res = parse_riff_wave(dat.data.data(), dat.data.size());
        if (!res)
            addError("Unable to parse embedded wav");
        return res;
    }
    else if (extensionMatches(fnP, ".aif") || extensionMatches(fnP, ".aiff"))
    {
        auto res = parse_aiff(dat.data.data(), dat.data.size());
        if (!res)
            addError("Unable to parse embedded aif");
        return res;
    }
    else if (extensionMatches(fnP, ".flac"))
    {
        auto res = parseFlac(dat.data.data(), dat.data.size());
        if (!res)
            addError("Unable to parse embedded FLAC");

        mFileName = path;
        preset = -1;
        instrument = -1;
        region = sampleIndex;
        type = SCXT_FILE;

        return res;
    }
    else if (extensionMatches(fnP, ".mp3"))
    {
        auto res = parseMP3(dat.data.data(), dat.data.size());
        if (!res)
            addError("Unable to parse embedded MP3");

        mFileName = path;
        preset = -1;
        instrument = -1;
        region = sampleIndex;
        type = SCXT_FILE;

        return res;
    }
    else
    {
        addError("Unknown embedded sample format " + fnP.extension().u8string());
        return false;
    }

    return false;
}

// TODO: Rename these
short *Sample::GetSamplePtrI16(int Channel)
{
    if (bitDepth != BD_I16)
        return nullptr;
    if (!sampleData[Channel])
        return nullptr;
    return &((short *)sampleData[Channel])[scxt::dsp::FIRoffset];
}
float *Sample::GetSamplePtrF32(int Channel)
{
    if (bitDepth != BD_F32)
        return nullptr;
    if (!sampleData[Channel])
        return nullptr;
    return &((float *)sampleData[Channel])[scxt::dsp::FIRoffset];
}

// TODO: What the heck is this doing?
bool Sample::allocateI16(int Channel, int Samples)
{
    // int samplesizewithmargin = Samples + 2*scxt::dsp::FIRipol_N + BLOCK_SIZE +
    // scxt::dsp::FIRoffset;
    int samplesizewithmargin = Samples + scxt::dsp::FIRipol_N;
    if (sampleData[Channel])
        free(sampleData[Channel]);
    sampleData[Channel] = malloc(sizeof(short) * samplesizewithmargin);
    if (!sampleData[Channel])
        return false;
    bitDepth = BD_I16;

    // clear pre/post zero area
    memset(sampleData[Channel], 0, scxt::dsp::FIRoffset * sizeof(short));
    memset((char *)sampleData[Channel] + (Samples + scxt::dsp::FIRoffset) * sizeof(short), 0,
           scxt::dsp::FIRoffset * sizeof(short));

    return true;
}
bool Sample::allocateF32(int Channel, int Samples)
{
    int samplesizewithmargin = Samples + scxt::dsp::FIRipol_N;
    if (sampleData[Channel])
        free(sampleData[Channel]);
    sampleData[Channel] = malloc(sizeof(float) * samplesizewithmargin);
    if (!sampleData[Channel])
        return false;
    bitDepth = BD_F32;

    // clear pre/post zero area
    memset(sampleData[Channel], 0, scxt::dsp::FIRoffset * sizeof(float));
    memset((char *)sampleData[Channel] + (Samples + scxt::dsp::FIRoffset) * sizeof(float), 0,
           scxt::dsp::FIRoffset * sizeof(float));

    return true;
}

bool Sample::load_data_ui8(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    allocateI16(channel, samplesize);
    short *sampledata = GetSamplePtrI16(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = (((short)*((unsigned char *)data + i * stride)) - 128) << 8;
    }
    return true;
}

bool Sample::load_data_i8(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    allocateI16(channel, samplesize);
    short *sampledata = GetSamplePtrI16(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = ((short)*((char *)data + i * stride)) << 8;
    }
    return true;
}

bool Sample::load_data_i16(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    allocateI16(channel, samplesize);
    short *sampledata = GetSamplePtrI16(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = endian_read_int16LE(*(short *)((char *)data + i * stride));
    }
    return true;
}

bool Sample::load_data_i16BE(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    allocateI16(channel, samplesize);
    short *sampledata = GetSamplePtrI16(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = endian_read_int16BE(*(short *)((char *)data + i * stride));
    }
    return true;
}
bool Sample::load_data_i32(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    allocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        int x = endian_read_int32LE(*(int *)((char *)data + i * stride));
        sampledata[i] = (4.6566128730772E-10f) * (float)x;
    }
    return true;
}

bool Sample::load_data_i32BE(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    allocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        int x = endian_read_int32BE(*(int *)((char *)data + i * stride));
        sampledata[i] = (4.6566128730772E-10f) * (float)x;
    }
    return true;
}

bool Sample::load_data_i24(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    allocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        unsigned char *cval = (unsigned char *)data + i * stride;
        int value = (cval[2] << 16) | (cval[1] << 8) | cval[0];
        value -= (value & 0x800000) << 1;
        sampledata[i] = 0.00000011920928955078f * float(value);
        ;
    }
    return true;
}

bool Sample::load_data_i24BE(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    allocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        unsigned char *cval = (unsigned char *)data + i * stride;
        int value = (cval[0] << 16) | (cval[1] << 8) | cval[2];
        value -= (value & 0x800000) << 1;
        sampledata[i] = 0.00000011920928955078f * float(value);
        ;
    }
    return true;
}

bool Sample::load_data_f32(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    allocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = (*(float *)((char *)data + i * stride));
    }
    return true;
}

bool Sample::load_data_f64(int channel, void *data, unsigned int samplesize, unsigned int stride)
{
    allocateF32(channel, samplesize);
    float *sampledata = GetSamplePtrF32(channel);

    for (int i = 0; i < samplesize; i++)
    {
        sampledata[i] = (float)(*(double *)((char *)data + i * stride));
    }
    return true;
}

bool Sample::SetMeta(unsigned int Channels, unsigned int SampleRate, unsigned int SampleLength)
{
    if (Channels > 2)
    {
        addError("Channel count " + std::to_string(Channels) + " is not supported");
        return false; // not supported
    }

    channels = Channels;
    sampleLengthPerChannel = SampleLength;
    sample_rate = SampleRate;
    InvSampleRate = 1.f / (float)SampleRate;

    return true;
}

void Sample::dumpInformationToLog()
{
    SCLOG_IF(sampleLoadAndPurge, "Sample Dump for " << getDisplayName());
    switch (type)
    {
    case WAV_FILE:
        SCLOG_IF(sampleLoadAndPurge, "WAV File : " << getPath().u8string());
        break;
    case FLAC_FILE:
        SCLOG_IF(sampleLoadAndPurge, "FLAC File : " << getPath().u8string());
        break;
    case MP3_FILE:
        SCLOG_IF(sampleLoadAndPurge, "MP3 File : " << getPath().u8string());
        break;
    case AIFF_FILE:
        SCLOG_IF(sampleLoadAndPurge, "AIFF File : " << getPath().u8string());
        break;
    case SF2_FILE:
        SCLOG_IF(sampleLoadAndPurge, "SF2 File : " << getPath().u8string()
                                                   << " Instrument=" << getCompoundInstrument()
                                                   << " Region=" << getCompoundRegion());
        break;
    case SFZ_FILE:
        SCLOG_IF(sampleLoadAndPurge, "SFZ File : " << getPath().u8string());
        break;
    case MULTISAMPLE_FILE:
        SCLOG_IF(sampleLoadAndPurge,
                 "Multisample File : " << getPath().u8string() << " at " << getCompoundRegion());
        break;
    case GIG_FILE:
        SCLOG_IF(sampleLoadAndPurge, "GIG File : " << getPath().u8string());
        break;
    case SCXT_FILE:
        SCLOG_IF(sampleLoadAndPurge, "SCXT File : " << getPath().u8string());
        break;
    }

    SCLOG_IF(sampleLoadAndPurge,
             "BitDepth=" << bitDepthByteSize(bitDepth) * 8 << " Channels=" << (int)channels);
    SCLOG_IF(sampleLoadAndPurge,
             "SampleRate=" << sample_rate << " sample_length=" << sampleLengthPerChannel);

    switch (bitDepth)
    {
    case BD_I16:
    {
        for (int c = 0; c < channels; ++c)
        {
            auto *dat = GetSamplePtrI16(c);
            auto mxv = std::numeric_limits<int16_t>::min();
            auto mnv = std::numeric_limits<int16_t>::max();
            for (int i = 0; i < sampleLengthPerChannel; ++i)
            {
                mxv = std::max(mxv, dat[i]);
                mnv = std::min(mnv, dat[i]);
            }
            SCLOG_IF(sampleLoadAndPurge, "Min/Max = " << mxv << " " << mnv);

            const float I16InvScale = (1.f / (16384.f * 32768.f));
            SCLOG_IF(sampleLoadAndPurge,
                     "Min/Max Float Scaled = " << mxv / 32768.f << " " << mnv / 32768.f);
            SCLOG_IF(sampleLoadAndPurge, "Min/Max Generator Scaled = " << mxv * I16InvScale << " "
                                                                       << mnv * I16InvScale);
        }
    }
    break;
    case BD_F32:
    {
        SCLOG_IF(sampleLoadAndPurge, "TODO: Implement Sapmle Scan for F32");
    }
    break;
    }
}

std::shared_ptr<Sample> Sample::createMissingPlaceholder(const Sample::SampleFileAddress &a)
{
    auto res = std::make_shared<Sample>();
    res->mFileName = a.path;
    res->md5Sum = a.md5sum;
    res->type = a.type;
    res->preset = a.preset;
    res->instrument = a.instrument;
    res->region = a.region;
    res->isMissingPlaceholder = true;
    res->displayName = fmt::format("Missing {}", a.path.filename().u8string());

    return res;
}

Sample::SourceType Sample::sourceTypeFromPath(const fs::path &path)
{
    if (extensionMatches(path, ".wav"))
        return SourceType::WAV_FILE;
    if (extensionMatches(path, ".sf2"))
        return SourceType::SF2_FILE;
    if (extensionMatches(path, ".sfz"))
        return SourceType::SFZ_FILE;
    if (extensionMatches(path, ".flac"))
        return SourceType::FLAC_FILE;
    if (extensionMatches(path, ".mp3"))
        return SourceType::MP3_FILE;
    if (extensionMatches(path, ".aif") || extensionMatches(path, ".aiff"))
        return SourceType::AIFF_FILE;
    if (extensionMatches(path, ".multisample"))
        return SourceType::MULTISAMPLE_FILE;
    if (extensionMatches(path, ".gig"))
        return SourceType::GIG_FILE;

    SCLOG_IF(sampleLoadAndPurge, "Unmatched extension : " << path.u8string());
    return WAV_FILE;
}

} // namespace scxt::sample