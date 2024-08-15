/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#if SCXT_USE_FLAC
#include "FLAC++/decoder.h"
namespace scxt::sample
{
namespace detail
{
class SampleFLACDecoder : public FLAC::Decoder::File
{
  public:
    Sample *sample{nullptr};
    SampleFLACDecoder(Sample *s) : FLAC::Decoder::File(), sample(s) {}

    bool isValid{false};

  protected:
    FILE *f;

    int64_t streamPos{0};

    virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame,
                                                            const FLAC__int32 *const buffer[])
    {
        if (bitDepth == 16 && sample->bitDepth == Sample::BD_I16)
        {
            for (int c = 0; c < sample->channels; ++c)
            {
                auto sdata = sample->GetSamplePtrI16(c);
                for (int i = 0; i < frame->header.blocksize; i++)
                {
                    sdata[i + streamPos] = (FLAC__int16)buffer[c][i];
                }
            }
            streamPos += frame->header.blocksize;
            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
        }
        else if (bitDepth == 24 && sample->bitDepth == Sample::BD_F32)
        {
            for (int c = 0; c < sample->channels; ++c)
            {
                auto sdata = sample->GetSamplePtrF32(c);
                for (int i = 0; i < frame->header.blocksize; i++)
                {
                    sdata[i + streamPos] = buffer[c][i] * 1.f / (1 << 24);
                }
            }
            streamPos += frame->header.blocksize;

            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
        }
        else if (bitDepth == 32 && sample->bitDepth == Sample::BD_F32)
        {
            for (int c = 0; c < sample->channels; ++c)
            {
                auto sdata = sample->GetSamplePtrF32(c);
                for (int i = 0; i < frame->header.blocksize; i++)
                {
                    sdata[i + streamPos] = (double)(buffer[c][i] * 1.0) / (1L << 32);
                }
            }
            streamPos += frame->header.blocksize;

            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
        }

        SCLOG("Unable to load flac; bitdepth ("
              << bitDepth << " ) and sample bitdepth are not a supported combo. Aborting.");
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
            sample->sample_length = total_samples;

            if (bps == 16)
            {
                if (channels == 1)
                {
                    sample->allocateI16(0, total_samples);
                }
                else if (channels == 2)
                {
                    sample->allocateI16(0, total_samples);
                    sample->allocateI16(1, total_samples);
                }
                isValid = true;
                bitDepth = 16;
            }
            else if (bps == 24 || bps == 32)
            {
                if (channels == 1)
                {
                    sample->allocateF32(0, total_samples);
                }
                else if (channels == 2)
                {
                    sample->allocateF32(0, total_samples);
                    sample->allocateF32(1, total_samples);
                }
                isValid = true;
                bitDepth = bps;
            }
            else
            {
                SCLOG("Unsupported FLAC bit depth " << bps);
            }
        }
    }
    virtual void error_callback(::FLAC__StreamDecoderErrorStatus status) {}

  private:
    int bitDepth{-1};
    SampleFLACDecoder(const SampleFLACDecoder &);
    SampleFLACDecoder &operator=(const SampleFLACDecoder &);
};
} // namespace detail
bool Sample::parseFlac(const fs::path &p)
{
    auto dec = detail::SampleFLACDecoder(this);
    auto status = dec.init(p.u8string());
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
        return false;

    auto res = dec.process_until_end_of_stream();

    mFileName = p;
    type = Sample::FLAC_FILE;
    instrument = 0;
    region = 0;

    return res && dec.isValid;
}
} // namespace scxt::sample
#else
namespace scxt::sample
{
bool Sample::parseFlac(const fs::path &p) { return false; }
} // namespace scxt::sample
#endif
