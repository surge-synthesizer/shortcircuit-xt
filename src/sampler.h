//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#pragma once

class sampler;
#include "infrastructure/import_fs.h"
#include "controllers.h"
#include "multiselect.h"
#include "sampler_state.h"
#include "infrastructure/logfile.h"
#include <list>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <vt_dsp/lipol.h>
#include <sampler_wrapper_actiondata.h>

#include <sstream>
#include <set>
#include <ostream>

class sample;
class sampler_voice;
class filter;

#if TARGET_VST2
class AEffEditor;
class AudioEffectX;

typedef AEffEditor EditorClass;
typedef AudioEffectX WrapperClass;
#endif

#if TARGET_HEADLESS
typedef int EditorClass;
typedef int WrapperClass;

#endif

class modmatrix;
class TiXmlElement;
class configuration;
class vt_LockFree;

struct voicestate
{
    bool active;
    unsigned char key, channel, part;
    uint32 zone_id;
};

class sampler
{
  public:
    // Aligned members
    float output alignas(16)[max_outputs << 1][block_size],
        output_part alignas(16)[n_sampler_parts << 1][block_size],
        output_fx alignas(16)[n_sampler_effects << 1][block_size];
    struct alignas(16) partvoice
    {
        lipol_ps fmix1, fmix2, ampL, ampR, pfg, aux1L, aux1R, aux2L, aux2R;
        filter *pFilter[2];
        int last_ft[2];
        modmatrix *mm;
    } partv[n_sampler_parts];
    struct alignas(16) multivoice
    {
        lipol_ps pregain, postgain;
        filter *pFilter[n_sampler_effects];
        int last_ft[n_sampler_effects];
    } multiv;
    float *output_ptr[max_outputs << 1];
    scxt::log::StreamLogger mLogger;

    // Public Interface

    sampler(EditorClass *editor, int NumOutputs, WrapperClass *effect = 0,
            scxt::log::LoggingCallback *cb = 0);
    virtual ~sampler(void);

    bool loadUserConfiguration(const fs::path &configFile);
    bool saveUserConfiguration(const fs::path &configFile);

    /*
     * Midi Messages
     */
    virtual bool PlayNote(char channel, char key, char velocity, bool is_release = false,
                          char detune = 0);
    void PitchBend(char channel, int value);
    void ChannelAftertouch(char channel, int value);
    void ChannelController(char channel, int cc, int value);
    void ReleaseNote(char channel, char key, char velocity);
    void AllNotesOff();

    void play_zone(int zone_id);
    void release_zone(int zone_id);
    void voice_off(uint32 voice_id);
    void kill_notes(uint32 zone_id);
    float *get_output_pointer(int id, int channel, int part); // internal
    bool get_key_name(char *str, int channel, int key);
    void process_audio();
    void process_part(int p);
    void process_global_effects();
    void processVUs();
    void part_check_filtertypes(int p, int f);
    void idle();

    std::string generateInternalStateView() const;

    // float output[outputs_total_max][block_size];
    float vu_rms[max_outputs << 1], vu_peak[max_outputs << 1];
    int vu_time;

    int get_headroom() { return headroom; };
    void set_samplerate(float sr);
    bool zone_exist(int id);
    bool verify_zone_validity(int zone_id);

    // automation interface

    unsigned int auto_get_n_parameters();
    const char *auto_get_parameter_name(unsigned int);
    const char *auto_get_parameter_display(unsigned int);
    float auto_get_parameter_value(unsigned int);
    void auto_set_parameter_value(unsigned int, float);

    // Interface to GUI wrapeprs
    struct WrapperListener
    {
        virtual ~WrapperListener() = default;
        virtual void receiveActionFromProgram(const actiondata &ad) = 0;
    };

    std::set<WrapperListener *> wrappers;
    void registerWrapperForEvents(WrapperListener *l) { wrappers.insert(l); }
    void unregisterWrapperForEvents(WrapperListener *l) { wrappers.erase(l); }
    void postEventsFromWrapper(const actiondata &ad);
    void postEventsToWrapper(const actiondata &ad, bool ErrorIfClosed = true);
    void processWrapperEvents();

    void post_initdata();
    void post_zonedata();
    void post_kgvdata();
    void post_samplelist();
    void post_initdata_mm(int);
    void post_initdata_mm_part();
    void post_zone_filterdata(int, int, bool send_data = false);
    void post_part_filterdata(int, int, bool send_data = false);
    void post_multi_filterdata(int i, bool send_data = false);
    void post_control_range(actiondata ad, int id_start, int id_end, int subid_start = 0,
                            int subid_end = 0);
    void post_data_from_structure(char *pointr, int id_start, int id_end);
    void relay_data_to_structure(actiondata ad, char *pt);
    void set_editorpart(int p, int layer);
    void set_editorzone(int z);
    void track_zone_triggered(int z, bool state);
    void track_key_triggered(int ch, int key, int vel);

    // part management
    void multi_init();
    void part_init(int p, bool clear_zones = false, bool leave_outputs_intact = false);
    void part_clear_zones(int p);

    // zone & group management
    bool add_zone(const fs::path &filename, int *new_z = 0, char part = 0,
                  bool use_root_key = false);
    void InitZone(int zone_id);
    static void SInitZone(sample_zone *pZone);
    bool clone_zone(int zone_id, int *new_z, bool same_key = true);
    bool slice_to_zone(int zone_id, int slice);
    bool slices_to_zones(int zone_id);
    bool replace_zone(int z, const fs::path &filename);
    bool free_zone(uint32 zoneid);
    void update_zone_switches(int zone);
    bool get_sample_id(const fs::path &filename, int *s_id);
    int find_next_free_key(int part);
    int GetFreeSampleId();
    int GetFreeZoneId();
    int GetFreeVoiceId(int group_id = 0); // get a free voice id. kills an old voice if necessary
    int softkill_oldest_note(int group_id = 0);
    void update_highest_voice_id();

    int get_zone_poly(int zone);
    int get_group_poly(int zone);
    bool get_slice_state(int zone, int slice);
    bool is_key_down(int channel, int key);

    // File I/O
    bool is_multisample_file(const fs::path &filename);
    bool is_multisample_extension(const std::string &extension);
    bool load_akai_s6k_program(const fs::path &filename, char channel = 0, bool replace = true);
    bool parse_dls_preset(void *data, size_t datasize, char channel, int patch,
                          const fs::path &filename);
    bool load_sf2_preset(const fs::path &filename, int *new_g = 0, char channel = 0,
                         int patch = -1);
    bool load_sfz(const char *data, size_t datasize, const fs::path &path, int *new_g = 0,
                  char channel = 0);
    bool load_battery_kit(const fs::path &fileName, char channel = 0, bool replace = true);
    bool load_file(const fs::path &filename, int *new_g = 0, int *new_z = 0, bool *is_group = 0,
                   char channel = 0, int add_zones_to_groupid = 0, bool replace = false);
    // bool load_file(const fs::path &filename, char part, int *new_z=0);

    long save_all(void **data); // , int group_id=-1); ?
    void free_all();
    bool load_all(void *data, int datasize);
    bool load_all_from_xml(const void *data, int datasize, const fs::path &filename = fs::path(),
                           bool replace = true, int channel = -1);
    bool load_all_from_sc1_xml(const void *data, int datasize,
                               const fs::path &filename = fs::path(), bool replace = true,
                               int channel = -1);
    bool save_all_to_disk(const fs::path &filename);
    size_t save_part_as_xml(int part_id, const fs::path &filename, bool copy_samples = false);

    // The new RIFF based format for SC2
    bool LoadAllFromRIFF(const void *data, size_t datasize, bool replace = true, int channel = -1);
    size_t SaveAllAsRIFF(void **data, const fs::path &fn = fs::path(), int PartID = -1);

    // helper functions for load/save
    void recall_zone_from_element(TiXmlElement &element, sample_zone *zone, int revision = 2,
                                  configuration *conf = 0);
    void store_zone_as_element(TiXmlElement &element, sample_zone *zone, configuration *conf);

  private:
    void ChannelControllerForPart(int Part, int cc, int value);
    void SetCustomController(int Part, int ControllerIdx, float NormalizedValue);
    void RPNForPart(int Part, int RPN, bool IsNRPN, int value);

    //! Preview path.
    class Preview
    {
      public:
        Preview(timedata *pTD, sampler *pParent);
        ~Preview();
        void Start(const wchar_t *pFilename);
        void Stop();
        void SetPlayingState(bool State);

      public:
        sample_zone mZone;
        sample_part mPart;
        sampler_voice *mpVoice;
        sample *mpSample;
        sampler *mpParent;
        bool mActive, mAutoPreview;
        wchar_t mFilename[1024];
    } * mpPreview;

  public:
    /*
     * These are the data representations of the innards of the synth.
     */
    sample_zone zones[max_zones];
    sample_part parts[n_sampler_parts];

    /*
     * Multi contains the global FX state. It configures at runtime 'multiv' which contains
     * the actual filter pointers. (In surge this would be called "MultiVoiceStorage" or some such)
     */
    sample_multi multi;

    int polyphony_cap;

  public:
    int editorpart, editorlayer, editorlfo, editormm;
    vt_LockFree *ActionBuffer;

    bool toggled_samplereplace;

    sample *samples[max_samples];
    int polyphony;
    int mNumOutputs;
    timedata time_data;
    int VUrate, VUidx;
    float automation[n_automation_parameters];

    // AudioEffectX	*effect;
    multiselect *selected;
    std::recursive_mutex cs_patch, cs_gui, cs_engine;
    configuration *conf;
    char sample_replace_filename[256];
    char keystate[16][128];
    float adsr[4], mastergain;
    std::string customcontrollers[16];
    bool customcontrollers_bp[16];
    float controllers[16 * n_controllers];
    float controllers_target[16 * n_controllers];

    bool volatile AudioHalted; // don't care to wait for the process thread
  protected:
    bool zone_exists[max_zones];
    bool holdengine;
    sampler_voice *voices[max_voices];
    voicestate voice_state[max_voices];
    double headroom_linear;
    int headroom;
    bool hold[16];
    std::list<int> holdbuffer;
    void purge_holdbuffer();
    char nrpn[16][2], nrpn_v[16][2];
    char rpn[16][2], rpn_v[16][2];
    bool nrpn_last[16];
    int highest_voice_id, highest_group_id;

    void *chunkDataPtr, *dbSampleListDataPtr;
};
