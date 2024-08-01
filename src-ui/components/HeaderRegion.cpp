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

#include "HeaderRegion.h"
#include "SCXTEditor.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/ToggleButtonRadioGroup.h"
#include "sst/jucegui/data/Discrete.h"
#include "widgets/ShortCircuitMenuButton.h"

namespace scxt::ui
{

namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

struct spData : sst::jucegui::data::Discrete
{
    HeaderRegion *headerRegion{nullptr};
    spData(HeaderRegion *hr) : headerRegion(hr) {}
    std::string getLabel() const override { return "Page"; }
    int getValue() const override
    {
        if (headerRegion && headerRegion->editor)
            return headerRegion->editor->activeScreen;
        return 0;
    }

    void setValueFromGUI(const int &f) override
    {
        if (headerRegion && headerRegion->editor)
        {
            headerRegion->editor->setActiveScreen((SCXTEditor::ActiveScreen)f);
            headerRegion->repaint();
        }
    }

    void setValueFromModel(const int &f) override {}

    std::string getValueAsStringFor(int i) const override
    {
        if (headerRegion && headerRegion->editor)
        {
            switch ((SCXTEditor::ActiveScreen)i)
            {
            case SCXTEditor::MULTI:
                return "EDIT";
            case SCXTEditor::MIXER:
                return "MIX";
            case SCXTEditor::PLAY:
                return "PLAY";
            }
        }
        assert(false);
        return "-error-";
    }

    int getMax() const override { return 2; }
};

HeaderRegion::HeaderRegion(SCXTEditor *e) : HasEditor(e)
{
    selectedPage = std::make_unique<sst::jucegui::components::ToggleButtonRadioGroup>();
    selectedPageData = std::make_unique<spData>(this);
    selectedPage->setSource(selectedPageData.get());

    addAndMakeVisible(*selectedPage);

    vuMeter = std::make_unique<sst::jucegui::components::VUMeter>();
    vuMeter->direction = sst::jucegui::components::VUMeter::HORIZONTAL;
    addAndMakeVisible(*vuMeter);

    scMenu = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::SHORTCIRCUIT_LOGO);
    scMenu->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->editor->showMainMenu();
    });
    addAndMakeVisible(*scMenu);

    undoButton = std::make_unique<jcmp::TextPushButton>();
    undoButton->setLabel("Undo");
    addAndMakeVisible(*undoButton);

    redoButton = std::make_unique<jcmp::TextPushButton>();
    redoButton->setLabel("Redo");
    addAndMakeVisible(*redoButton);

    tuningButton = std::make_unique<jcmp::TextPushButton>();
    tuningButton->setLabel("Tune");
    addAndMakeVisible(*tuningButton);

    zoomButton = std::make_unique<jcmp::TextPushButton>();
    zoomButton->setLabel("Zoom");
    addAndMakeVisible(*zoomButton);

    chipButton = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::MEMORY);
    addAndMakeVisible(*chipButton);

    saveAsButton = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::SAVE);
    addAndMakeVisible(*saveAsButton);

    multiMenuButton = std::make_unique<jcmp::MenuButton>();
    multiMenuButton->setLabel("This is my Super Awesome Multi");
    addAndMakeVisible(*multiMenuButton);

    cpuLabel = std::make_unique<jcmp::Label>();
    cpuLabel->setText("CPU");
    cpuLabel->setJustification(juce::Justification::centredLeft);
    addAndMakeVisible(*cpuLabel);

    cpuLevel = std::make_unique<jcmp::Label>();
    cpuLevel->setText("x%");
    cpuLevel->setJustification(juce::Justification::centredRight);
    addAndMakeVisible(*cpuLevel);

    ramLabel = std::make_unique<jcmp::Label>();
    ramLabel->setText("RAM");
    ramLabel->setJustification(juce::Justification::centredLeft);
    addAndMakeVisible(*ramLabel);

    ramLevel = std::make_unique<jcmp::Label>();
    ramLevel->setText("xMB");
    ramLevel->setJustification(juce::Justification::centredRight);
    addAndMakeVisible(*ramLevel);

    editor->themeApplier.applyHeaderTheme(this);
}

HeaderRegion::~HeaderRegion()
{
    if (selectedPage)
        selectedPage->setSource(nullptr);
}

void HeaderRegion::resized()
{
    auto b = getBounds().reduced(6);
    selectedPage->setBounds(b.withWidth(196));

    undoButton->setBounds(b.withTrimmedLeft(246).withWidth(48));
    redoButton->setBounds(b.withTrimmedLeft(246 + 50).withWidth(48));

    tuningButton->setBounds(b.withTrimmedLeft(823).withWidth(48));
    zoomButton->setBounds(b.withTrimmedLeft(823 + 50).withWidth(48));

    chipButton->setBounds(b.withTrimmedLeft(393).withWidth(24));
    saveAsButton->setBounds(b.withTrimmedLeft(755).withWidth(24));

    multiMenuButton->setBounds(b.withTrimmedLeft(421).withWidth(330));

    scMenu->setBounds(b.withTrimmedLeft(1148).withWidth(24));

    cpuLabel->setBounds(b.withTrimmedLeft(979).withWidth(20).withHeight(12));
    ramLabel->setBounds(b.withTrimmedLeft(979).withWidth(20).withHeight(12).translated(0, 12));

    cpuLevel->setBounds(b.withTrimmedLeft(1002).withWidth(35).withHeight(12));
    ramLevel->setBounds(b.withTrimmedLeft(1002).withWidth(35).withHeight(12).translated(0, 12));

    vuMeter->setBounds(b.withTrimmedLeft(1048).withWidth(96));
}

void HeaderRegion::setVULevel(float L, float R)
{
    if (vuMeter)
    {
        float ub = 1.4f;
        auto nvuL = sqrt(std::clamp(L, 0.f, ub)) / sqrt(ub);
        auto nvuR = sqrt(std::clamp(R, 0.f, ub)) / sqrt(ub);

        if (std::fabs(nvuL - vuLevel[0]) + std::fabs(nvuR - vuLevel[1]) > 1e-6)
        {
            vuLevel[0] = nvuL;
            vuLevel[1] = nvuR;

            vuMeter->setLevels(vuLevel[0], vuLevel[1]);
            vuMeter->repaint();
        }
    }
}

} // namespace scxt::ui