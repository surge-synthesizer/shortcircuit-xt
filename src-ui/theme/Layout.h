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
} // namespace constants

template <typename F> inline F centerOfColumnMofN(const juce::Rectangle<F> &r, int m, int N)
{
    assert(N > 0);
    float w = r.getWidth() * 1.f / N;
    return (F)(w * (m + 0.5));
}

template <typename F> inline std::vector<F> columnCenters(const juce::Rectangle<F> &r, int N)
{
    assert(N > 0);
    float w = r.getWidth() * 1.f / N;
    std::vector<F> res;
    for (int i = 0; i < N; ++i)
        res.push_back((F)(w * (i + 0.5)));
    return res;
}

inline float centerOfColumnMofN(juce::Component *r, int m, int N)
{
    return centerOfColumnMofN(r->getLocalBounds().toFloat(), m, N);
}

inline float centerOfColumnMofN(const juce::Component &r, int m, int N)
{
    return centerOfColumnMofN(r.getLocalBounds().toFloat(), m, N);
}

template <typename F>
inline juce::Rectangle<F> columnMofN(const juce::Rectangle<F> &r, int m, int N)
{
    auto w = r.getWidth() * 1.f / N;
    return r.withTrimmedLeft((F)(w * m)).withWidth((F)w);
}

struct Layout
{
    struct Position : juce::Rectangle<float>
    {
        Position(float x, float y, float w, float h) : juce::Rectangle<float>(x, y, w, h) {}
        Position(const juce::Rectangle<float> &other) : juce::Rectangle<float>(other) {}

        Position belowWith(float withHeight) const
        {
            return {getX(), getY() + getHeight(), getWidth(), withHeight};
        }

        Position beneathLabel() const
        {
            return {getX(),
                    getY() + getHeight() + constants::labelHeight + 2 * constants::labelMargin,
                    getWidth(), getHeight()};
        }

        Position toRightOf() const
        {
            return {getX() + getWidth() + constants::labelMargin, getY(), getWidth(), getHeight()};
        }

        Position dX(float tx) const { return translated(tx, 0); }
        Position dY(float ty) const { return translated(0, ty); }

        Position labelBelow() const
        {
            return belowWith(constants::labelHeight).dY(constants::labelMargin);
        }
    };

    Position add(juce::Component &c, float x, float y, float w, float h)
    {
        auto res = Position(x, y, w, h);
        c.setBounds(res.toNearestIntEdges());
        return res;
    }

    Position add(juce::Component &c, const Position &p)
    {
        c.setBounds(p.toNearestIntEdges());
        return p;
    }

    template <typename T>
    Position addLabeled(const sst::jucegui::components::Labeled<T> &lt, const Position &p)
    {
        add(*lt.item, p);
        add(*lt.label, p.labelBelow());
        return p;
    }

    template <int size, typename T>
    Position knob(sst::jucegui::components::Labeled<T> &lt, float x, float y)
    {
        auto kc = Position(x, y, size, size);
        return addLabeled(lt, kc);
    }

    template <int size, typename T>
    Position knobCX(sst::jucegui::components::Labeled<T> &lt, float x, float y)
    {
        auto kc = Position(x - size / 2, y, size, size);
        return addLabeled(lt, kc);
    }
};

} // namespace scxt::ui::theme::layout
#endif // SHORTCIRCUITXT_LAYOUT_H
