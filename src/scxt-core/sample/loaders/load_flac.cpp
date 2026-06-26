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
#include "sample/sample.h"

#include <memory>

#if SCXT_USE_FLAC
#include <fstream>
#include "FLAC++/decoder.h"
#include "FLAC++/metadata.h"
#include "riff_wave.h" // this lets us unpack smpl chunks

namespace scxt::sample
{
namespace detail
{
template <class T> class SampleFLACDecoderBase : public T
{
  public:
    Sample *sample{nullptr};
    SampleFLACDecoderBase(Sample *s) : FLAC::Decoder::File(), sample(s) {}
    SampleFLACDecoderBase(Sample *s, size_t cs)
        : FLAC::Decoder::File(), collectedSize{cs}, sample(s)
    {
    }

    bool isValid{false};
    bool needsSecondPass{false};
    size_t collectedSize{0};

  protected:
    bool collectSizeOnly{false};

    FILE *f;

    int64_t streamPos{0};

    virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame,
                                                            const FLAC__int32 *const buffer[])
    {
        if (collectSizeOnly)
        {
            collectedSize += frame->header.blocksize;
            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
        }

        // STREAMINFO can understate the total frame count; never write past
        // the buffer that metadata_callback allocated for sampleLengthPerChannel.
        int64_t avail = (int64_t)sample->sampleLengthPerChannel - streamPos;
        if (avail <= 0)
            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
        const unsigned n = (unsigned)std::min<int64_t>(frame->header.blocksize, avail);

        if (bitDepth == 16 && sample->bitDepth == Sample::BD_I16)
        {
            for (int c = 0; c < sample->channels; ++c)
            {
                auto sdata = sample->GetSamplePtrI16(c);
                for (unsigned i = 0; i < n; i++)
                {
                    sdata[i + streamPos] = (FLAC__int16)buffer[c][i];
                }
            }
            streamPos += n;
            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
        }
        else if (bitDepth == 24 && sample->bitDepth == Sample::BD_F32)
        {
            for (int c = 0; c < sample->channels; ++c)
            {
                auto sdata = sample->GetSamplePtrF32(c);
                for (unsigned i = 0; i < n; i++)
                {
                    sdata[i + streamPos] = buffer[c][i] * 1.f / (1 << 24);
                }
            }
            streamPos += n;

            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
        }
        else if (bitDepth == 32 && sample->bitDepth == Sample::BD_F32)
        {
            for (int c = 0; c < sample->channels; ++c)
            {
                auto sdata = sample->GetSamplePtrF32(c);
                for (unsigned i = 0; i < n; i++)
                {
                    sdata[i + streamPos] = (double)(buffer[c][i] * 1.0) / (1LL << 32);
                }
            }
            streamPos += n;

            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
        }

        SCLOG_IF(warnings, "Unable to load flac; bitdepth ("
                               << bitDepth
                               << " ) and sample bitdepth are not a supported combo. Aborting.");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata)
    {
        if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
        {
            /* save for later */
            auto total_samples = metadata->data.stream_info.total_samples;
            auto sample_rate = metadata->data.stream_info.sample_rate;
            auto channels = metadata->data.stream_info.channels;
            auto bps = metadata->data.stream_info.bits_per_sample;

            sample->sample_rate = sample_rate;
            sample->channels = channels;
            sample->bitDepth = (bps <= 16 ? Sample::BD_I16 : Sample::BD_F32);
            if (collectedSize > 0)
            {
                total_samples = collectedSize;
                collectSizeOnly = false;
            }

            sample->sampleLengthPerChannel = total_samples;

            if (total_samples == 0)
            {
                collectedSize = 0;
                collectSizeOnly = true;
                needsSecondPass = true;
            }
            else if (channels != 1 && channels != 2)
            {
                // with no buffer allocated for 3+ channels, leaving
                // isValid/bitDepth unset makes write_callback abort cleanly
                // instead of writing through a null GetSamplePtr.
                SCLOG_IF(warnings, "Unsupported FLAC channel count " << channels);
            }
            else if (bps == 16)
            {
                for (unsigned c = 0; c < channels; ++c)
                    sample->allocateI16(c, total_samples);
                isValid = true;
                bitDepth = 16;
            }
            else if (bps == 24 || bps == 32)
            {
                for (unsigned c = 0; c < channels; ++c)
                    sample->allocateF32(c, total_samples);
                isValid = true;
                bitDepth = bps;
            }
            else
            {
                SCLOG_IF(warnings, "Unsupported FLAC bit depth " << bps);
            }
        }
        else
        {
            SCLOG_IF(warnings,
                     "Ignoring decode-time FLAC metadata chunk " << (int)(metadata->type));
        }
    }
    virtual void error_callback(::FLAC__StreamDecoderErrorStatus status) {}

  private:
    int bitDepth{-1};
    SampleFLACDecoderBase(const SampleFLACDecoderBase &);
    SampleFLACDecoderBase &operator=(const SampleFLACDecoderBase &);
};

struct SampleFLACDecoder : public SampleFLACDecoderBase<FLAC::Decoder::File>
{
    SampleFLACDecoder(Sample *s) : SampleFLACDecoderBase(s) {}
    SampleFLACDecoder(Sample *s, size_t cs) : SampleFLACDecoderBase(s, cs) {}
};
} // namespace detail

bool Sample::parseFlac(const uint8_t *data, size_t len)
{
    try
    {
        fs::path tf = fs::temp_directory_path();
        auto rf = rand() % 1000000;
        auto ff = tf / (std::string("scxt_flac_temp_") + std::to_string(rf) + ".flac");

        while (fs::exists(ff))
        {
            rf = rand() % 1000000;
            ff = tf / (std::string("scxt_flac_temp_") + std::to_string(rf) + ".flac");
        }

        std::ofstream outFile(ff, std::ios::binary);
        if (!outFile)
        {
            return false;
        }
        outFile.write((const char *)data, len);
        outFile.close();

        auto res = parseFlac(ff);

        fs::remove(ff);
        return res;
    }
    catch (fs::filesystem_error &e)
    {
        SCLOG_IF(warnings, "flac temp blew up " << e.what());
        return false;
    }
}

bool Sample::parseFlac(const fs::path &p)
{
    auto dec = detail::SampleFLACDecoder(this);
    auto status = dec.init(p.u8string());
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
    {
        addError("Unable to initiate flac streamer " + std::to_string(status));
        return false;
    }

    auto res = dec.process_until_end_of_stream();

    if (dec.needsSecondPass)
    {
        auto dec2 = detail::SampleFLACDecoder(this, dec.collectedSize);
        auto status = dec2.init(p.u8string());
        if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
        {
            addError("Unable to initiate flac streamer " + std::to_string(status));
            return false;
        }

        res = dec2.process_until_end_of_stream();
        res = res && dec2.isValid;
    }
    else
    {
        res = res && dec.isValid;
    }

    mFileName = p;
    type = Sample::FLAC_FILE;
    instrument = 0;
    region = 0;

    if (res)
    {
        auto si = std::make_unique<FLAC::Metadata::SimpleIterator>();
        res = res && si->init(p.u8string().c_str(), true, true);

        bool go = si->is_valid();
        while (go)
        {
            if (si->get_block_type() == FLAC__METADATA_TYPE_APPLICATION)
            {
                // must not `continue` out of here — that would skip the
                // si->next() at the loop bottom and spin on this block. Guard
                // with positive conditions and always delete instead.
                auto a = dynamic_cast<FLAC::Metadata::Application *>(si->get_block());
                if (a)
                {
                    auto fourB2 = [](const auto *b) {
                        std::string ids;
                        for (int i = 0; i < 4; ++i)
                            ids += (char)b[i];
                        return ids;
                    };
                    // get_length() excludes the block header but includes the
                    // 4-byte application id; get_data() points past that id.
                    size_t appLen = a->get_length() >= 4 ? (size_t)a->get_length() - 4 : 0;
                    if (fourB2(a->get_id()) == "riff" && appLen >= 8)
                    {
                        auto *d = a->get_data();
                        auto blockType = fourB2(d);
                        // consumed from get_data(): blockType(4) + chunk size(4)
                        size_t off = 8;
                        // bounds-check each read against the application block
                        // length before copying the fixed-size structs.
                        if (blockType == "smpl" && off + sizeof(loaders::SamplerChunk) <= appLen)
                        {
                            loaders::SamplerChunk smpl_chunk;
                            loaders::SampleLoop smpl_loop;

                            memcpy(&smpl_chunk, d + off, sizeof(loaders::SamplerChunk));
                            off += sizeof(loaders::SamplerChunk);

                            meta.key_root = smpl_chunk.dwMIDIUnityNote & 0xFF;
                            meta.rootkey_present = true;

                            if (smpl_chunk.cSampleLoops > 0 &&
                                off + sizeof(loaders::SampleLoop) <= appLen)
                            {
                                meta.loop_present = true;
                                memcpy(&smpl_loop, d + off, sizeof(loaders::SampleLoop));
                                off += sizeof(loaders::SampleLoop);

                                meta.loop_start = smpl_loop.dwStart;
                                meta.loop_end =
                                    std::min((uint32_t)smpl_loop.dwEnd + 1, sampleLengthPerChannel);
                                if (smpl_loop.dwType == 1)
                                    meta.playmode = pm_forward_loop_bidirectional;
                                else
                                    meta.playmode = pm_forward_loop;
                            }
                        }
                    }
                    delete a;
                }
            }
            else if (si->get_block_type() == FLAC__METADATA_TYPE_STREAMINFO)
            {
            }
            else if (si->get_block_type() == FLAC__METADATA_TYPE_VORBIS_COMMENT)
            {
                auto a = dynamic_cast<FLAC::Metadata::VorbisComment *>(si->get_block());
                if (a)
                {
                    for (int q = 0; q < a->get_num_comments(); ++q)
                    {
                        auto c = a->get_comment(q);
                    }
                    delete a;
                }
            }
            else
            {
            }

            go = si->next();
        }
    }
    else
    {
        addError("Unable to decode flac stream");
    }
    return res;
}
} // namespace scxt::sample
#else
namespace scxt::sample
{
bool Sample::parseFlac(const fs::path &p) { return false; }
bool Sample::parseFlac(const uint8_t *data, size_t len) { return false; }
} // namespace scxt::sample
#endif
