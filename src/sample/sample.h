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
#ifndef SCXT_SRC_SAMPLE_SAMPLE_H
#define SCXT_SRC_SAMPLE_SAMPLE_H

#include "utils.h"
#include "infrastructure/filesystem_import.h"
#include "SF.h"

namespace scxt::sample
{

struct alignas(16) Sample : MoveableOnly<Sample>
{
    enum SourceType
    {
        WAV_FILE,
        SF2_FILE,
        SFZ_FILE,
        FLAC_FILE,
        MP3_FILE,
        AIFF_FILE,
        MULTISAMPLE_FILE,
    } type{WAV_FILE};

    Sample() {}
    Sample(const SampleID &sid) : displayName(sid.to_string()), id(sid) {}
    virtual ~Sample();

    void dumpInformationToLog();

    std::string displayName{};
    std::string getDisplayName() const { return displayName; }
    bool load(const fs::path &path);
    bool loadFromSF2(const fs::path &path, sf2::File *f, int sampleIndex);

    const fs::path &getPath() const { return mFileName; }
    std::string md5Sum{};
    std::string getMD5Sum() const { return md5Sum; }

    int preset{-1}, instrument{-1}, region{-1};
    int getCompoundPreset() const { return preset; }
    int getCompoundInstrument() const { return instrument; }
    int getCompoundRegion() const { return region; }

    struct SampleFileAddress
    {
        SourceType type{WAV_FILE};
        fs::path path{};
        std::string md5sum{};
        int preset{-1};
        int instrument{-1};
        int region{-1};
    };

    bool isMissingPlaceholder{false};
    static std::shared_ptr<Sample> createMissingPlaceholder(const SampleFileAddress &a);

    SampleFileAddress getSampleFileAddress() const
    {
#if BUILD_IS_DEBUG
        if (getMD5Sum().empty())
        {
            SCLOG("Streaming empty MD5 sum for " << getPath().u8string());
        }
#endif
        return {type,
                getPath(),
                getMD5Sum(),
                getCompoundPreset(),
                getCompoundInstrument(),
                getCompoundRegion()};
    }

    size_t getDataSize() const { return sample_length * bitDepthByteSize(bitDepth) * channels; }
    size_t getSampleLength() const { return sample_length; }
    std::string getBitDepthText() const { return bitDepthName(bitDepth); }

    bool parseFlac(const fs::path &p);
    bool parseMP3(const fs::path &p);

    void *__restrict sampleData[2]{nullptr, nullptr};

    // TODO: Review evertyhing from here down before moving it above this comment
    bool parse_riff_wave(void *data, size_t filesize, bool skip_riffchunk = false);
    bool parse_aiff(void *data, size_t filesize);
    short *GetSamplePtrI16(int Channel);
    float *GetSamplePtrF32(int Channel);
    char *GetName();

  private:
    bool parse_sf2_sample(void *data, size_t filesize, unsigned int sampleid);
    bool parse_dls_sample(void *data, size_t filesize, unsigned int sampleid);

  public:
    size_t SaveWaveChunk(void *data);
    bool save_wave_file(const fs::path &filename);

    // TODO: Consistent Names here for goodness sake
    // public data
    enum BitDepth
    {
        // Right now 8 -> I16 at load and 24 -> F32 at load and noone supports 12 so just make this
        // BD_I8,
        // BD_I12,
        BD_I16,
        // BD_I24,
        BD_F32
    } bitDepth{BD_F32};

    static std::string bitDepthName(BitDepth bd)
    {
        switch (bd)
        {
        case BD_I16:
            return "I16";
        case BD_F32:
            return "F32";
        default:
            return "UNKWN";
        }
    }

    static constexpr int bitDepthByteSize(BitDepth bd)
    {
        switch (bd)
        {
        case BD_I16:
            return 2;
        case BD_F32:
            return 4;
        default:
            return 1;
        }
        return 1; // wuh?
    }

    uint8_t channels{0};
    bool Embedded{false}; // if true, sample data will be stored inside the patch/multi
    uint32_t sample_length{0};
    uint32_t sample_rate{1};
    float InvSampleRate{1};
    uint32_t *graintable{nullptr};
    int num_grains{0};
    bool grains_initialized{false};
    void init_grains();
    char name[64]{'\0'};

    enum PlayMode : uint8_t
    {
        pm_forward = 0,
        pm_forward_loop,
        // pm_forward_loop_crossfade,
        pm_forward_loop_until_release,
        pm_forward_loop_bidirectional,
        pm_forward_shot,
        pm_forward_hitpoints,
        pm_forward_release,
        // pm_reverse,
        // pm_reverse_shot,
        n_playmodes,

        pm_forwardRIFF = 0,
        pm_forward_loopRIFF = 1,
        pm_forward_loop_until_releaseRIFF = 3,
        pm_forward_loop_bidirectionalRIFF = 4,
        pm_forward_shotRIFF = 5,
        pm_forward_hitpointsRIFF = 6,
        pm_forward_releaseRIFF = 7,
    };

    // meta data
    struct
    {
        char key_low{0}, key_high{0}, key_root{0};
        char vel_low{0}, vel_high{0};
        PlayMode playmode{pm_forward};
        float detune{0};
        unsigned int loop_start{0}, loop_end{0};
        bool rootkey_present{false}, key_present{false}, vel_present{false}, loop_present{false},
            playmode_present{false};
        int n_slices{0};
        int *slice_start{nullptr};
        int *slice_end{nullptr};
        int n_beats{0};
    } meta;

  private:
    void clear_data()
    {
        // TODO: Figure Out and Implement clear_data
        // deletes sample data and marks sample as unused
    }

  public:
    bool allocateI16(int Channel, int Samples);
    bool allocateF32(int Channel, int Samples);

    bool load_data_ui8(int channel, void *data, unsigned int samplesize, unsigned int stride);
    bool load_data_i8(int channel, void *data, unsigned int samplesize, unsigned int stride);
    bool load_data_i16(int channel, void *data, unsigned int samplesize, unsigned int stride);
    bool load_data_i16BE(int channel, void *data, unsigned int samplesize, unsigned int stride);
    bool load_data_i24(int channel, void *data, unsigned int samplesize, unsigned int stride);
    bool load_data_i24BE(int channel, void *data, unsigned int samplesize, unsigned int stride);
    bool load_data_i32(int channel, void *data, unsigned int samplesize, unsigned int stride);
    bool load_data_i32BE(int channel, void *data, unsigned int samplesize, unsigned int stride);
    bool load_data_f32(int channel, void *data, unsigned int samplesize, unsigned int stride);
    bool load_data_f64(int channel, void *data, unsigned int samplesize, unsigned int stride);
    bool sample_loaded{false};

    bool SetMeta(unsigned int channels, unsigned int SampleRate, unsigned int SampleLength);
    fs::path mFileName{};

  public:
    SampleID id;
};
} // namespace scxt::sample

#endif