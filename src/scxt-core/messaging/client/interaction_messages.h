/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_INTERACTION_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_INTERACTION_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/selection_traits.h"
#include "selection/selection_manager.h"
#include "engine/engine.h"
#include "client_macros.h"
#include "patch_io/patch_io.h"
#include "utils.h"

namespace scxt::messaging::client
{
// Severity for reportItem-style messages. Order matters — Error must be 0
// for back-compat with the original error-only tuple.
enum ReportItemSeverity : int
{
    Severity_Error = 0,
    Severity_Warning = 1,
    Severity_Info = 2,
};

// severity, title, message, source file, source line
typedef std::tuple<int, std::string, std::string, std::string, int> s2cError_t;
SERIAL_TO_CLIENT(ReportError, s2c_report_error, s2cError_t, onErrorFromEngine);

// worker threads can't send to the client, so they bounce reports through here
inline void doReportItemFromWorker(const s2cError_t &payload, MessageController &cont)
{
    const auto &[severity, title, body, source, line] = payload;
    cont.reportItemToClient(severity, title, body, source, line);
}
CLIENT_TO_SERIAL(ReportItemFromWorker, c2s_report_item_from_worker, s2cError_t,
                 doReportItemFromWorker(payload, cont));

// Behind-the-scenes batch of (format, key, value) triples emitted by an
// importer at finish() to surface tokens it recognized but didn't route.
// Not user-facing; used by diagnostic tools (e.g. check-multi-loadability)
// to aggregate coverage gaps.
typedef std::vector<std::tuple<std::string, std::string, std::string>> s2cUnusedItems_t;
SERIAL_TO_CLIENT(ReportUnusedItems, s2c_report_unused_items, s2cUnusedItems_t,
                 onUnusedItemsFromEngine);

// Fired by the engine after a compound-file import (SFZ/EXS/AKP/multisample/
// SF2/GIG) finishes — successfully or not. Payload is (path, success). The
// scanner tool waits on this so async error/warn messages get attributed to
// the correct file.
typedef std::tuple<std::string, bool> s2cImportComplete_t;
SERIAL_TO_CLIENT(ReportImportComplete, s2c_compound_import_complete, s2cImportComplete_t,
                 onImportCompleteFromEngine);

inline void raiseDebugError(MessageController &c, int count)
{
    for (int i = 0; i < count; ++i)
    {
        RAISE_ERROR_CONT(c, "A Dummy Error " + std::to_string(i),
                         "This is a dummy error " + std::to_string(i) +
                             ". I chose to have it have "
                             "a very long message like this one so I can test multiline "
                             "string rendering in the error box. So this one has details "
                             "like "
                             "this and that");
    }
}
CLIENT_TO_SERIAL(RaiseDebugError, c2s_raise_debug_error, int, raiseDebugError(cont, payload))

// note, 0...1 velocity, onoff
typedef std::tuple<int32_t, float, bool> noteOnOff_t;
inline void processMidiFromGUI(const noteOnOff_t &g, const engine::Engine &engine,
                               MessageController &cont)
{
    auto [n, v, onoff] = g;

    auto sel = engine.getSelectionManager()->selectedPart;
    if (sel < 0)
        return;

    auto p = sel;
    auto ch = engine.getPatch()->getPart(p)->configuration.channel;
    if (ch < 0)
        ch = 0;

    if (onoff)
    {
        // via the engine, not straight to the voice manager, so the onscreen keyboard
        // reaches release triggers the same way a MIDI or CLAP note does
        cont.scheduleAudioThreadCallback([ch, vel = v, note = n](auto &eng) {
            eng.processNoteOnEvent(0, ch, note, -1, vel, 0.f);
        });
    }
    else
    {
        cont.scheduleAudioThreadCallback(
            [ch, vel = v, note = n](auto &eng) { eng.processNoteOffEvent(0, ch, note, -1, vel); });
    }
}
CLIENT_TO_SERIAL(NoteFromGUI, c2s_noteonoff, noteOnOff_t, processMidiFromGUI(payload, engine, cont))

// arm or cancel a one-shot learn; the next note-on comes back as SendLearnedNote instead of playing
inline void doArmNoteLearn(bool arm, MessageController &cont)
{
    cont.scheduleAudioThreadCallback([arm](auto &eng) { eng.noteLearnArmed = arm; });
}
CLIENT_TO_SERIAL(ArmNoteLearn, c2s_arm_note_learn, bool, doArmNoteLearn(payload, cont));
SERIAL_TO_CLIENT(SendLearnedNote, s2c_send_learned_note, int16_t, onLearnedNote);

inline void doHostCallback(uint64_t pl, MessageController &cont)
{
    if (cont.requestHostCallback)
    {
        cont.requestHostCallback(pl);
    }
}
CLIENT_TO_SERIAL(RequestHostCallback, c2s_request_host_callback, uint64_t,
                 doHostCallback(payload, cont));

inline void doResetEngine(const std::string &fl, engine::Engine &e, MessageController &cont)
{
    e.undoManager.clear();
    scxt::patch_io::initFromResourceBundle(e, fl);
    e.markDirty();
    e.sendFullRefreshToClient();
}
CLIENT_TO_SERIAL(ResetEngine, c2s_reset_engine, std::string, doResetEngine(payload, engine, cont));

// payload marks the host session dirty, which a new instance should not
inline void doResetEngineToStartupPatch(bool markDirty, engine::Engine &e, MessageController &cont)
{
    if (!scxt::patch_io::initFromStartupPatch(e))
        return;
    e.undoManager.clear();
    if (markDirty)
        e.markDirty();
    e.sendFullRefreshToClient();
}
CLIENT_TO_SERIAL(ResetEngineToStartupPatch, c2s_reset_engine_to_startup_patch, bool,
                 doResetEngineToStartupPatch(payload, engine, cont));

inline void doResendFullState(const bool &b, engine::Engine &e, MessageController &cont)
{
    if (b)
    {
        e.sendFullRefreshToClient();
    }
    else
    {
        SCLOG_IF(debug, "Why did you bother sending the resend false message?");
    }
}
CLIENT_TO_SERIAL(ResendFullState, c2s_resend_full_state, bool,
                 doResendFullState(payload, engine, cont));

/*
 * The UI owns the colormap visual; the serialization side stores an opaque
 * JSON blob on DawExtraState so the DAW session can round-trip an unsaved
 * theme edit. Payload is the ColorMap JSON string.
 */
SERIAL_TO_CLIENT(SetColormap, s2c_set_colormap, std::string, onColormap);

inline void doStoreColormap(const std::string &payload, engine::Engine &engine,
                            MessageController &cont)
{
    engine.dawExtraState.editedColormap = payload;
}
CLIENT_TO_SERIAL(StoreColormap, c2s_store_colormap, std::string,
                 doStoreColormap(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_INTERACTION_MESSAGES_H
