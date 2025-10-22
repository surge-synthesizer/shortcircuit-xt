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

#ifndef SCXT_SRC_DSP_PROCESSOR_PROCESSOR_H
#define SCXT_SRC_DSP_PROCESSOR_PROCESSOR_H

/*
 * The non-fiction novel of how to port a filter from the old codebase to the next codebase,
 * with the microgate as an example. This goes up to the generic UI. For a custom UI see
 * (something I haven't written yet).
 *
 * The microgate is a zone filter which in the old code is ft_fx_microgate. It lives in
 * previous/src/synthesis/filters_destruction.cpp. So here's how to bring it here.
 *
 * First put the class definition in processor_defs.h. A few changes will be in order
 * - we use CamelCase names for classes so microgate->MicroGate
 * - rather than a global int make a class constexpr int for mg_size
 * - All processor constructors in next take the '3 arg' form of float*, int*, bool
 * - While it doesn't apply to microgate, int params API is renamed so get_ip_entry becomes
 * getIntParamEnntry etc... (compare the SVF).
 *
 * I defined process as{} and init as {floatParameterCount=0} so I had a stub
 *
 * Next make the stub available to the engine by registering it. This used to be editing a big
 * global switch but is now localized through the Magic Of Templates (tm). So immediately under the
 * MicroGate class definitino, specialize the ProcessorImplementation for the type. At this point if
 * you compile you will start getting errors about the configuration missing on the type.
 *
 * So add the constexprs (isZone etc.. and name). The processorStreamignName is what we stream
 * to patches across sessions so it should be stable and generally not contain special characters
 * and things. UTF-8 emojis might work but don't try it OK? It is never shown to a user anyway.
 *
 * With that in place, if you run the symth and compile, MicroGate will be available in the UI.
 * You can select it as a zone filter and voila. With the original port we now run into a problem
 * that we can't in-place new the microgate because it has a buffer member. These buffer members
 * were a primary driver of allocation. At the original port with loopbuffer[2][buffersize] the
 * static assert i placed in the specialization failed. So we need to fix that. Luckily we have
 * a processor memory pool which limits allocation so in the ::init() (not the constrctur)
 * we grab a block of memory and assign it to the loopbuffers.
 *
 * We then move most of the rest of the constructor to c++ initializers in the header and
 * go about setting up the control modes. Basically we set the objects onto the controlmodes
 * array in the constructor for the appropriate type. (As of this writing we don't actually
 * have the correct units for microgate param 0 though). Then implemente preocess_stereo and voila.
 *
 * At this point the effect should just work and the UI should appear. To learn about customizing
 * the ui, take a look in ProcessorPane.cpp in src-ui
 */

#include <cstdint>
#include <array>
#include <optional>
#include <vector>
#include <utility>
#include <cmath>

#include "sst/basic-blocks/simd/setup.h"
#include "sst/basic-blocks/mechanics/block-ops.h"

#include "datamodel/metadata.h"
#include "utils.h"
#include "dsp/data_tables.h"
#include "tuning/equal.h"
#include "configuration.h"

namespace scxt::engine
{
struct MemoryPool;
}

namespace scxt::dsp::processor
{

static constexpr size_t maxProcessorFloatParams{scxt::maxProcessorFloatParams};
static constexpr size_t maxProcessorIntParams{scxt::maxProcessorIntParams};
static constexpr size_t processorLabelSize{32};

/**
 * ProcessorType Type: These are streamed as strings using the toStreamingName / frommStreamingName
 * so we can reorder them. The only requirement is they are contiguous
 * (so don't assign values) and the last is the proct_num_types
 */
enum ProcessorType
{
    proct_none = 0,
    proct_SuperSVF, // First zone
    proct_CytomicSVF,
    proct_StaticPhaser,
    proct_SurgeBiquads,
    proct_VemberClassic,
    proct_K35,
    proct_diodeladder,
    proct_tripole,
    proct_obx4,
    proct_cutoffwarp,
    proct_reswarp,
    proct_vintageladder,
    proct_snhfilter,
    proct_SurgeFilters,
    proct_Tremolo,
    proct_Phaser,
    proct_shepard,
    proct_Chorus,
    proct_volpan,
    proct_stereotool,
    proct_gainmatrix,
    proct_Compressor,
    proct_autowah,

    proct_stringResonator,

    proct_moogLP4sat,
    proct_eq_1band_parametric_A,
    proct_eq_2band_parametric_A,
    proct_eq_3band_parametric_A,
    proct_eq_6band,
    proct_eq_morph,
    proct_eq_tilt,
    proct_comb1,
    proct_fx_bitcrusher,
    proct_fx_distortion1,
    proct_fx_clipper,
    proct_fx_slewer,
    proct_fx_limiter,
    proct_fx_gate,
    proct_fx_microgate,
    proct_fx_ringmod,
    proct_fx_freqshift,
    proct_fx_waveshaper,
    proct_fx_freqshiftmod,
    proct_fx_widener,
    proct_fx_simple_delay,
    proct_fmfilter,
    proct_noise_am,
    proct_osc_phasemod, // last part/fx

    proct_lifted_reverb1,
    proct_lifted_reverb2,
    proct_lifted_delay,
    proct_lifted_flanger,

    proct_fx_treemonster,
    proct_osc_VA,
    proct_osc_EBWaveforms,
    proct_osc_saw,
    proct_osc_sineplus,
    proct_osc_tiltnoise,
    proct_osc_correlatednoise, // last zone
    proct_num_types,
};

static constexpr const char *noneDisplayName{"None"};

bool isProcessorImplemented(ProcessorType id); // choice: return 'true' for none
const char *getProcessorName(ProcessorType id);
const char *getProcessorStreamingName(ProcessorType id);
const char *getProcessorDisplayGroup(ProcessorType id);
float getProcessorDefaultMix(ProcessorType id);
bool getProcessorGroupOnly(ProcessorType id);
int16_t getProcessorStreamingVersion(ProcessorType id);
using remapFn_t = void (*)(int16_t, float *const, int *const);
remapFn_t getProcessorRemapParametersFromStreamingVersion(ProcessorType id);
std::optional<ProcessorType> fromProcessorStreamingName(const std::string &s);

struct ProcessorDescription
{
    ProcessorType id;
    std::string streamingName{"ERROR"}, displayName{"ERROR"}, displayGroup{"ERROR"};
    bool groupOnly{false};
};

typedef std::vector<ProcessorDescription> processorList_t;
processorList_t getAllProcessorDescriptions();

/**
 * If you choose to spawnProcessorOnto you need a block at least this size.
 * This should be a multiple of 16 if you enlarge it.
 */
static constexpr size_t processorMemoryBufferSize{1024 * 20};

struct ProcessorStorage
{
    ProcessorStorage()
    {
        std::fill(floatParams.begin(), floatParams.end(), 0.f);
        std::fill(intParams.begin(), intParams.end(), 0);
    }

    ProcessorType type{proct_none};
    static constexpr float maxOutputDB{18.f};
    static constexpr float maxOutputAmp{7.943282347242815}; //  std::pow(10.f, maxOutputDB / 20.0);
    static constexpr float zeroDbAmp{0.5011872336272724};
    float mix{0}, outputCubAmp{zeroDbAmp}; // thats 1.0 / cubrt(maxOutputAmp)
    std::array<float, maxProcessorFloatParams> floatParams;
    std::array<bool, maxProcessorFloatParams> deactivated{};
    std::array<int32_t, maxProcessorIntParams> intParams;
    bool isActive{true};
    bool isKeytracked{false};
    int previousIsKeytracked{-1}; // make this an int and -1 means don't know previous
    bool isTemposynced{false};
    int16_t streamingVersion{-1};

    bool procTypeConsistent{true};

    bool operator==(const ProcessorStorage &other) const
    {
        return (type == other.type && mix == other.mix && floatParams == other.floatParams &&
                intParams == other.intParams && isActive == other.isActive);
    }
    bool operator!=(const ProcessorStorage &other) const { return !(*this == other); }
};

struct ProcessorControlDescription
{
    ProcessorControlDescription() = default;
    ~ProcessorControlDescription() = default;

    ProcessorType type{proct_none};
    std::string typeDisplayName{noneDisplayName};

    bool requiresConsistencyCheck{false};
    bool supportsKeytrack{false};
    bool supportsTemposync{false};

    int numFloatParams{0}; // between 0 and max
    std::array<sst::basic_blocks::params::ParamMetaData, maxProcessorFloatParams>
        floatControlDescriptions;

    int numIntParams{0}; // between 0 and max
    std::array<sst::basic_blocks::params::ParamMetaData, maxProcessorIntParams>
        intControlDescriptions;
};

struct Processor : MoveableOnly<Processor>, SampleRateSupport
{
  public:
    virtual ~Processor() = default;

  protected:
    Processor() {}
    ProcessorType myType{proct_none};

    template <typename T>
    void setupProcessor(T *that, ProcessorType t, engine::MemoryPool *mp,
                        const ProcessorStorage &ps, float *fp, int *ip, bool needsMetadata);

  public:
    bool bypassAnyway{false};

    size_t preReserveSize[16]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    size_t preReserveSingleInstanceSize[16]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    ProcessorType getType() const { return myType; }
    std::string getName() const { return getProcessorName(getType()); }

    virtual ProcessorControlDescription getControlDescription() const;
    int getFloatParameterCount() const { return floatParameterCount; }
    virtual size_t getIntParameterCount() const { return intParameterCount; }

    // TODO: Review and rename everything below here once the procesors are all ported

    virtual bool init_freq_graph() { return false; } // old z-plot honk (visa p� waveformdisplay)
    virtual float get_freq_graph(float f) { return 0; }

    virtual void init_params() {}
    virtual void init() {}

    virtual bool isKeytracked() const { return false; }
    virtual bool setKeytrack(bool b) { return false; }
    virtual bool getDefaultKeytrack() { return false; }

    virtual bool supportsMakingParametersConsistent() { return false; }
    virtual bool makeParametersConsistent() { return false; }

    virtual void resetMetadata() { assert(false); }

    double *tempoPointer{nullptr};
    virtual void setTempoPointer(double *t) { tempoPointer = t; }
    virtual double *getTempoPointer() const { return tempoPointer; }

    virtual int16_t getStreamingVersion() const
    {
        assert(false);
        return -1;
    }

    /*
     * The default behavior of a processor is stereo -> stereo and all
     * processors must implement process_stereo.
     */
    virtual void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                                float pitch) = 0;

    /*
     * Mono/Stereo processing is slightly changed from SC2. Each
     * processor has a pair of data methods 'canProcessMono' and
     * 'monoInputCreatesStereoOutput'. If canProcessMono is true
     * the processor will implement process_mono which takes a single
     * input stream. If monoInputCreatesStereoOutput is true that
     * will populate a left and right buffer, otherwise it will just
     * populate a left buffer.
     */
    virtual bool canProcessMono() { return false; }
    virtual bool monoInputCreatesStereoOutput() { return false; }
    virtual void process_monoToMono(float *datain, float *dataoutL, float pitch) { assert(false); }
    virtual void process_monoToStereo(float *datain, float *dataoutL, float *dataoutR, float pitch)
    {
        assert(false);
    }

    virtual void suspend() {}

    /*
     * Tail length in samples
     */
    virtual int tail_length() { return 0; }

    float modulation_output; // processors can use this to output modulation data to the matrix

    // move these back to protected and friend the adapter when done
    // Much of it can also be removed once we are all in voice-effects
    // land
    //
    // protected:
    engine::MemoryPool *memoryPool{nullptr};
    float *param{nullptr};
    int *iparam{nullptr};
    const bool *deactivated{nullptr};
    const bool *temposync{nullptr};
    int floatParameterCount{0};
    std::array<sst::basic_blocks::params::ParamMetaData, maxProcessorFloatParams>
        floatParameterMetaData;

    int intParameterCount{0};
    std::array<sst::basic_blocks::params::ParamMetaData, maxProcessorIntParams>
        intParameterMetaData;
};

/**
 * Spawn with in-place new onto a pre-allocated block. The memory must
 * be a 16byte aligned block of at least size processorMemoryBufferSize.
 */
Processor *spawnProcessorInPlace(ProcessorType id, engine::MemoryPool *mp, uint8_t *memory,
                                 size_t memorySize, const ProcessorStorage &ps, float *f, int *i,
                                 bool oversample, bool needsMetaData);

/**
 * Whetner a processor is spawned in place or onto fresh memory, release it here.
 * We assume that all processors which *can* spawn placement new did.
 */
void unspawnProcessor(Processor *f);

typedef uint8_t unimpl_t;
template <ProcessorType ft> struct ProcessorImplementor
{
    typedef unimpl_t T;
    typedef unimpl_t TOS;
};

template <ProcessorType ft> struct ProcessorDefaultMix
{
    static constexpr float defaultMix{1.0};
};

template <ProcessorType ft> struct ProcessorForGroupOnly
{
    static constexpr bool isGroupOnly{false};
};

} // namespace scxt::dsp::processor

SC_DESCRIBE(scxt::dsp::processor::ProcessorStorage,
            SC_FIELD_COMPUTED(mix,
                              {
                                  auto dr = dsp::processor::getProcessorDefaultMix(payload.type);
                                  return datamodel::pmd().asPercent().withName("Mix").withDefault(
                                      dr);
                              });
            SC_FIELD(outputCubAmp,
                     pmd()
                         .asCubicDecibelUpTo(scxt::dsp::processor::ProcessorStorage::maxOutputDB)
                         .withName("Output")););

#endif // __SCXT_DSP_PROCESSOR_PROCESSOR_H
