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

#ifndef SCXT_SRC_UI_APP_SHARED_UIHELPERS_H
#define SCXT_SRC_UI_APP_SHARED_UIHELPERS_H

#include <juce_core/juce_core.h>
#include "filesystem/import.h"

namespace scxt::ui::app::shared
{

inline fs::path juceFileToFsPath(const juce::File &f)
{
    auto js = f.getFullPathName();
#if JUCE_WINDOWS
    return fs::path(js.toUTF16().getAddress());
#else
    return fs::path(fs::u8path(js.toUTF8().getAddress()));
#endif
}

} // namespace scxt::ui::app::shared

#endif // SHORTCIRCUITXT_UIHELPERS_H
