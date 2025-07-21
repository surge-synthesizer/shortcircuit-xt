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

#ifndef SCXT_SRC_UI_APP_BROWSER_UI_BROWSERPANEINTERFACES_H
#define SCXT_SRC_UI_APP_BROWSER_UI_BROWSERPANEINTERFACES_H

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
    virtual std::set<WithSampleInfo *> getMultipleSampleInfos() const = 0;
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
