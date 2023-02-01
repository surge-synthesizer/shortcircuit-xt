#ifndef __SCXT_ENGINE_KEYBOARD_H
#define __SCXT_ENGINE_KEYBOARD_H

#include <utility>
#include <cassert>

namespace scxt::engine
{
/**
 * A class representing a range of the keyboard with fade zones on either
 * end. The range covers keyStart to keyEnd with a note length fade in
 * on either side.
 */
struct KeyboardRange
{
    int keyStart{-1}, keyEnd{-1}; // Start is >= end is <= so a single note has start == end
    int fadeStart{0}, fadeEnd{0};

    KeyboardRange() = default;
    KeyboardRange(int s, int e) : keyStart(s), keyEnd(e) { normalize(); }

    void normalize()
    {
        if (keyEnd < keyStart)
            std::swap(keyStart, keyEnd);
        if (fadeStart + fadeEnd > keyEnd - keyStart)
        {
            // TODO: Handle this case
            assert(false);
        }
    }

    bool includes(int16_t key)
    {
        if (key < 0) return false;
        if (key >= keyStart && key <= keyEnd)
            return true;
        return false;
    }
};
} // namespace scxt::engine
#endif // __SCXT_ENGINE_KEYBOARD_H
