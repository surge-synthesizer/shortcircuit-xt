#ifndef SCXT_SRC_SAMPLE_SAMPLE_H
#define SCXT_SRC_SAMPLE_SAMPLE_H

#include "utils.h"
#include "infrastructure/filesystem_import.h"

namespace scxt::sample
{

struct alignas(16) Sample : MoveableOnly<Sample>
{
    Sample() : id(SampleID::next()) {}
    Sample(const SampleID &sid) : id(sid) {}
    virtual ~Sample() = default;

    bool load(const fs::path &path);
    const fs::path &getPath() const { return mFileName; }

    size_t getDataSize() const { return sample_length * (UseInt16 ? 2 : 4) * channels; }
    size_t getSampleLength() const { return sample_length; }

    void *__restrict sampleData[2]{nullptr, nullptr};

    // TODO: Review evertyhing from here down before moving it above this comment
    bool parse_riff_wave(void *data, size_t filesize, bool skip_riffchunk = false);
    short *GetSamplePtrI16(int Channel);
    float *GetSamplePtrF32(int Channel);
    char *GetName();

  private:
    bool parse_aiff(void *data, size_t filesize);
    bool parse_sf2_sample(void *data, size_t filesize, unsigned int sampleid);
    bool parse_dls_sample(void *data, size_t filesize, unsigned int sampleid);

  public:
    size_t SaveWaveChunk(void *data);
    bool save_wave_file(const fs::path &filename);

    // TODO: Consistent Names here for goodness sake
    // public data
    bool UseInt16{false}; // TODO: Make this an enum not a binary
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
        std::cout << __FILE__ << ":" << __LINE__ << " " << __func__ << std::endl;
    }; // deletes sample data and marks sample as unused

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