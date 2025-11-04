/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_SAMPLE_EXS_SUPPORT_EXS_IMPORT_H
#define SCXT_SRC_SAMPLE_EXS_SUPPORT_EXS_IMPORT_H

#include "filesystem/import.h"
#include <engine/engine.h>

namespace scxt::exs_support
{
/*
 * The EXS import ability is based almost entirely on the java
 * implementation from mossgraber's ConvertWithMoss available
 *
 * https://github.com/git-moss/ConvertWithMoss/
 *
 * which is released under Gnu GPL3. None of the code is copied
 * here but without the reference to that code, this implementation
 * would have been impossible.
 *
 * The implementation here is very incomplete, and may be removed
 * before our 1.0 release if it turns out to not be tenable.
 */
bool importEXS(const fs::path &, engine::Engine &);

void dumpEXSToLog(const fs::path &);

} // namespace scxt::exs_support

#endif // SHORTCIRCUITXT_EXS_IMPORT_H
