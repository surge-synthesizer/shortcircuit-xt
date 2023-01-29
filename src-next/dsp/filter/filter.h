//
// Created by Paul Walker on 1/31/23.
//

#ifndef __SCXT_DSP_FILTER_FILTER_H
#define __SCXT_DSP_FILTER_FILTER_H

#include <cstdint>
#include "datamodel/parameter.h"
#include "utils.h"

namespace scxt::dsp::filter
{

static constexpr size_t maxFilterFloatParams{9};
static constexpr size_t maxFilterIntParams{2};
static constexpr size_t filterLabelSize{32};

/**
 * Filter Type: These are streamed so their order matters
 */
enum FilterType
{
    ft_none = 0,
    ft_e_delay, // First part and fx filter
    ft_e_reverb,
    ft_e_chorus,
    ft_e_phaser,
    ft_e_rotary,
    ft_e_fauxstereo,
    ft_e_fsflange,
    ft_e_fsdelay,
    ft_biquadSBQ, // First zone filter
    ft_SuperSVF,

    ft_moogLP4sat,
    ft_eq_2band_parametric_A,
    ft_eq_6band,
    ft_comb1,
    ft_fx_bitfucker,
    ft_fx_distortion1,
    ft_fx_clipper,
    ft_fx_slewer,
    ft_fx_stereotools,
    ft_fx_limiter,
    ft_fx_gate,
    ft_fx_microgate,
    ft_fx_ringmod,
    ft_fx_freqshift,
    ft_fx_phasemod, // last part/fx filter


    ft_fx_treemonster,
    ft_osc_pulse,
    ft_osc_pulse_sync,
    ft_osc_saw,
    ft_osc_sin, // last zone filter
    ft_num_types,
};

/**
 * Can we in-place new if we are handed a buffer of at least filterMemoryBufferSize?
 */
bool canInPlaceNew(FilterType id);

/**
 * Can this filter be used in a zone, part, or FX position
 */
bool isFilterImplemented(FilterType id); // choice: return 'true' for none
bool isZoneFilter(FilterType id);
bool isPartFilter(FilterType id);
bool isFXFilter(FilterType id);
const char *getFilterName(FilterType id);

/**
 * If you choose to spawnFilterOnto you need a block at least this size.
 * This should be a multiple of 16 if you enlarge it.
 */
static constexpr size_t filterMemoryBufferSize{1028 * 8};

static constexpr int tailInfinite = 0x1000000;

struct Filter : NonCopyable<Filter>, SampleRateSupport
{
  public:
    virtual ~Filter() = default;

  protected:
    Filter(FilterType t, float *p, int32_t *ip = 0, bool stereo = true)
        : myType(t), param(p), iparam(ip), is_stereo(stereo)
    {
    }
    FilterType myType;

  public:
    FilterType getType() { return myType; }
    std::string getName() { return getFilterName(getType()); }

    // TODO: Review and rename everything below here once the filters are all ported
    int get_parameter_count() { return parameter_count; }
    char *get_parameter_label(int p_id);
    char *get_parameter_ctrlmode_descriptor(int p_id);
    virtual int get_ip_count() { return 0; }
    virtual const char *get_ip_label(int ip_id) { return (""); }
    virtual int get_ip_entry_count(int ip_id) { return 0; }
    virtual const char *get_ip_entry_label(int ip_id, int c_id) { return (""); }

    virtual bool init_freq_graph() { return false; } // filter z-plot honk (visa p� waveformdisplay)
    virtual float get_freq_graph(float f) { return 0; }

    virtual void init_params() {}
    virtual void init() {}
    virtual void process(float *datain, float *dataout, float pitch) {}
    virtual void process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                                float pitch)
    {
    }
    // filters are required to be able to process stereo blocks if stereo is true in the constructor
    virtual void suspend() {}
    virtual int tail_length() { return 1000; }

    float modulation_output; // filters can use this to output modulation data to the matrix

  protected:
    float *param{nullptr};
    int *iparam{nullptr};
    float lastparam[maxFilterFloatParams];
    int lastiparam[maxFilterIntParams];
    int parameter_count{0};
    datamodel::ControlMode ctrlmode[maxFilterFloatParams];
    char ctrllabel[maxFilterFloatParams][filterLabelSize];
    datamodel::ControlDescription ctrlmode_desc[maxFilterFloatParams];
    bool is_stereo{true};

    void setStr(char target[filterLabelSize], const char *v)
    {
        strncpy(target, v, filterLabelSize);
        target[filterLabelSize - 1] = '\0';
    }
};

/**
 * Spawn with in-place new onto a pre-allocated block. The memory must
 * be a 16byte aligned block of at least size filterMemoryBufferSize.
 */
Filter *spawnFilterInPlace(FilterType id, uint8_t *memory, size_t memorySize, float *fp, int *ip,
                           bool stereo);

/**
 * Spawn a filter, potentially allocating memory. Call this if canInPlaceNew
 * returns false.
 *
 * TODO: Make it so we can remove this
 */
Filter *spawnFilterAllocating(int id, float *fp, int *ip, bool stereo);

/**
 * Whetner a filter is spawned in place or onto fresh memory, release it here.
 */
void unspawnFilter(Filter *f);

} // namespace scxt::dsp::filter
#endif // __SCXT_DSP_FILTER_FILTER_H
