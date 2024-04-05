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

#ifndef SCXT_SRC_UI_THEME_LAYOUT_H
#define SCXT_SRC_UI_THEME_LAYOUT_H

#include <vector>
#include <cassert>
#include <juce_gui_basics/juce_gui_basics.h>

#include "sst/jucegui/components/LabeledItem.h"

namespace scxt::ui::theme::layout
{
namespace constants
{
static constexpr int labelHeight{12};
static constexpr int labelMargin{2};
static constexpr int extraLargeKnob{80};
static constexpr int largeKnob{60};
static constexpr int mediumKnob{40};
static constexpr int mediumSmallKnob{32};
} // namespace constants

using coord_t = int;
using posn_t = juce::Rectangle<coord_t>;

inline posn_t belowWithHeight(const posn_t &p, int withHeight)
{
    return {p.getX(), p.getY() + p.getHeight(), p.getWidth(), withHeight};
}

inline posn_t belowLabel(const posn_t &p)
{
    return {p.getX(),
            p.getY() + p.getHeight() + constants::labelHeight + 2 * constants::labelMargin,
            p.getWidth(), p.getHeight()};
}

inline posn_t toRightOf(const posn_t &p)
{
    return {p.getX() + p.getWidth() + constants::labelMargin, p.getY(), p.getWidth(),
            p.getHeight()};
}

inline posn_t labelBelow(const posn_t &p)
{
    return belowWithHeight(p, constants::labelHeight).translated(0, constants::labelMargin);
}

inline std::vector<posn_t> columns(const posn_t &p, size_t N)
{
    assert(N > 0);

    float w = p.getWidth() * 1.f / N;
    std::vector<posn_t> res;
    for (int i = 0; i < N; ++i)
    {
        res.emplace_back(p.getX() + i * w, p.getY(), w, p.getHeight());
    }
    return res;
}

inline std::vector<juce::Point<coord_t>> columnCenters(const posn_t &r, int N)
{
    assert(N > 0);
    auto cols = columns(r, N);
    auto res = std::vector<juce::Point<coord_t>>();
    std::transform(cols.begin(), cols.end(), std::back_inserter(res),
                   [](const auto &a) { return a.getCentre(); });
    return res;
}

inline std::vector<coord_t> columnCenterX(const posn_t &r, int N)
{
    assert(N > 0);
    auto cols = columns(r, N);
    auto res = std::vector<coord_t>();
    std::transform(cols.begin(), cols.end(), std::back_inserter(res),
                   [](const auto &a) { return a.getCentreX(); });
    return res;
}

template <typename T>
inline posn_t labeledAt(const sst::jucegui::components::Labeled<T> &lt, const posn_t &p)
{
    lt.item->setBounds(p);
    lt.label->setBounds(labelBelow(p));
    return p;
}

template <int size, typename T>
inline posn_t knob(sst::jucegui::components::Labeled<T> &lt, float x, float y)
{
    auto kc = posn_t(x, y, size, size);
    return labeledAt(lt, kc);
}

template <int size, typename T>
inline posn_t knobCX(sst::jucegui::components::Labeled<T> &lt, float x, float y)
{
    return knob<size, T>(lt, x - size / 2, y);
}

} // namespace scxt::ui::theme::layout
#endif // SHORTCIRCUITXT_LAYOUT_H
