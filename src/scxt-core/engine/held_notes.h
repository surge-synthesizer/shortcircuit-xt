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

#ifndef SCXT_SRC_SCXT_CORE_ENGINE_HELD_NOTES_H
#define SCXT_SRC_SCXT_CORE_ENGINE_HELD_NOTES_H

#include <array>
#include <cstdint>

#include "configuration.h"
#include "utils.h"

namespace scxt::engine
{

/**
 * A group set to create voices on release plays the note at the velocity it was pressed with,
 * so the press has to outlive its note-on event. This is the table which remembers it.
 *
 * Fixed capacity with a linear scan: every note event on the audio thread touches it, so it
 * must not allocate, and the scan is cheap next to findZone which runs on the same events.
 *
 * Matching follows the same wildcard rules the voice manager uses, so it behaves the same for
 * CLAP notes (which carry a note id) and MIDI 1 notes (which do not, and arrive as -1).
 */
struct HeldNotes
{
    static constexpr size_t capacity{256};

    struct Entry
    {
        int16_t channel{-1};
        int16_t key{-1};
        int32_t noteId{-1};
        float velocity{0.f};
        uint64_t order{0}; // press order, so the newest of several matches wins
        bool inUse{false};
    };

    std::array<Entry, capacity> entries{};
    uint64_t nextOrder{1};

    void clear()
    {
        entries.fill(Entry());
        nextOrder = 1;
    }

    void noteOn(int16_t channel, int16_t key, int32_t noteId, float velocity)
    {
        for (auto &e : entries)
        {
            if (e.inUse)
                continue;
            e = {channel, key, noteId, velocity, nextOrder++, true};
            return;
        }
        SCLOG_IF(warnings, "HeldNotes full at " << capacity << " notes; release triggers for "
                                                << "this press will not fire");
    }

    /*
     * Free every entry this note-off lets go of and hand back the velocity of the most recent
     * of them. Returns -1 when the key was never pressed, which is how the caller knows there
     * is no release trigger to fire.
     */
    float releaseNote(int16_t channel, int16_t key, int32_t noteId)
    {
        float res{-1.f};
        uint64_t best{0};
        for (auto &e : entries)
        {
            if (!e.inUse || !matches(e, channel, key, noteId))
                continue;

            if (e.order >= best)
            {
                best = e.order;
                res = e.velocity;
            }
            e = Entry();
        }
        return res;
    }

    // Peek without consuming. Tests and asserts only; the engine always releases.
    float velocityFor(int16_t channel, int16_t key, int32_t noteId) const
    {
        float res{-1.f};
        uint64_t best{0};
        for (const auto &e : entries)
        {
            if (e.inUse && matches(e, channel, key, noteId) && e.order >= best)
            {
                best = e.order;
                res = e.velocity;
            }
        }
        return res;
    }

    size_t heldCount() const
    {
        size_t n{0};
        for (const auto &e : entries)
            n += (e.inUse ? 1 : 0);
        return n;
    }

  private:
    static bool matches(const Entry &e, int16_t channel, int16_t key, int32_t noteId)
    {
        auto res = (channel == -1 || e.channel == -1 || channel == e.channel);
        res = res && (key == -1 || e.key == -1 || key == e.key);
        if (noteId != -1 && e.noteId != -1)
            res = res && (noteId == e.noteId);
        return res;
    }
};

} // namespace scxt::engine
#endif // SCXT_SRC_SCXT_CORE_ENGINE_HELD_NOTES_H
