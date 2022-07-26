/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#pragma once

class sampler;
#include "filesystem/import.h"
#include "controllers.h"
#include "multiselect.h"
#include "sampler_state.h"
#include "infrastructure/logfile.h"
#include "browser/ContentBrowser.h"
#include <list>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <vt_dsp/lipol.h>
#include <sampler_wrapper_actiondata.h>
#include <readerwriterqueue.h>

#include <sstream>
#include <set>
#include <ostream>
#include "sampler_userdefaults.h"

class sample;
class sampler_voice;
class filter;

typedef int EditorClass;
typedef int WrapperClass;

class modmatrix;
class TiXmlElement;
class configuration;

struct voicestate
{
    bool active;
    unsigned char key, channel, part;
    uint32_t zone_id;
};

static constexpr int n_custom_controllers = 16;

enum external_controller_type
{
    extctrl_none = 0,
    extctrl_midicc,
    extctrl_midirpn,
    extctrl_midinrpn,
    extctrl_vstparam,
    n_ctypes,
};

const char ct_titles[n_ctypes][8] = {("none"), ("CC"), ("RPN"), ("NRPN"), ("VST")};

struct external_controller
{
    external_controller_type type;
    int number;
    char name[16];
};

class sampler
{
  public:
    // Aligned members
    float output alignas(16)[MAX_OUTPUTS << 1][BLOCK_SIZE],
        output_part alignas(16)[N_SAMPLER_PARTS << 1][BLOCK_SIZE],
        output_fx alignas(16)[N_SAMPLER_EFFECTS << 1][BLOCK_SIZE];
    struct alignas(16) partvoice
    {
        lipol_ps fmix1, fmix2, ampL, ampR, pfg, aux1L, aux1R, aux2L, aux2R;
        filter *pFilter[2];
        int last_ft[2];
        modmatrix *mm;
    } partv[N_SAMPLER_PARTS];
    struct alignas(16) multivoice
    {
        lipol_ps pregain, postgain;
        filter *pFilter[N_SAMPLER_EFFECTS];
        int last_ft[N_SAMPLER_EFFECTS];
    } multiv;
    float *output_ptr[MAX_OUTPUTS << 1];
    scxt::log::StreamLogger mLogger;

    // Public Interface

    sampler(EditorClass *editor, int NumOutputs, WrapperClass *effect = 0,
            scxt::log::LoggingCallback *cb = 0);
    virtual ~sampler(void);

    /*
     * User Directory
     */
    fs::path userDocumentDirectory;
    void setupUserDocumentDirectory();
    std::unique_ptr<scxt::defaults::Provider> defaultsProvider;

    /*
     * Associated content
     */
    scxt::content::ContentBrowser browser;

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
    void voice_off(uint32_t voice_id);
    void kill_notes(uint32_t zone_id);
    float *get_output_pointer(int id, int channel, int part); // internal
    bool get_key_name(char *str, int channel, int key);
    void process_audio();
    void process_part(int p);
    void process_global_effects();
    void processVUsAndPolyphonyUpdates();
    void part_check_filtertypes(int p, int f);
    void idle();

    std::string generateInternalStateView() const;

    // float output[outputs_total_max][BLOCK_SIZE];
    float vu_rms[MAX_OUTPUTS << 1], vu_peak[MAX_OUTPUTS << 1];
    int vu_time;

    int get_headroom() { return headroom; };
    void set_samplerate(float sr);
    bool zone_exist(int id);
    bool verify_zone_validity(int zone_id);

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
    bool free_zone(uint32_t zoneid);
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
        void Start(const fs::path &pFilename);
        void Stop();
        void SetPlayingState(bool State);

      public:
        sample_zone mZone;
        sample_part mPart;
        std::unique_ptr<sampler_voice> mpVoice;
        std::unique_ptr<sample> mpSample;
        sampler *mpParent{nullptr};
        bool mActive{false}, mAutoPreview{false};
        fs::path mFilename;
    };
    std::unique_ptr<Preview> mpPreview;
    float mPreviewLevel;
    bool mAutoPreview;
    bool mPreviewConfigChanged{false};

  public:
    /*
     * These are the data representations of the innards of the synth.
     */
    sample_zone zones[MAX_ZONES];
    sample_part parts[N_SAMPLER_PARTS];

    /*
     * Multi contains the global FX state. It configures at runtime 'multiv' which contains
     * the actual filter pointers. (In surge this would be called "MultiVoiceStorage" or some such)
     */
    sample_multi multi;

    int polyphony_cap;

  public:
    /*
     * We no longer have an editor object here to read things from.
     * But we do have a set of state the editor can update on the sampler
     * which will happen under a lock for non-realtime things like
     * saving patches.
     */
    struct EditorProxy
    {
        template <typename T> struct getset
        {
            std::mutex &mu;
            T val{};
            getset(std::mutex &m) : mu(m) {}
            void set(const T &v)
            {
                auto g = std::lock_guard<std::mutex>(mu);
                val = v;
            }
            T get() const
            {
                auto g = std::lock_guard<std::mutex>(mu);
                return val;
            }
        };

        std::mutex editorMutex;
        getset<fs::path> savepart_path{editorMutex};
        getset<fs::path> preview_path{editorMutex};
    } editorProxy;

    int editorpart, editorlayer, editorlfo, editormm;
    moodycamel::ReaderWriterQueue<actiondata> actionBuffer;

    std::string wrapperType{"Not Set"};

    bool toggled_samplereplace;

    std::shared_ptr<sample> samples[MAX_SAMPLES];
    int polyphony;
    int mNumOutputs;
    timedata time_data;
    // surge needed this so presume SC3 will too one day. For now make it a noop
    void resetStateFromTimeData() {}

    int VUrate, VUidx, lastSentPolyphony{-1};
    float automation[N_AUTOMATION_PARAMETERS];

    // AudioEffectX	*effect;
    std::unique_ptr<multiselect> selected;
    std::recursive_mutex cs_patch, cs_gui, cs_engine;
    std::unique_ptr<configuration> conf;
    external_controller externalControllers[n_custom_controllers];

    char sample_replace_filename[256];
    char keystate[16][128];
    float adsr[4], mastergain;
    std::string customcontrollers[16];
    bool customcontrollers_bp[16];
    float controllers[16 * n_controllers];
    float controllers_target[16 * n_controllers];

    bool volatile AudioHalted; // don't care to wait for the process thread
  protected:
    bool zone_exists[MAX_ZONES];
    bool holdengine;
    sampler_voice *voices[MAX_VOICES];
    voicestate voice_state[MAX_VOICES];
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
