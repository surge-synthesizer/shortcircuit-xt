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

#include "MultiScreen.h"
#include "multi/AdsrPane.h"
#include "multi/PartGroupSidebar.h"

namespace scxt::ui
{

struct DebugRect : public sst::jucegui::components::NamedPanel
{
    struct CL : juce::Component
    {
        juce::Colour color;
        std::string label;
        void paint(juce::Graphics &g) override
        {
            g.setFont(juce::Font("Comic Sans MS", 40, juce::Font::plain));

            g.setColour(color);
            g.drawText(label, getLocalBounds(), juce::Justification::centred);
        }
    };
    std::unique_ptr<CL> cl;
    DebugRect(const juce::Colour &c, const std::string &s) : sst::jucegui::components::NamedPanel(s)
    {
        cl = std::make_unique<CL>();
        cl->color = c;
        cl->label = s;
        addAndMakeVisible(*cl);
    }
    void resized() override { cl->setBounds(getContentArea()); }
};

MultiScreen::MultiScreen(SCXTEditor *e) : HasEditor(e)
{
    parts = std::make_unique<multi::PartGroupSidebar>(editor);
    addAndMakeVisible(*parts);
    browser = std::make_unique<DebugRect>(juce::Colour(200, 120, 100), "Browser");
    addAndMakeVisible(*browser);
    sample = std::make_unique<DebugRect>(juce::Colour(200, 100, 0), "Sample");
    addAndMakeVisible(*sample);

    for (int i = 0; i < 4; ++i)
    {
        fx[i] = std::make_unique<DebugRect>(juce::Colour(i * 60, 255, (4 - i) * 60),
                                            "FX" + std::to_string(i));
        addAndMakeVisible(*(fx[i]));
    }
    mod = std::make_unique<DebugRect>(juce::Colour(100, 200, 150), "Mod");
    addAndMakeVisible(*mod);
    mix = std::make_unique<DebugRect>(juce::Colour(150, 220, 100), "Mix");
    addAndMakeVisible(*mix);

    for (int i = 0; i < 2; ++i)
    {
        eg[i] = std::make_unique<multi::AdsrPane>(editor, i);
        addAndMakeVisible(*(eg[i]));
    }
    lfo = std::make_unique<DebugRect>(juce::Colour(250, 80, 120), "LFO");
    addAndMakeVisible(*lfo);
}

MultiScreen::~MultiScreen() = default;

void MultiScreen::layout()

{
    parts->setBounds(pad, pad, sideWidths, getHeight() - 3 * pad);
    browser->setBounds(getWidth() - sideWidths - pad, pad, sideWidths, getHeight() - 3 * pad);

    auto mainRect = juce::Rectangle<int>(
        sideWidths + 3 * pad, pad, getWidth() - 2 * sideWidths - 6 * pad, getHeight() - 3 * pad);

    auto wavHeight = mainRect.getHeight() - envHeight - modHeight - fxHeight;
    sample->setBounds(mainRect.withHeight(wavHeight));

    auto fxRect = mainRect.withTrimmedTop(wavHeight).withHeight(fxHeight);
    auto fw = fxRect.getWidth() * 0.25;
    auto tfr = fxRect.withWidth(fw);
    for (int i = 0; i < 4; ++i)
    {
        fx[i]->setBounds(tfr);
        tfr.translate(fw, 0);
    }

    auto modRect = mainRect.withTrimmedTop(wavHeight + fxHeight).withHeight(modHeight);
    auto mw = modRect.getWidth() * 0.750;
    mod->setBounds(modRect.withWidth(mw));
    auto xw = modRect.getWidth() * 0.250;
    mix->setBounds(modRect.withWidth(xw).translated(mw, 0));

    auto envRect = mainRect.withTrimmedTop(wavHeight + fxHeight + modHeight).withHeight(envHeight);
    auto ew = envRect.getWidth() * 0.25;
    eg[0]->setBounds(envRect.withWidth(ew));
    eg[1]->setBounds(envRect.withWidth(ew).translated(ew, 0));
    lfo->setBounds(envRect.withWidth(ew * 2).translated(ew * 2, 0));
}

} // namespace scxt::ui