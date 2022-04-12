/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include "HeaderPanel.h"
#include "widgets/OutlinedTextButton.h"
#include "widgets/CompactVUMeter.h"
#include "widgets/PolyphonyDisplay.h"
#include "menus/MainMenuProvider.h"

namespace scxt
{
namespace components
{
HeaderPanel::HeaderPanel(SCXTEditor *ed) : editor(ed), DOMParticipant("header")
{
    for (int i = 0; i < n_sampler_parts; ++i)
    {
        auto b = std::make_unique<scxt::widgets::OutlinedTextButton>(std::to_string(i + 1));
        b->setClickingTogglesState(true);
        b->setRadioGroupId(174, juce::NotificationType::dontSendNotification);
        addAndMakeVisible(*b);
        partsButtons[i] = std::move(b);
        partsButtons[i]->onClick = [this, i]() {
            if (partsButtons[i]->getToggleState())
                editor->selectPart(i);
        };
    }
    partsButtons[editor->selectedPart]->setToggleState(
        true, juce::NotificationType::dontSendNotification);

    zonesButton = std::make_unique<scxt::widgets::OutlinedTextButton>("Zones");
    zonesButton->setClickingTogglesState(true);
    zonesButton->setRadioGroupId(175, juce::NotificationType::dontSendNotification);
    zonesButton->onClick = [this]() { editor->showPage(SCXTEditor::ZONE); };
    zonesButton->setToggleState(true, juce::NotificationType::dontSendNotification);
    addAndMakeVisible(*zonesButton);

    partButton = std::make_unique<scxt::widgets::OutlinedTextButton>("Part");
    partButton->setClickingTogglesState(true);
    partButton->setRadioGroupId(175, juce::NotificationType::dontSendNotification);
    partButton->onClick = [this]() { editor->showPage(SCXTEditor::PART); };
    addAndMakeVisible(*partButton);

    fxButton = std::make_unique<scxt::widgets::OutlinedTextButton>("FX");
    fxButton->setClickingTogglesState(true);
    fxButton->setRadioGroupId(175, juce::NotificationType::dontSendNotification);
    fxButton->onClick = [this]() { editor->showPage(SCXTEditor::FX); };
    addAndMakeVisible(*fxButton);

    aboutButton = std::make_unique<scxt::widgets::OutlinedTextButton>("About");
    aboutButton->setClickingTogglesState(true);
    aboutButton->setRadioGroupId(175, juce::NotificationType::dontSendNotification);
    aboutButton->onClick = [this]() { editor->showPage(SCXTEditor::ABOUT); };
    addAndMakeVisible(*aboutButton);

    menuButton = std::make_unique<scxt::widgets::OutlinedTextButton>("Menu");
    menuButton->setClickingTogglesState(false);
    menuButton->onClick = [this]() {
        auto pm = scxt::menus::MainMenuProvider::createMenu(this->editor);
        pm.showMenuAsync(juce::PopupMenu::Options());
    };
    addAndMakeVisible(*menuButton);

    vuMeter0 = std::make_unique<scxt::widgets::CompactVUMeter>(editor);
    addAndMakeVisible(*vuMeter0);

    polyDisplay = std::make_unique<scxt::widgets::PolyphonyDisplay>(editor);
    addAndMakeVisible(*polyDisplay);

    // This means i probably have a new look and feel so
    auto attachColor = [this](const auto &b) {
        b->remapColour(widgets::OutlinedTextButton::upColour, SCXTColours::headerButton);
        b->remapColour(widgets::OutlinedTextButton::downColour, SCXTColours::headerButtonDown);
        b->remapColour(widgets::OutlinedTextButton::textColour, SCXTColours::headerButtonText);
    };

    for (const auto &b : partsButtons)
    {
        attachColor(b);
    }

    attachColor(zonesButton);
    attachColor(partButton);
    attachColor(fxButton);
    attachColor(aboutButton);
    attachColor(menuButton);
}

HeaderPanel::~HeaderPanel() {}

void HeaderPanel::resized()
{
    auto r = getLocalBounds().reduced(2, 2);
    r = r.withWidth(r.getHeight());

    for (int i = 0; i < n_sampler_parts; ++i)
    {
        partsButtons[i]->setBounds(r);
        r = r.translated(r.getHeight(), 0);
    }

    auto partR = r.getRight();

    auto nRButtons = 5; // zones part fx config menu
    auto rbWidth = 50;
    auto margin = 0;
    auto menuMargin = 20;
    r = getLocalBounds().reduced(2, 2);
    r = r.withLeft(r.getRight() - nRButtons * (rbWidth + margin) - menuMargin);
    r = r.withWidth(rbWidth);

    auto buttonL = r.getX();

    zonesButton->setBounds(r);
    r = r.translated(rbWidth + margin, 0);
    partButton->setBounds(r);
    r = r.translated(rbWidth + margin, 0);
    fxButton->setBounds(r);
    r = r.translated(rbWidth + margin, 0);
    aboutButton->setBounds(r);
    r = r.translated(rbWidth + margin + menuMargin, 0);

    menuButton->setBounds(r);

    r = getLocalBounds().withWidth(128).withCentre({getWidth() / 2, getHeight() / 2}).reduced(0, 2);
    vuMeter0->setBounds(r);

    r = r.translated(r.getWidth() + 2, 0).withWidth(20);
    polyDisplay->setBounds(r);
}

void HeaderPanel::onProxyUpdate()
{
    if (editor->selectedPart >= 0 && editor->selectedPart < n_sampler_parts)
    {
        partsButtons[editor->selectedPart]->setToggleState(
            true, juce::NotificationType::dontSendNotification);
    }
}

void HeaderPanel::paint(juce::Graphics &g)
{
    SCXTLookAndFeel::fillWithGradientHeaderBand(g, getLocalBounds(),
                                                findColour(SCXTColours::headerBackground));
}

} // namespace components
} // namespace scxt