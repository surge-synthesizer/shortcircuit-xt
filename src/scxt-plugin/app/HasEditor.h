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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_HASEDITOR_H
#define SCXT_SRC_SCXT_PLUGIN_APP_HASEDITOR_H

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
    HasEditor(HasEditor *e);
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
    void setupWidgetForValueTooltip(W *widget, const A &attachment);

    template <typename W, typename A>
    void setupIntAttachedWidgetForValueMenu(W *widget, const A &attachment);

    template <typename P, typename A> void addSubscription(const P &, A &);
};
} // namespace scxt::ui::app
#endif // SHORTCIRCUIT_HASEDITOR_H
