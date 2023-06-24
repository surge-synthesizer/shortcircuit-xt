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

#include "BrowserPane.h"
#include "components/SCXTEditor.h"
#include "browser/browser.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/NamedPanelDivider.h"

namespace scxt::ui::browser
{
namespace jcmp = sst::jucegui::components;

struct TempPane : HasEditor, juce::Component
{
    std::string label;
    TempPane(SCXTEditor *e, const std::string &s) : HasEditor(e), label(s) {}
    void paint(juce::Graphics &g) override
    {
        auto ft = editor->style()->getFont(jcmp::Label::Styles::styleClass,
                                           jcmp::Label::Styles::controlLabelFont);
        g.setFont(ft.withHeight(20));
        g.setColour(juce::Colours::white);
        g.drawText(label, getLocalBounds().withTrimmedBottom(20), juce::Justification::centred);

        g.setFont(ft.withHeight(14));
        g.setColour(juce::Colours::white);
        g.drawText("Coming Soon", getLocalBounds().withTrimmedTop(20),
                   juce::Justification::centred);
    }
};

struct DriveArea : juce::Component, HasEditor
{
    BrowserPane *browserPane{nullptr};
    DriveArea(BrowserPane *b, SCXTEditor *e) : browserPane(b), HasEditor(e){};

    void paint(juce::Graphics &g)
    {
        auto yp = 5, xp = 5;
        g.setFont(10);
        g.setColour(juce::Colours::pink);
        for (const auto &[path, desc] : browserPane->roots)
        {
            auto render = desc;
            if (render.empty())
                render = path.u8string();
            g.drawText(render, xp, yp, getWidth(), 20, juce::Justification::topLeft);
            yp += 20;
        }
        auto c = browserPane->style()->getColour(jcmp::NamedPanel::Styles::styleClass,
                                                 jcmp::NamedPanel::Styles::regionBorder);
        g.setColour(c);
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 2, 1);
    }
};

struct DriveFSArea : juce::Component, HasEditor
{
    BrowserPane *browserPane{nullptr};
    DriveFSArea(BrowserPane *b, SCXTEditor *e) : browserPane(b), HasEditor(e){};

    void paint(juce::Graphics &g)
    {
        auto c = browserPane->style()->getColour(jcmp::NamedPanel::Styles::styleClass,
                                                 jcmp::NamedPanel::Styles::regionBorder);
        g.setColour(c);
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 2, 1);
    }
};

struct DevicesPane : HasEditor, juce::Component
{
    BrowserPane *browserPane{nullptr};
    std::unique_ptr<DriveArea> driveArea;
    std::unique_ptr<DriveFSArea> driveFSArea;
    std::unique_ptr<jcmp::NamedPanelDivider> divider;
    DevicesPane(BrowserPane *p) : browserPane(p), HasEditor(p->editor)
    {
        driveArea = std::make_unique<DriveArea>(p, editor);
        addAndMakeVisible(*driveArea);
        driveFSArea = std::make_unique<DriveFSArea>(p, editor);
        addAndMakeVisible(*driveFSArea);
        divider = std::make_unique<jcmp::NamedPanelDivider>();
        addAndMakeVisible(*divider);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        auto drives = b.withHeight(200);
        auto div = b.withTrimmedTop(200).withHeight(8);
        auto rest = b.withTrimmedTop(228);

        driveArea->setBounds(drives);
        divider->setBounds(div);
        driveFSArea->setBounds(rest);
    }
};

struct FavoritesPane : TempPane
{
    FavoritesPane(SCXTEditor *e) : TempPane(e, "Favorites") {}
};

struct SearchPane : TempPane
{
    SearchPane(SCXTEditor *e) : TempPane(e, "Search") {}
};

struct sfData : sst::jucegui::data::Discrete
{
    BrowserPane *browserPane{nullptr};
    sfData(BrowserPane *hr) : browserPane(hr) {}
    std::string getLabel() const override { return "Function"; }
    int getValue() const override { return browserPane->selectedPane; }

    void setValueFromGUI(const int &f) override
    {
        if (browserPane)
        {
            browserPane->selectPane(f);
        }
    }

    void setValueFromModel(const int &f) override {}

    std::string getValueAsStringFor(int i) const override
    {
        if (browserPane)
        {
            switch (i)
            {
            case 0:
                return "DEVICES";
            case 1:
                return "FAVORITES";
            case 2:
                return "SEARCH";
            }
        }
        SCLOG("getValueAsStringFor with invalid value " << i);
        assert(false);
        return "-error-";
    }

    int getMax() const override { return 2; }
};

BrowserPane::BrowserPane(SCXTEditor *e)
    : HasEditor(e), sst::jucegui::components::NamedPanel("Browser")
{
    hasHamburger = true;

    selectedFunction = std::make_unique<sst::jucegui::components::ToggleButtonRadioGroup>();
    selectedFunctionData = std::make_unique<sfData>(this);
    selectedFunction->setSource(selectedFunctionData.get());

    addAndMakeVisible(*selectedFunction);

    devicesPane = std::make_unique<DevicesPane>(this);
    addChildComponent(*devicesPane);
    favoritesPane = std::make_unique<FavoritesPane>(e);
    addChildComponent(*favoritesPane);
    searchPane = std::make_unique<SearchPane>(e);
    addChildComponent(*searchPane);

    selectPane(selectedPane);

    resetRoots();
}

BrowserPane::~BrowserPane() { selectedFunction->setSource(nullptr); }

void BrowserPane::resized()
{
    auto r = getContentArea();
    auto sel = r.withHeight(22);
    auto ct = r.withTrimmedTop(25);
    selectedFunction->setBounds(sel);

    devicesPane->setBounds(ct);
    favoritesPane->setBounds(ct);
    searchPane->setBounds(ct);
}

void BrowserPane::resetRoots()
{
    roots = editor->browser.getRootPathsForDeviceView();
    repaint();
}

void BrowserPane::selectPane(int i)
{
    selectedPane = i;

    devicesPane->setVisible(i == 0);
    favoritesPane->setVisible(i == 1);
    searchPane->setVisible(i == 2);
    repaint();
}
} // namespace scxt::ui::browser
