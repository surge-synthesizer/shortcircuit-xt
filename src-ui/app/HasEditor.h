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

#ifndef SCXT_SRC_UI_COMPONENTS_HASEDITOR_H
#define SCXT_SRC_UI_COMPONENTS_HASEDITOR_H

#include <cstdint>
#include <cassert>
#include <type_traits>

namespace scxt::ui::app
{
struct SCXTEditor;

struct HasEditor
{
    SCXTEditor *editor{nullptr};
    HasEditor(SCXTEditor *e);
    virtual ~HasEditor() = default;

    template <typename T> void sendToSerialization(const T &msg);
    template <typename T, typename P, typename V>
    void sendSingleToSerialization(const P &payload, const V &value)
    {
        auto pdiff = (uint8_t *)&value - (uint8_t *)&payload;
        static_assert(std::is_standard_layout_v<P>);
        assert(pdiff >= 0);
        assert(pdiff <= sizeof(payload) - sizeof(value));
        this->sendToSerialization(T({pdiff, value}));
    }

    template <typename T> void updateValueTooltip(const T &attachment);
    template <typename W, typename A>
    void setupWidgetForValueTooltip(const W &widget, const A &attachment);
};
} // namespace scxt::ui::app
#endif // SHORTCIRCUIT_HASEDITOR_H
