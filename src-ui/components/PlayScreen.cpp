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

#include "PlayScreen.h"
#include "utils.h"
#include "components/SCXTEditor.h"
#include "infrastructure/user_defaults.h"
#include "browser/BrowserPane.h"

namespace scxt::ui
{
namespace jcmp = sst::jucegui::components;

struct ViewportComponent : juce::Component, HasEditor
{
    PlayScreen *playScreen{nullptr};
    ViewportComponent(PlayScreen *p, SCXTEditor *e) : playScreen(p), HasEditor(e) {}

    void paint(juce::Graphics &g)
    {
        for (int i = 0; i < scxt::numParts; ++i)
        {
            auto col = editor->themeColor(theme::ColorMap::grid_secondary);
            if (i == editor->selectedPart)
            {
                col = editor->themeColor(theme::ColorMap::accent_1a);
            }
            g.setColour(col);
            auto bx =
                playScreen->rectangleForPart(i).withTrimmedBottom(PlayScreen::interPartMargin);
            g.drawRect(bx, 1);
            g.drawVerticalLine(bx.getX() + multi::PartSidebarCard::width, bx.getY(),
                               bx.getY() + bx.getHeight());
        }
    }
};

PlayScreen::PlayScreen(scxt::ui::SCXTEditor *e) : HasEditor(e)
{
    browser = std::make_unique<browser::BrowserPane>(editor);
    addAndMakeVisible(*browser);

    playNamedPanel = std::make_unique<jcmp::NamedPanel>("PARTS");
    playNamedPanel->hasHamburger = true;
    playNamedPanel->onHamburger = [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showMenu();
    };
    addAndMakeVisible(*playNamedPanel);

    int pt{0};
    viewport = std::make_unique<jcmp::Viewport>("Parts");
    viewportContents = std::make_unique<ViewportComponent>(this, editor);
    for (int i = 0; i < scxt::numParts; ++i)
    {
        partSidebars[i] = std::make_unique<multi::PartSidebarCard>(i, editor);
        partSidebars[i]->selfAccent = false;
        viewportContents->addAndMakeVisible(*partSidebars[i]);

        skinnyPage[i] = 0;
        auto ms = std::make_unique<jcmp::MultiSwitch>();
        viewportContents->addAndMakeVisible(*ms);
        auto ds = std::make_unique<skinnyPage_t>(ms, skinnyPage[i]);
        static_assert(scxt::macrosPerPart == 16); // assuming this here for now
        ds->valueToString = [](auto b) {
            if (b)
                return "9-16";
            else
                return "1-8";
        };
        ds->onValueChanged = [w = juce::Component::SafePointer(this)](auto v) {
            if (w)
                w->rebuildPositionsAndVisibilites();
        };
        skinnyPageSwitches[i] = std::move(ds);
    }

    for (auto &ped : macroEditors)
    {
        int id{0};
        for (auto &sed : ped)
        {
            sed = std::make_unique<multi::SingleMacroEditor>(editor, pt, id, true);
            sed->changePart(pt);
            id++;
            viewportContents->addAndMakeVisible(*sed);
        }
        pt++;
    }
    viewport->setViewedComponent(viewportContents.get(), false);
    playNamedPanel->addAndMakeVisible(*viewport);

    tallMode = (bool)editor->defaultsProvider.getUserDefaultValue(
        infrastructure::DefaultKeys::playModeExpanded, true);
    rebuildPositionsAndVisibilites();
}

PlayScreen::~PlayScreen() {}

void PlayScreen::rebuildPositionsAndVisibilites()
{
    if (tallMode)
    {
        for (int p = 0; p < scxt::numParts; ++p)
        {
            skinnyPageSwitches[p]->widget->setVisible(false);
            for (int i = 0; i < scxt::macrosPerPart; ++i)
            {
                macroEditors[p][i]->setVisible(true);
            }
        }
    }
    else
    {
        for (int p = 0; p < scxt::numParts; ++p)
        {
            skinnyPageSwitches[p]->widget->setVisible(true);
            static_assert(scxt::macrosPerPart == 16); // assuming this here too
            for (int i = 0; i < scxt::macrosPerPart; ++i)
            {
                auto firstHalf = (i < scxt::macrosPerPart / 2);
                macroEditors[p][i]->setVisible(firstHalf == !skinnyPage[p]);
            }
        }
    }
    if (isVisible())
        resized();
}
void PlayScreen::showMenu()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Play Screen");
    p.addSeparator();
    p.addItem("Expanded Display", true, tallMode, [w = juce::Component::SafePointer(this)]() {
        if (w)
        {
            w->editor->defaultsProvider.updateUserDefaultValue(
                infrastructure::DefaultKeys::playModeExpanded, true);
            w->tallMode = true;
            w->rebuildPositionsAndVisibilites();
        }
    });
    p.addItem("Compressed Display", true, !tallMode, [w = juce::Component::SafePointer(this)]() {
        if (w)
        {
            w->editor->defaultsProvider.updateUserDefaultValue(
                infrastructure::DefaultKeys::playModeExpanded, false);
            w->tallMode = false;
            w->rebuildPositionsAndVisibilites();
        }
    });
    p.showMenuAsync(editor->defaultPopupMenuOptions());
}
void PlayScreen::visibilityChanged()
{
    if (isVisible())
    {
        for (auto &ped : macroEditors)
        {
            for (auto &sed : ped)
            {
                sed->updateFromEditorData();
            }
        }

        repaint();
    }
}

void PlayScreen::resized()
{
    int pad = 0;
    int busHeight = 425;
    browser->setBounds(getWidth() - browserPanelWidth - pad, pad, browserPanelWidth,
                       getHeight() - 3 * pad);
    playNamedPanel->setBounds(getLocalBounds().withTrimmedRight(browserPanelWidth));

    viewport->setBounds(playNamedPanel->getContentArea());
    auto w = viewport->getWidth() - viewport->getScrollBarThickness() - 2;
    viewportContents->setBounds(
        0, 0, w, multi::PartSidebarCard::height * scxt::numParts * (tallMode ? 2 : 1));
    for (int i = 0; i < scxt::numParts; ++i)
    {
        auto rb = rectangleForPart(i);

        partSidebars[i]->setBounds(
            rb.withWidth(multi::PartSidebarCard::width).withHeight(multi::PartSidebarCard::height));
    }

    int pt{0};

    for (const auto &ped : macroEditors)
    {
        int knobMargin{5};

        auto pBox = rectangleForPart(pt);
        // write it this way to make tall mode easier in the future
        auto totalS = w - multi::PartSidebarCard::width - knobMargin;
        int switchW = 32;
        if (!tallMode)
            totalS -= switchW - 2 * knobMargin;
        int off = 5;
        auto wid = (totalS - 2 * off) / (macrosPerPart / 2);
        for (int idx = 0; idx < macrosPerPart; idx++)
        {
            int xPos = multi::PartSidebarCard::width + knobMargin + off +
                       wid * (idx % (macrosPerPart / 2));
            int yPos = pBox.getY();
            if (tallMode)
            {
                if (idx >= macrosPerPart / 2)
                {
                    yPos += multi::PartSidebarCard::height;
                }
            }
            auto bx = juce::Rectangle<int>(xPos, yPos, 92, multi::PartSidebarCard::height);
            macroEditors[pt][idx]->setBounds(bx.reduced(3));
        }
        if (!tallMode)
        {
            auto rr = pBox.withTrimmedLeft(pBox.getWidth() - switchW - knobMargin)
                          .withTrimmedRight(knobMargin);
            auto ht = (rr.getHeight() - switchW) / 2;
            rr = rr.reduced(0, ht);
            skinnyPageSwitches[pt]->widget->setBounds(rr);
        }
        pt++;
    }
}

juce::Rectangle<int> PlayScreen::rectangleForPart(int part)
{
    if (!viewportContents)
        return {};
    auto b = viewportContents->getLocalBounds();
    auto xp{0};
    auto w{b.getWidth()};
    auto h{multi::PartSidebarCard::height * (tallMode ? 2 : 1) + interPartMargin};
    auto yp{h * part};

    return {xp, (int)yp, w, (int)h};
}
} // namespace scxt::ui