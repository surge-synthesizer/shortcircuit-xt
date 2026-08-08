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

#include "scl_kbm.h"

#include <cmath>
#include "Tunings.h"

namespace scxt::tuning
{

bool buildRetuneTable(const std::string &sclText, const std::string &kbmText,
                      MidikeyRetuner::RetuneTable &out, std::string &errorOut)
{
    if (sclText.empty())
    {
        errorOut = "No scale (SCL) provided.";
        return false;
    }

    try
    {
        auto scale = Tunings::parseSCLData(sclText);

        Tunings::KeyboardMapping kbm;
        auto haveKbm = !kbmText.empty();
        if (haveKbm)
            kbm = Tunings::parseKBMData(kbmText);

        /*
         * A KBM which skips keys leaves nonsense in the unmapped slots. Since we
         * interpolate between adjacent keys for continuous retuning, take the
         * interpolated variant so a skipped key never poisons its neighbours.
         */
        auto tuning = (haveKbm ? Tunings::Tuning(scale, kbm) : Tunings::Tuning(scale))
                          .withSkippedNotesInterpolated();

        MidikeyRetuner::RetuneTable res;
        for (int i = 0; i < MidikeyRetuner::RetuneTable::tableSize; ++i)
        {
            auto mn = i - MidikeyRetuner::RetuneTable::midiOffset;
            auto v = tuning.retuningFromEqualInSemitonesForMidiNote(mn);
            if (!std::isfinite(v))
            {
                errorOut = "Scale produced a non-finite pitch at midi note " + std::to_string(mn);
                return false;
            }
            res.semitones[i] = (float)v;
        }

        // A KBM count of zero means a linear mapping, so the keyboard repeats on the scale
        auto ri = (haveKbm && kbm.count > 0) ? kbm.count : scale.count;
        res.repetitionInterval = ri > 0 ? ri : 12;

        out = res;
        return true;
    }
    catch (const Tunings::TuningError &e)
    {
        errorOut = e.what();
        return false;
    }
    catch (const std::exception &e)
    {
        errorOut = e.what();
        return false;
    }
}

std::string twelveTETSclText()
{
    try
    {
        return Tunings::evenTemperament12NoteScale().rawText;
    }
    catch (const std::exception &)
    {
        return {};
    }
}

/*
 * Spelled out rather than taken from Tunings::KeyboardMapping().rawText, which
 * writes the reference frequency at six significant figures (261.626) and so is
 * a few thousandths of a cent shy of a true no-op. The comments make it a usable
 * starting point for hand editing too.
 */
std::string defaultKbmText()
{
    return R"KBM(! Default keyboard mapping
!
! Size of map. 0 is a linear map of the scale onto the keyboard
0
! First and last MIDI note to map
0
127
! Middle note, where the scale begins
60
! Reference note and its frequency
60
261.6255653
! Scale degree for the formal octave. 0 means the size of the scale
0
)KBM";
}

} // namespace scxt::tuning
