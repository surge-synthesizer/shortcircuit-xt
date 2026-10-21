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

#include "engine.h"
#include "part.h"
#include "group.h"
#include "zone.h"
#include "messaging/messaging.h"
#include "messaging/audio/audio_serial.h"
#include "messaging/client/client_serial.h"
#include "messaging/client/detail/client_serial_impl.h"
#include "messaging/client/zone_messages.h"
#include "undo_manager/payload_undoable_items.h"

namespace scxt::engine
{
namespace
{
// the groups the mapping pane draws: every group with a selected zone
std::set<int32_t> mappingPaneVisibleGroups(const selection::SelectionManager &sm)
{
    std::set<int32_t> res;
    const auto &st = sm.state[sm.selectedPart];
    for (const auto &z : st.selectedZones)
        res.insert(z.group);
    if (st.leadZone.group >= 0)
        res.insert(st.leadZone.group);
    return res;
}
} // namespace

void Engine::observeMidiZoneNote(int16_t key, int16_t velocity)
{
    using mza = MidiZoneAction;
    auto act = midiZoneAction.load();

    auto post = [this](mza kind, int16_t a, int16_t b) {
        messaging::audio::AudioToSerialization a2s;
        a2s.id = messaging::audio::a2s_midi_zone_note;
        a2s.payloadType = messaging::audio::AudioToSerialization::INT;
        a2s.payload.i[0] = (int32_t)kind;
        a2s.payload.i[1] = a;
        a2s.payload.i[2] = b;
        getMessageController()->sendAudioToSerialization(a2s);
    };

    // the client may switch a learn off underneath us, so step it only if it is still on
    switch (act)
    {
    case mza::SELECT:
        post(act, key, velocity);
        break;
    case mza::SET_KEY_BOUNDS_START:
    case mza::SET_VEL_BOUNDS_START:
    {
        auto keys = act == mza::SET_KEY_BOUNDS_START;
        midiZoneLearnFirst = keys ? key : velocity;
        midiZoneAction.compare_exchange_strong(act, keys ? mza::SET_KEY_BOUNDS_SECOND
                                                         : mza::SET_VEL_BOUNDS_SECOND);
        break;
    }
    case mza::SET_KEY_BOUNDS_SECOND:
    case mza::SET_VEL_BOUNDS_SECOND:
    {
        auto kind = act;
        if (midiZoneAction.compare_exchange_strong(act, mza::NONE))
            post(kind, midiZoneLearnFirst, kind == mza::SET_KEY_BOUNDS_SECOND ? key : velocity);
        break;
    }
    default:
        break;
    }
}

void Engine::setMidiZoneAction(MidiZoneAction a)
{
    assert(messageController->threadingChecker.isSerialThread());
    // a learn always starts from its first note
    if (a == MidiZoneAction::SET_KEY_BOUNDS_SECOND)
        a = MidiZoneAction::SET_KEY_BOUNDS_START;
    if (a == MidiZoneAction::SET_VEL_BOUNDS_SECOND)
        a = MidiZoneAction::SET_VEL_BOUNDS_START;

    if (a == MidiZoneAction::SELECT)
    {
        midiSelectGroups = mappingPaneVisibleGroups(*selectionManager);
        // with nothing showing, any zone in the part is fair game
        auto pidx = selectionManager->selectedPart;
        if (midiSelectGroups.empty() && pidx >= 0 && pidx < numParts)
            for (int g = 0; g < (int)patch->getPart(pidx)->getGroups().size(); ++g)
                midiSelectGroups.insert(g);
    }
    midiZoneAction.store(a);
    sendMappingHeaderStateToClient();
}

void Engine::onMidiZoneNote(MidiZoneAction kind, int16_t a, int16_t b)
{
    assert(messageController->threadingChecker.isSerialThread());
    switch (kind)
    {
    case MidiZoneAction::SELECT:
    {
        auto t = midiSelectTarget(a, b);
        if (!t.has_value())
            break;
        const auto &st = selectionManager->state[selectionManager->selectedPart];
        if (st.leadZone == *t && st.selectedZones.size() == 1)
            break;
        selectionManager->applySelectActions(selection::SelectionManager::SelectActionContents(*t));
        break;
    }
    case MidiZoneAction::SET_KEY_BOUNDS_SECOND:
    case MidiZoneAction::SET_VEL_BOUNDS_SECOND:
        learnLeadZoneRange(kind == MidiZoneAction::SET_KEY_BOUNDS_SECOND, a, b);
        sendMappingHeaderStateToClient();
        break;
    default:
        break;
    }
}

std::optional<selection::SelectionManager::ZoneAddress> Engine::midiSelectTarget(int16_t key,
                                                                                 int16_t velocity)
{
    auto pidx = selectionManager->selectedPart;
    if (pidx < 0 || pidx >= numParts)
        return std::nullopt;

    // what was showing when select went on, and whatever has been brought into view since
    auto groups = midiSelectGroups;
    for (auto g : mappingPaneVisibleGroups(*selectionManager))
        groups.insert(g);

    const auto &part = patch->getPart(pidx);
    // match the way findZone maps a key, so the zone selected is one the note plays
    auto mk = (int16_t)(key + part->configuration.transpose);
    for (auto g : groups)
    {
        if (g < 0 || g >= (int)part->getGroups().size())
            continue;
        const auto &group = part->getGroup(g);
        for (int z = 0; z < (int)group->getZones().size(); ++z)
        {
            auto &m = group->getZone(z)->mapping;
            if (m.keyboardRange.includes(mk) && m.velocityRange.includes(velocity))
                return selection::SelectionManager::ZoneAddress{pidx, g, z};
        }
    }
    return std::nullopt;
}

void Engine::learnLeadZoneRange(bool keys, int16_t a, int16_t b)
{
    assert(messageController->threadingChecker.isSerialThread());
    auto lz = selectionManager->currentLeadZone(*this);
    if (!lz.has_value())
        return;

    const auto &part = patch->getPart(lz->part);
    auto m = part->getGroup(lz->group)->getZone(lz->zone)->mapping;
    auto lo = std::min(a, b);
    auto hi = std::max(a, b);

    auto fit = [](int16_t &fs, int16_t &fe, int span) {
        if (fs + fe > span)
        {
            fs = (int16_t)std::min<int>(fs, span / 2);
            fe = (int16_t)std::min<int>(fe, span - fs);
        }
    };
    if (keys)
    {
        // a transposed part maps on the transposed key, so learn where the played keys land
        auto t = part->configuration.transpose;
        m.keyboardRange.keyStart = (int16_t)std::clamp(lo + t, 0, 127);
        m.keyboardRange.keyEnd = (int16_t)std::clamp(hi + t, 0, 127);
        fit(m.keyboardRange.fadeStart, m.keyboardRange.fadeEnd,
            m.keyboardRange.keyEnd - m.keyboardRange.keyStart + 1);
    }
    else
    {
        m.velocityRange.velStart = (int16_t)std::clamp((int)lo, 1, 127);
        m.velocityRange.velEnd = (int16_t)std::clamp((int)hi, 1, 127);
        fit(m.velocityRange.fadeStart, m.velocityRange.fadeEnd,
            m.velocityRange.velEnd - m.velocityRange.velStart + 1);
    }

    undo::pushPayloadUndoFor<undo::ZoneMappingSpec>(*this, {*lz});
    messageController->scheduleAudioThreadCallback(
        [za = *lz, m](auto &e) {
            if (e.isValidZoneAddress(za))
                e.getPatch()->getPart(za.part)->getGroup(za.group)->getZone(za.zone)->mapping = m;
        },
        [](const auto &e) { undo::refreshLeadDisplay(e, true); });
}

void Engine::setZoneSolo(bool b)
{
    assert(messageController->threadingChecker.isSerialThread());
    zoneSoloEnabled = b;
    refreshZoneSolo();
    sendMappingHeaderStateToClient();
}

void Engine::refreshZoneSolo() const
{
    if (!zoneSoloEnabled && !zoneSoloApplied)
        return;
    zoneSoloApplied = zoneSoloEnabled;

    int16_t part{-1};
    std::vector<selection::SelectionManager::ZoneAddress> zones;
    if (zoneSoloEnabled)
    {
        part = selectionManager->selectedPart;
        const auto &sz = selectionManager->state[part].selectedZones;
        zones.assign(sz.begin(), sz.end());
    }

    // addresses rather than pointers, since a structure change may be queued ahead of us
    messageController->scheduleAudioThreadCallback([part, zones](auto &e) {
        for (const auto &p : *e.getPatch())
        {
            p->zoneSoloActive = false;
            for (const auto &g : *p)
                for (const auto &z : *g)
                    z->soloedBySelection = false;
        }
        // with nothing selected there is nothing to solo, so everything plays
        if (part < 0 || part >= numParts || zones.empty())
            return;
        e.getPatch()->getPart(part)->zoneSoloActive = true;
        for (const auto &a : zones)
            if (e.isValidZoneAddress(a))
                e.getPatch()
                    ->getPart(a.part)
                    ->getGroup(a.group)
                    ->getZone(a.zone)
                    ->soloedBySelection = true;
    });
}

void Engine::sendMappingHeaderStateToClient() const
{
    messaging::client::serializationSendToClient(
        messaging::client::s2c_send_mapping_header_state,
        messaging::client::mappingHeaderState_t{(int32_t)midiZoneAction.load(), zoneSoloEnabled},
        *messageController);
}

} // namespace scxt::engine
