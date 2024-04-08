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
 * I defined process as{} and init as {parameter_count=0} so I had a stub
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
    proct_biquadSBQ, // First zone
    proct_SuperSVF,
    proct_CytomicSVF,

    proct_moogLP4sat,
    proct_eq_1band_parametric_A,
    proct_eq_2band_parametric_A,
    proct_eq_3band_parametric_A,
    proct_eq_6band,
    proct_eq_morph,
    proct_comb1,
    proct_fx_bitcrusher,
    proct_fx_distortion1,
    proct_fx_clipper,
    proct_fx_slewer,
    proct_fx_stereotools,
    proct_fx_limiter,
    proct_fx_gate,
    proct_fx_microgate,
    proct_fx_ringmod,
    proct_fx_freqshift,
    proct_fx_waveshaper,
    proct_fx_pitchring,
    proct_fx_fauxstereo,
    proct_osc_phasemod, // last part/fx

    proct_fx_treemonster,
    proct_osc_pulse,
    proct_osc_correlatednoise,
    proct_osc_pulse_sync,
    proct_osc_saw,
    proct_osc_sin, // last zone
    proct_num_types,
};

bool isProcessorImplemented(ProcessorType id); // choice: return 'true' for none
const char *getProcessorName(ProcessorType id);
const char *getProcessorStreamingName(ProcessorType id);
const char *getProcessorDisplayGroup(ProcessorType id);
std::optional<ProcessorType> fromProcessorStreamingName(const std::string &s);

struct ProcessorDescription
{
    ProcessorType id;
    std::string streamingName{"ERROR"}, displayName{"ERROR"}, displayGroup{"ERROR"};
};

typedef std::vector<ProcessorDescription> processorList_t;
processorList_t getAllProcessorDescriptions();

/**
 * If you choose to spawnProcessorOnto you need a block at least this size.
 * This should be a multiple of 16 if you enlarge it.
 */
static constexpr size_t processorMemoryBufferSize{1028 * 16};

struct ProcessorStorage
{
    ProcessorStorage()
    {
        std::fill(floatParams.begin(), floatParams.end(), 0.f);
        std::fill(intParams.begin(), intParams.end(), 0);
    }

    ProcessorType type{proct_none};
    float mix{0};
    std::array<float, maxProcessorFloatParams> floatParams;
    std::array<int32_t, maxProcessorIntParams> intParams;
    bool isActive{true};

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
    std::string typeDisplayName{"Off"};

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
    Processor(ProcessorType t, engine::MemoryPool *mp, float *p, int32_t *ip = 0)
        : myType(t), memoryPool(mp), param(p), iparam(ip)
    {
    }
    ProcessorType myType{proct_none};

    template <typename T>
    void setupProcessor(T *that, ProcessorType t, engine::MemoryPool *mp, float *fp, int *ip);

  public:
    size_t preReserveSize[16]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    ProcessorType getType() const { return myType; }
    std::string getName() const { return getProcessorName(getType()); }

    ProcessorControlDescription getControlDescription() const;
    int getFloatParameterCount() const { return parameter_count; }
    virtual size_t getIntParameterCount() const { return 0; }
    virtual std::string getIntParameterLabel(int ip_id) const { return (""); }
    virtual size_t getIntParameterChoicesCount(int ip_id) const { return 0; }
    virtual std::string getIntParameterChoicesLabel(int ip_id, int c_id) const { return (""); }

    // TODO: Review and rename everything below here once the procesors are all ported

    virtual bool init_freq_graph() { return false; } // old z-plot honk (visa p� waveformdisplay)
    virtual float get_freq_graph(float f) { return 0; }

    virtual void init_params() {}
    virtual void init() {}

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
    virtual void process_mono(float *datain, float *dataoutL, float *dataoutR, float pitch)
    {
        assert(false);
    }

    // processors are required to be able to process stereo blocks if stereo is true in the
    // constructor
    virtual void suspend() {}
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
    int parameter_count{0};
    sst::basic_blocks::params::ParamMetaData ctrlmode_desc[maxProcessorFloatParams];
};

/**
 * Spawn with in-place new onto a pre-allocated block. The memory must
 * be a 16byte aligned block of at least size processorMemoryBufferSize.
 */
Processor *spawnProcessorInPlace(ProcessorType id, engine::MemoryPool *mp, uint8_t *memory,
                                 size_t memorySize, float *fp, int *ip, bool oversample);

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

} // namespace scxt::dsp::processor

SC_DESCRIBE(scxt::dsp::processor::ProcessorStorage,
            SC_FIELD(mix, datamodel::pmd().asPercent().withName("Mix")));
#endif // __SCXT_DSP_PROCESSOR_PROCESSOR_H
