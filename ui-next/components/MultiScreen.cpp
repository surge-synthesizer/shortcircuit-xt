/*
 * ShortCircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortCircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortCircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortCircuitXT is inspired by, and shares some code with, the
 * commercial product ShortCircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for ShortCircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortCircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "MultiScreen.h"
#include "multi/AdsrPane.h"

namespace scxt::ui
{
MultiScreen::MultiScreen(SCXTEditor *e) : HasEditor(e)
{
    parts = std::make_unique<DebugRect>(juce::Colour(100, 120, 200), "Parts");
    addAndMakeVisible(*parts);
    mainSection = std::make_unique<DebugRect>(juce::Colour(80, 80, 80), "Main");
    addAndMakeVisible(*mainSection);
    browser = std::make_unique<DebugRect>(juce::Colour(200, 120, 100), "Browser");
    addAndMakeVisible(*browser);
    sample = std::make_unique<DebugRect>(juce::Colour(200, 100, 0), "Sample");
    addAndMakeVisible(*sample);

    for (int i = 0; i < 4; ++i)
    {
        fx[i] = std::make_unique<DebugRect>(juce::Colour(i * 60, 0, (4 - i) * 60),
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

    auto mainRect =
        juce::Rectangle<int>(sideWidths + 3 * pad, pad, getWidth() - 2 * sideWidths - 6 * pad,
                             getHeight() - 3 * pad);
    mainSection->setBounds(mainRect);

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

    auto envRect =
        mainRect.withTrimmedTop(wavHeight + fxHeight + modHeight).withHeight(envHeight);
    auto ew = envRect.getWidth() * 0.25;
    eg[0]->setBounds(envRect.withWidth(ew));
    eg[1]->setBounds(envRect.withWidth(ew).translated(ew, 0));
    lfo->setBounds(envRect.withWidth(ew * 2).translated(ew * 2, 0));
}

} // namespace scxt::ui