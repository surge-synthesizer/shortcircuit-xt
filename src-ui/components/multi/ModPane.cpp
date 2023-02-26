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

#include "ModPane.h"
#include "connectors/SCXTStyleSheetCreator.h"
#include "sst/jucegui/components/HSlider.h"

namespace scxt::ui::multi
{
struct ModRow : juce::Component, HasEditor
{
    int index{0};
    ModPane *parent{nullptr};
    std::unique_ptr<juce::Label> rowL;
    std::unique_ptr<juce::ToggleButton> power;
    std::unique_ptr<juce::TextButton> source, sourceVia, curve, target;
    std::unique_ptr<juce::TextButton> slider;
    // std::unique_ptr<sst::jucegui::components::HSlider> slider;
    ModRow(SCXTEditor *e, int i, ModPane *p) : HasEditor(e), index(i), parent(p)
    {
        rowL = std::make_unique<juce::Label>("Row", std::to_string(index + 1));
        addAndMakeVisible(*rowL);

        power = std::make_unique<juce::ToggleButton>();
        addAndMakeVisible(*power);

        source = std::make_unique<juce::TextButton>("Source", "Source");
        addAndMakeVisible(*source);

        sourceVia = std::make_unique<juce::TextButton>("Via", "Via");
        addAndMakeVisible(*sourceVia);

        slider = std::make_unique<juce::TextButton>("Slider", "Slider");
        addAndMakeVisible(*slider);

        curve = std::make_unique<juce::TextButton>("Crv", "Crv");
        addAndMakeVisible(*curve);

        target = std::make_unique<juce::TextButton>("Target", "Target");
        addAndMakeVisible(*target);
    }

    void resized()
    {
        auto b = getLocalBounds().reduced(1, 1);

        rowL->setBounds(b.withWidth(b.getHeight() + 10));
        b = b.translated(b.getHeight() + 12, 0);

        power->setBounds(b.withWidth(b.getHeight()));
        b = b.translated(b.getHeight() + 2, 0);
        source->setBounds(b.withWidth(90));
        b = b.translated(92, 0);
        sourceVia->setBounds(b.withWidth(90));
        b = b.translated(92, 0);
        slider->setBounds(b.withWidth(170));
        b = b.translated(172, 0);
        curve->setBounds(b.withWidth(60));
        b = b.translated(62, 0);
        target->setBounds(b.withWidth(90));
    }
    void refreshRow() { repaint(); }

    void paint(juce::Graphics &g)
    {
        g.setColour(juce::Colour(255, (index / 6) * 128, (index % 6) * 255 / 6).withAlpha(0.6f));
        g.fillRect(getLocalBounds());
    }
};

ModPane::ModPane(SCXTEditor *e) : sst::jucegui::components::NamedPanel(""), HasEditor(e)
{
    setCustomClass(connectors::SCXTStyleSheetCreator::ModulationTabs);

    hasHamburger = true;
    isTabbed = true;
    tabNames = {"MOD 1-6", "MOD 7-12"};

    resetTabState();

    onTabSelected = [wt = juce::Component::SafePointer(this)](int i) {
        if (!wt)
            return;

        wt->tabRange = i;
        wt->rebuildMatrix();
    };

    setActive(false);
}

ModPane::~ModPane() = default;

void ModPane::resized()
{
    auto en = isEnabled();
    if (!en)
        return;
    auto r = getContentArea().toFloat();
    auto rh = r.getHeight() * 1.f / numRowsOnScreen;
    r = r.withHeight(rh);
    for (int i = 0; i < numRowsOnScreen; ++i)
    {
        if (rows[i])
            rows[i]->setBounds(r.toNearestIntEdges());
        r = r.translated(0, rh);
    }
}

void ModPane::setActive(bool b)
{
    setEnabled(b);
    rebuildMatrix();
}

void ModPane::rebuildMatrix()
{
    auto en = isEnabled();
    removeAllChildren();
    for (auto &a : rows)
        a.reset(nullptr);
    if (!en)
        return;
    for (int i = 0; i < numRowsOnScreen; ++i)
    {
        rows[i] = std::make_unique<ModRow>(editor, i + tabRange * numRowsOnScreen, this);
        addAndMakeVisible(*(rows[i]));
    }
    resized();
}

void ModPane::refreshMatrix()
{
    assert(isEnabled());
    for (auto &r : rows)
    {
        if (r)
            r->refreshRow();
    }
}
} // namespace scxt::ui::multi
