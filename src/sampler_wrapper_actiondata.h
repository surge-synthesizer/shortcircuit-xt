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

/*
 * THIS DOCUMENTATION is still getting written. But it's a start
 *
 * So the question arises: how should the audio thread and the UI thread interact?
 * scxt has a clear model
 *
 * 1. It has a lockfree buffer for small messages (class actiondata)
 * 2. A client which is hosting the engine (which we call a "Wrapper") can send messages
 * 3. A wrapper can receive messages
 * 4. The engine can generate messages of its own accord which are destined to the wrapper
 *
 * In sampler.h you will see `struct WrapperListener` which has a single method
 * 'receiveActionFromProgram(actiondata)' and you will see a registerWrapperListener
 * and unregisterWrapperListener.
 *
 * You will also see a method postEventsFromWrapper and postEventsToWrapper
 *
 * So great. What's the pattern then?
 *
 * 1. A wrapper starts up and posts an event. Here's the code from the JUCE wrapper
 * when we open the UI
 *
 * actiondata ad;
 * ad.actiontype = vga_openeditor;
 * audioProcessor.sc3->postEventsFromWrapper(ad);
 *
 * 2. That pushes that message onto a lockfree queue in the sampler
 * 3. The audio thread picks up the message and processes it. In this case
 *    openeditor is a pretty chunky message. This is all done in sampler_wrapper_interaction.cpp
 * 4. That results in messages being posted back to the wrapper
 * 5. The wrapper then responds to those messages asynchronously and does what it wants
 *    with them. (Probably: Update the UI is what it wanst to do).
 *
 * Alright that's a pretty basic model. So the only thing we need to understand
 * then are what are the messages wrapper -> engine, engine -> wrapper, and the thread
 * patterns of both.
 *
 * Lets do thred patterns first
 *
 * 1. The wrapper can call `postEventsFromWrapper` from any thread it wants
 * 2. The engine will process events on the audio thread
 * 3. When the engine calls the wrapper back it will call it on the Audio thread if audio is
 *    running and the ui thread if not. So you *have* to handle the case that your wrapper
 *    will be called on non-UI thread appropriately (basically: Blast an async message away)
 *
 * Then the messages. The messages are always of type messageData. Wrapper -> engine are the vga_
 * types basically and engine->wrapper are the ip types it seems. But the protocol is really just
 * documented by the code in sampler_wrapper_actiondata.h. Right now we are passing objects with
 * stuff in them and slowly will document what they do.
 */

#pragma once
#include <cassert>
#include <variant>
#include "interaction_parameters.h"
#include "filesystem/import.h"

enum VUnInitialized
{
    vuininit = 0
};

enum VGet
{
    vget_mousedown = 0,
    vget_mouseup,
    vget_mousemove,
    vget_mouseenter,
    vget_mouseleave,
    vget_mousedblclick,
    vget_mousewheel,
    vget_character,
    vget_keypress,
    vget_keyrepeat,
    vget_keyrelease,
    vget_hotkey,
    vget_idle,
    vget_process_actionbuffer,
    vget_dragfiles,
    vget_dropfiles,
    vget_deactivate
};

enum VMouse
{
    vgm_LMB = 0x1,
    vgm_RMB = 0x2,
    vgm_shift = 0x4,
    vgm_control = 0x8,
    vgm_MMB = 0x10,
    vgm_MB4 = 0x20,
    vgm_MB5 = 0x40,
    vgm_alt = 0x80
};

enum VCursor
{
    cursor_arrow = 0,
    cursor_hand_point,
    cursor_hand_pan,
    cursor_wait,
    cursor_forbidden,
    cursor_move,
    cursor_text,
    cursor_sizeNS,
    cursor_sizeWE,
    cursor_sizeNESW,
    cursor_sizeNWSE,
    cursor_copy,
    cursor_zoomin,
    cursor_zoomout,
    cursor_vmove,
    cursor_hmove
};

enum VAlign
{
    vg_align_top = -1,
    vg_align_left = -1,
    vg_align_center = 0,
    vg_align_bottom = 1,
    vg_align_right = 1,
};

enum VType
{
    vgvt_int = 0,
    vgvt_bool,
    vgvt_float
};

enum VAction
{
    vga_nothing = 0,
    vga_floatval,
    vga_intval,
    vga_intval_inc,
    vga_intval_dec,
    vga_boolval,
    vga_beginedit,
    vga_endedit,
    vga_load_dropfiles,
    vga_load_patch,
    vga_text,
    vga_click,
    vga_exec_external,
    vga_url_external,
    vga_doc_external,
    vga_disable_state,
    vga_stuck_state,
    vga_hide,
    vga_label,
    vga_temposync,
    vga_filter,
    vga_datamode,
    vga_menu,
    vga_entry_add,
    vga_entry_add_ival_from_self,
    vga_entry_add_ival_from_self_with_id,
    vga_entry_replace_label_on_id,
    vga_entry_setactive,
    vga_entry_clearall,
    vga_select_zone_clear,
    vga_select_zone_primary,
    vga_select_zone_secondary,
    vga_select_zone_previous,
    vga_select_zone_next,
    vga_zone_playtrigger,
    vga_deletezone,
    vga_createemptyzone,
    vga_clonezone,
    vga_clonezone_next,
    vga_movezonetopart,
    vga_movezonetolayer,
    vga_zonelist_clear,
    vga_zonelist_populate,
    vga_zonelist_done,
    vga_zonelist_mode,
    vga_toggle_zoom,
    vga_note,
    vga_audition_zone,
    vga_request_refresh,
    vga_set_zone_keyspan,
    vga_set_zone_keyspan_clone,
    vga_openeditor,
    vga_closeeditor,
    vga_wavedisp_sample,
    vga_wavedisp_multiselect,
    vga_wavedisp_plot,
    vga_wavedisp_editpoint,
    vga_steplfo_repeat,
    vga_steplfo_shape,
    vga_steplfo_data,
    vga_steplfo_data_single,
    vga_browser_listptr,
    vga_browser_entry_next,
    vga_browser_entry_prev,
    vga_browser_entry_load,
    vga_browser_category_next,
    vga_browser_category_prev,
    vga_browser_category_parent,
    vga_browser_category_child,
    vga_browser_preview_start,
    vga_browser_preview_stop,
    vga_browser_is_refreshing,
    vga_inject_database,
    vga_database_samplelist,
    vga_save_patch,
    vga_save_multi,
    vga_vudata,
    vga_set_range_and_units // see sampler_parameter_ranges.h
};

enum VColor
{
    // color palette entries
    col_standard_bg = 0,
    col_standard_text,
    col_standard_bg_disabled,
    col_standard_text_disabled,
    col_selected_bg,
    col_selected_text,
    col_alt_bg,
    col_alt_text,
    col_frame,
    col_tintarea,
    col_nowavebg,
    col_slidertray1,
    col_slidertray2,
    col_misclabels,
    col_LFOgrid,
    col_LFOBG1,
    col_LFOBG2,
    col_LFObars,
    col_LFOcurve,
    col_list_bg,
    col_list_text,
    col_list_text_unassigned,
    col_list_text_keybed,
    col_vu_back,
    col_vu_front,
    col_vu_overload
};

enum VKbm
{
    kbm_autodetect = 0,
    kbm_os,
    kbm_vst,
    kbm_globalhook,
    kbm_seperate_window
};

typedef std::variant<VUnInitialized, VGet, VMouse, VCursor, VAlign, VType, VAction, VColor, VKbm>
    actiontype_t;

union vg_pdata
{
    int i;
    bool b;
    float f;
    void *ptr;
};

const int actiondata_maxstring = 13 * 4 - 1;

struct DropList
{
    struct File
    {
        fs::path p;
        int key_lo = 0, key_hi = 0, key_root = 0, vel_lo = 0, vel_hi = 0, database_id = 0;
    };
    std::vector<File> files;
};

struct actiondata
{
    actiontype_t actiontype;
    /*InteractionId*/ int id;
    int subid;

    union
    {
        int i[13];
        float f[13];
        char str[52];
        void *ptr[6]; // assume the pointer is 64-bit and always put the pointers in the first slots
                      // of the message

        DropList *dropList; // Ownership is with the processor now
    } data;

    // try to avoid any uninitialized mem issues
    actiondata()
    {
        actiontype = vuininit;
        id = ip_none;
        subid = 0;
        data.str[0] = 0;
    }

}; // 64 bytes of whatever you like.. should be plenty

template <class... Ts> struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

inline std::ostream &operator<<(std::ostream &stream, const actiontype_t &t)
{
    // clang-format off
    std::visit(overloaded{[&stream](auto arg) { stream << "Generic [" << arg << "]"; },
                                [&stream](VUnInitialized arg) { stream << "UnInitialized [" << arg << "]"; },
                                [&stream](VAction arg)
                                 {
                                    stream << "VAction [";
#define C(x)                                                                                       \
    case x:                                                                                        \
        stream << #x;                                                                              \
        break;

                              switch (arg)
                              {
                                  C(vga_floatval)
                                  C(vga_intval)
                                  C(vga_intval_inc)
                                  C(vga_intval_dec)
                                  C(vga_boolval)
                                  C(vga_beginedit)
                                  C(vga_endedit)
                                  C(vga_load_dropfiles)
                                  C(vga_load_patch)
                                  C(vga_text)
                                  C(vga_click)
                                  C(vga_exec_external)
                                  C(vga_url_external)
                                  C(vga_doc_external)
                                  C(vga_disable_state)
                                  C(vga_stuck_state)
                                  C(vga_hide)
                                  C(vga_label)
                                  C(vga_temposync)
                                  C(vga_filter)
                                  C(vga_datamode)
                                  C(vga_menu)
                                  C(vga_entry_add)
                                  C(vga_entry_add_ival_from_self)
                                  C(vga_entry_add_ival_from_self_with_id)
                                  C(vga_entry_replace_label_on_id)
                                  C(vga_entry_setactive)
                                  C(vga_entry_clearall)
                                  C(vga_select_zone_clear)
                                  C(vga_select_zone_primary)
                                  C(vga_select_zone_secondary)
                                  C(vga_select_zone_previous)
                                  C(vga_select_zone_next)
                                  C(vga_zone_playtrigger)
                                  C(vga_deletezone)
                                  C(vga_createemptyzone)
                                  C(vga_clonezone)
                                  C(vga_clonezone_next)
                                  C(vga_movezonetopart)
                                  C(vga_movezonetolayer)
                                  C(vga_zonelist_clear)
                                  C(vga_zonelist_populate)
                                  C(vga_zonelist_done)
                                  C(vga_zonelist_mode)
                                  C(vga_toggle_zoom)
                                  C(vga_note)
                                  C(vga_audition_zone)
                                  C(vga_request_refresh)
                                  C(vga_set_zone_keyspan)
                                  C(vga_set_zone_keyspan_clone)
                                  C(vga_openeditor)
                                  C(vga_closeeditor)
                                  C(vga_wavedisp_sample)
                                  C(vga_wavedisp_multiselect)
                                  C(vga_wavedisp_plot)
                                  C(vga_wavedisp_editpoint)
                                  C(vga_steplfo_repeat)
                                  C(vga_steplfo_shape)
                                  C(vga_steplfo_data)
                                  C(vga_steplfo_data_single)
                                  C(vga_browser_listptr)
                                  C(vga_browser_entry_next)
                                  C(vga_browser_entry_prev)
                                  C(vga_browser_entry_load)
                                  C(vga_browser_category_next)
                                  C(vga_browser_category_prev)
                                  C(vga_browser_category_parent)
                                  C(vga_browser_category_child)
                                  C(vga_browser_preview_start)
                                  C(vga_browser_preview_stop)
                                  C(vga_browser_is_refreshing)
                                  C(vga_inject_database)
                                  C(vga_database_samplelist)
                                  C(vga_save_patch)
                                  C(vga_save_multi)
                                  C(vga_vudata)
                                  C(vga_set_range_and_units)

                              default:
                                  stream << arg;
                                  break;
                              }
                              stream << "]";
                          }},
#undef C

               t);
    // clang-format on
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const actiondata &d)
{
    extern std::string debug_wrapper_ip_to_string(int);
    stream << "actiondata[id=" << debug_wrapper_ip_to_string(d.id) << " subid=" << d.subid
           << " at=" << d.actiontype << "]";
    return stream;
}

struct ad_zonedata
{
    int actiontype;
    int id, subid;

    int zid;

    char part, layer;
    char keylo, keyhi, keyroot, vello, velhi;
    char keylofade, keyhifade, vellofade, velhifade;
    char is_active, is_selected, mute;
    char name[34];
};

// corresponds to vga_wavedisp_editpoint
struct ActionWaveDisplayEditPoint : public actiondata
{
    enum PointType
    {
        start = 0,
        end,
        loopStart,
        loopEnd,
        loopXFade,
        hitPointStart, // hitpoints first item
    };

    // Regular type
    ActionWaveDisplayEditPoint(PointType dragId, int samplePos)
    {
        actiontype = vga_wavedisp_editpoint;
        data.i[0] = dragId;
        data.i[1] = samplePos;
    }
    // hitpoint type
    ActionWaveDisplayEditPoint(int hitPoint, unsigned startSample, unsigned endSample, bool muted,
                               float env)
    {

        data.i[0] = hitPointStart + hitPoint;
        data.i[1] = startSample;
        data.i[2] = endSample;
        data.i[3] = muted;
        data.f[4] = env;
    }

    inline PointType dragId() const { return (PointType)data.i[0]; }
    inline int hitPoint() const
    {
        assert(data.i[0] >= hitPointStart);
        return data.i[0] - hitPointStart;
    }
    bool muted() const { return (bool)data.i[3]; }
    float env() const { return (bool)data.i[4]; }
    inline int samplePos() const { return data.i[1]; }
    inline int endSamplePos() const { return data.i[2]; }
};

// corresponds to vga_wavedisp_sample
struct ActionWaveDisplaySample : public actiondata
{
    ActionWaveDisplaySample(void *samplePtr,
                            int playMode, // todo enum
                            unsigned int start, unsigned int end, unsigned int loopStart,
                            unsigned int loopEnd, unsigned int loopCrossfade,
                            unsigned int numHitpoints)
    {
        id = ip_wavedisplay;
        subid = 0;
        actiontype = vga_wavedisp_sample;
        data.ptr[0] = samplePtr;
        data.i[2] = playMode;
        data.i[3] = start;
        data.i[4] = end;
        data.i[5] = loopStart;
        data.i[6] = loopEnd;
        data.i[7] = loopCrossfade;
        data.i[8] = numHitpoints;
    }
    inline void *samplePtr() { return data.ptr[0]; }
    inline int playMode() { return data.i[2]; }
    inline unsigned int start() { return data.i[3]; }
    inline unsigned int end() { return data.i[4]; }
    inline unsigned int loopStart() { return data.i[5]; }
    inline unsigned int loopEnd() { return data.i[6]; }
    inline unsigned int loopCrossfade() { return data.i[7]; }
    inline unsigned int numHitpoints() { return data.i[8]; }
};
