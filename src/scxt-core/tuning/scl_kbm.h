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

#ifndef SCXT_SRC_SCXT_CORE_TUNING_SCL_KBM_H
#define SCXT_SRC_SCXT_CORE_TUNING_SCL_KBM_H

#include <string>
#include "midikey_retuner.h"

namespace scxt::tuning
{
/*
 * Flatten a Scala SCL scale, and an optional KBM keyboard mapping, into the POD
 * table the retuner reads on the audio thread.
 *
 * Serialization thread only: this allocates and the tuning library reports errors
 * by exception, all of which are caught here so none escape into engine code.
 *
 * On failure returns false, sets errorOut, and leaves out untouched.
 */
bool buildRetuneTable(const std::string &sclText, const std::string &kbmText,
                      MidikeyRetuner::RetuneTable &out, std::string &errorOut);

/*
 * The SCL text for plain 12-TET, used to seed the editor so it always shows a
 * well formed example rather than an empty box.
 */
std::string twelveTETSclText();

/*
 * The default KBM - linear map, scale starting on 60, note 60 at 261.6255653Hz -
 * used to seed the editor with something well formed to edit.
 *
 * This is the mapping that means "whatever the scale already does", so applying an
 * untouched seed is a no-op against any scale. A 69-at-440 mapping would not be:
 * it only coincides with this in 12-TET, and shifts a non-12 scale by however far
 * 69 sits from 60 in that scale.
 */
std::string defaultKbmText();

} // namespace scxt::tuning

#endif // SCXT_SRC_SCXT_CORE_TUNING_SCL_KBM_H
