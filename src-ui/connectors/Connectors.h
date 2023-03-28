/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#ifndef SCXT_SRC_UI_CONNECTORS_CONNECTORS_H
#define SCXT_SRC_UI_CONNECTORS_CONNECTORS_H

#include <utility>
#include <memory>
#include <string>
namespace scxt::ui::connectors
{
template <typename attachment_t, typename widget_t> struct ConnectorFactory
{
    typedef std::function<void(const attachment_t &a)> onGuiChange_t;
    onGuiChange_t onGuiChange{nullptr};
    typename attachment_t::parent_t *parent{nullptr};
    typename attachment_t::payload_t &payload;
    ConnectorFactory(onGuiChange_t fn, typename attachment_t::parent_t *that,
                     typename attachment_t::payload_t &pl)
        : onGuiChange(fn), parent(that), payload(pl)
    {
    }

    template <typename D, typename M>
    std::pair<std::unique_ptr<attachment_t>, std::unique_ptr<widget_t>> attach(const std::string &l,
                                                                               D &d, M m)
    {
        auto at = std::make_unique<attachment_t>(
            parent, d, l, onGuiChange,
            [m](const typename attachment_t::payload_t &pl) { return pl.*m; }, payload.*m);
        auto w = std::make_unique<widget_t>();
        w->setSource(at.get());
        return {std::move(at), std::move(w)};
    }
};
} // namespace scxt::ui::connectors
#endif // SHORTCIRCUIT_CONNECTORS_H
