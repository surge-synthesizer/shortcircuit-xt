/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#include <cstdint>
#include <array>
#include <optional>
#include <vector>
#include <utility>
#include "datamodel/parameter.h"
#include "utils.h"

namespace scxt::dsp::processor
{

static constexpr size_t maxProcessorFloatParams{9};
static constexpr size_t maxProcessorIntParams{2};
static constexpr size_t processorLabelSize{32};

/**
 * ProcessorType Type: These are streamed as strings using the toStreamingName / frommStreamingName
 * so we can reorder them. The only requirement is they are contiguous
 * (so don't assign values) and the last is the proct_num_types
 */
enum ProcessorType
{
    proct_none = 0,
    proct_e_delay, // First part and fx
    proct_e_reverb,
    proct_e_chorus,
    proct_e_phaser,
    proct_e_rotary,
    proct_e_fauxstereo,
    proct_e_fsflange,
    proct_e_fsdelay,
    proct_biquadSBQ, // First zone
    proct_SuperSVF,

    proct_moogLP4sat,
    proct_eq_2band_parametric_A,
    proct_eq_6band,
    proct_comb1,
    proct_fx_bitfucker,
    proct_fx_distortion1,
    proct_fx_clipper,
    proct_fx_slewer,
    proct_fx_stereotools,
    proct_fx_limiter,
    proct_fx_gate,
    proct_fx_microgate,
    proct_fx_ringmod,
    proct_fx_freqshift,
    proct_fx_phasemod, // last part/fx

    proct_fx_treemonster,
    proct_osc_pulse,
    proct_osc_pulse_sync,
    proct_osc_saw,
    proct_osc_sin, // last zone
    proct_num_types,
};

/**
 * Can we in-place new if we are handed a buffer of at least processorMemoryBufferSize?
 */
bool canInPlaceNew(ProcessorType id);

/**
 * Can this processor be used in a zone, part, or FX position
 */
bool isProcessorImplemented(ProcessorType id); // choice: return 'true' for none
bool isZoneProcessor(ProcessorType id);
bool isPartProcessor(ProcessorType id);
bool isFXProcessor(ProcessorType id);
const char *getProcessorName(ProcessorType id);
const char *getProcessorStreamingName(ProcessorType id);
std::optional<ProcessorType> fromProcessorStreamingName(const std::string &s);

struct ProcessorDescription
{
    ProcessorType id;
    std::string streamingName{"ERROR"}, displayName{"ERROR"};
    bool isZone{false}, isPart{false}, isFX{false};
};

typedef std::vector<ProcessorDescription> processorList_t;
processorList_t getAllProcessorDescriptions();

/**
 * If you choose to spawnProcessorOnto you need a block at least this size.
 * This should be a multiple of 16 if you enlarge it.
 */
static constexpr size_t processorMemoryBufferSize{1028 * 8};

static constexpr int tailInfinite = 0x1000000;

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
    std::array<int, maxProcessorIntParams> intParams;
    bool isBypassed{false};

    bool operator==(const ProcessorStorage &other) const
    {
        return (type == other.type && mix == other.mix && floatParams == other.floatParams &&
                intParams == other.intParams && isBypassed == other.isBypassed);
    }
    bool operator!=(const ProcessorStorage &other) const { return !(*this == other); }
};

struct ProcessorControlDescription
{
    ProcessorControlDescription() = default;
    ~ProcessorControlDescription() = default;

    ProcessorType type;
    std::string typeDisplayName{};

    int numFloatParams{0}; // between 0 and max
    std::array<std::string, maxProcessorFloatParams> floatControlNames;
    std::array<datamodel::ControlDescription, maxProcessorFloatParams> floatControlDescriptions;

    int numIntParams{0}; // between 0 and max
    std::array<std::string, maxProcessorIntParams> intControlNames;
    datamodel::ControlDescription intControlDescriptions[maxProcessorIntParams];
};

struct Processor : MoveableOnly<Processor>, SampleRateSupport
{
  public:
    virtual ~Processor() = default;

  protected:
    Processor(ProcessorType t, float *p, int32_t *ip = 0, bool stereo = true)
        : myType(t), param(p), iparam(ip), is_stereo(stereo)
    {
    }
    ProcessorType myType;

  public:
    ProcessorType getType() { return myType; }
    std::string getName() { return getProcessorName(getType()); }

    ProcessorControlDescription getControlDescription();

    // TODO: Review and rename everything below here once the procesors are all ported
    int get_parameter_count() { return parameter_count; }
    char *get_parameter_label(int p_id);
    char *get_parameter_ctrlmode_descriptor(int p_id);
    virtual int get_ip_count() { return 0; }
    virtual const char *get_ip_label(int ip_id) { return (""); }
    virtual int get_ip_entry_count(int ip_id) { return 0; }
    virtual const char *get_ip_entry_label(int ip_id, int c_id) { return (""); }

    virtual bool init_freq_graph() { return false; } // old z-plot honk (visa p� waveformdisplay)
    virtual float get_freq_graph(float f) { return 0; }

    virtual void init_params() {}
    virtual void init() {}
    virtual void process(float *datain, float *dataout, float pitch) {}
    virtual void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                                float pitch)
    {
    }
    // processors are required to be able to process stereo blocks if stereo is true in the
    // constructor
    virtual void suspend() {}
    virtual int tail_length() { return 1000; }

    float modulation_output; // processors can use this to output modulation data to the matrix

  protected:
    float *param{nullptr};
    int *iparam{nullptr};
    float lastparam[maxProcessorFloatParams];
    int lastiparam[maxProcessorIntParams];
    int parameter_count{0};
    char ctrllabel[maxProcessorFloatParams][processorLabelSize];
    datamodel::ControlDescription ctrlmode_desc[maxProcessorFloatParams];
    bool is_stereo{true};

    void setStr(char target[processorLabelSize], const char *v)
    {
        strncpy(target, v, processorLabelSize);
        target[processorLabelSize - 1] = '\0';
    }
};

/**
 * Spawn with in-place new onto a pre-allocated block. The memory must
 * be a 16byte aligned block of at least size processorMemoryBufferSize.
 */
Processor *spawnProcessorInPlace(ProcessorType id, uint8_t *memory, size_t memorySize, float *fp,
                                 int *ip, bool stereo);

/**
 * Spawn a processors, potentially allocating memory. Call this if canInPlaceNew
 * returns false.
 *
 * TODO: Make it so we can remove this
 */
Processor *spawnProcessorAllocating(int id, float *fp, int *ip, bool stereo);

/**
 * Whetner a processor is spawned in place or onto fresh memory, release it here.
 * We assume that all processors which *can* spawn placement new did.
 */
void unspawnProcessor(Processor *f);

} // namespace scxt::dsp::processor
#endif // __SCXT_DSP_PROCESSOR_PROCESSOR_H
