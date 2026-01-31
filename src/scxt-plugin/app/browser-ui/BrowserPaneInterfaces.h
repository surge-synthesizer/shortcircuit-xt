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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_BROWSER_UI_BROWSERPANEINTERFACES_H
#define SCXT_SRC_SCXT_PLUGIN_APP_BROWSER_UI_BROWSERPANEINTERFACES_H

#include <optional>
#include <set>
#include "filesystem/import.h"
#include "sample/compound_file.h"

namespace scxt::ui::app::browser_ui
{
struct WithSampleInfo
{
    virtual ~WithSampleInfo() {}
    virtual std::optional<fs::directory_entry> getDirEnt() const = 0;
    virtual std::optional<sample::compound::CompoundElement> getCompoundElement() const = 0;
    virtual bool encompassesMultipleSampleInfos() const = 0;
    virtual std::vector<WithSampleInfo *> getMultipleSampleInfos() const = 0;
};

template <typename T> static bool hasSampleInfo(const T &t)
{
    return dynamic_cast<const WithSampleInfo *>(t.get()) != nullptr;
}

template <typename T> static WithSampleInfo *asSampleInfo(const T &t)
{
    return dynamic_cast<WithSampleInfo *>(t.get());
}

} // namespace scxt::ui::app::browser_ui
#endif // BROWSERPANEINTERFACES_H
