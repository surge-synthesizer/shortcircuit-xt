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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_SHARED_UIHELPERS_H
#define SCXT_SRC_SCXT_PLUGIN_APP_SHARED_UIHELPERS_H

#include <juce_core/juce_core.h>
#include "filesystem/import.h"

namespace scxt::ui::app::shared
{

inline fs::path juceStringToFSPath(const juce::String &fullPathName)
{
#if JUCE_WINDOWS
    fs::path fullPath = fullPathName.toWideCharPointer();
#else

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    fs::path fullPath = fs::u8path(fullPathName.toRawUTF8());
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif
    return fullPath;
}

inline fs::path juceFileToFSPath(const juce::File &f)
{
    auto fullPathName = f.getFullPathName();
    return juceStringToFSPath(fullPathName);
}

} // namespace scxt::ui::app::shared

#endif // SHORTCIRCUITXT_UIHELPERS_H
