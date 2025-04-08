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
#include "app/SCXTEditor.h"
#include "app/shared/PatchMultiIO.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/ToggleButtonRadioGroup.h"
#include "sst/jucegui/data/Discrete.h"

namespace scxt::ui::app::shared
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
    undoButton->setOnCallback(editor->makeComingSoon());
    addAndMakeVisible(*undoButton);

    redoButton = std::make_unique<jcmp::TextPushButton>();
    redoButton->setLabel("Redo");
    redoButton->setOnCallback(editor->makeComingSoon());
    addAndMakeVisible(*redoButton);

    tuningButton = std::make_unique<jcmp::TextPushButton>();
    tuningButton->setLabel("Tune");
    tuningButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        juce::PopupMenu p;
        w->editor->addTuningMenu(p, true);
        p.showMenuAsync(w->editor->defaultPopupMenuOptions(w->tuningButton.get()));
    });
    addAndMakeVisible(*tuningButton);

    zoomButton = std::make_unique<jcmp::TextPushButton>();
    zoomButton->setLabel("Zoom");
    zoomButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        juce::PopupMenu p;
        w->editor->addZoomMenu(p, true);
        p.showMenuAsync(w->editor->defaultPopupMenuOptions(w->zoomButton.get()));
    });
    addAndMakeVisible(*zoomButton);

    chipButton = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::MEMORY);
    chipButton->setOnCallback(editor->makeComingSoon());
    addAndMakeVisible(*chipButton);

    saveAsButton = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::SAVE);
    saveAsButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;

        w->showSaveMenu();
        /// used to do stuff with auto mod = juce::ModifierKeys::currentModifiers;
    });
    addAndMakeVisible(*saveAsButton);

    multiMenuButton = std::make_unique<jcmp::MenuButton>();
    multiMenuButton->setLabel("This is my Super Awesome Multi");
    multiMenuButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showMultiSelectionMenu();
    });
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
    editor->themeApplier.setLabelToHighlight(cpuLevel.get());
    editor->themeApplier.setLabelToHighlight(ramLevel.get());
    editor->themeApplier.applyHeaderSCButtonTheme(scMenu.get());
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

    cpuLabel->setBounds(b.withTrimmedLeft(962).withWidth(25).withHeight(14));
    ramLabel->setBounds(b.withTrimmedLeft(962).withWidth(25).withHeight(14).translated(0, 14));

    cpuLevel->setBounds(b.withTrimmedLeft(992).withWidth(48).withHeight(14));
    ramLevel->setBounds(b.withTrimmedLeft(992).withWidth(48).withHeight(14).translated(0, 14));

    vuMeter->setBounds(b.withTrimmedLeft(1048).withWidth(96).withHeight(28));
}

void HeaderRegion::setVULevel(float L, float R)
{
    if (vuMeter)
    {
        float ub = 8.f;
        auto nvuL = sqrt(std::clamp(L, 0.f, ub)); // sqrt(ub);
        auto nvuR = sqrt(std::clamp(R, 0.f, ub)); // sqrt(ub);

        if (std::fabs(nvuL - vuLevel[0]) + std::fabs(nvuR - vuLevel[1]) > 1e-6)
        {
            vuLevel[0] = nvuL;
            vuLevel[1] = nvuR;

            vuMeter->setLevels(vuLevel[0], vuLevel[1]);
            vuMeter->repaint();
        }
    }
}

void HeaderRegion::setCPULevel(float lev)
{
    if (lev < 1)
    {
        if (cpuLevValue != 0)
        {
            cpuLevValue = lev;
            cpuLevel->setText(fmt::format("{:.0f} %", 0.0));
        }
        return;
    }
    if (std::fabs(cpuLevValue - lev) > 1.5)
    {
        cpuLevValue = lev;
        cpuLevel->setText(fmt::format("{:.0f} %", lev));
    }
}

bool HeaderRegion::isInterestedInFileDrag(const juce::StringArray &files)
{
    return files.size() == 1 && std::all_of(files.begin(), files.end(), [this](const auto &f) {
               try
               {
                   auto pt = fs::path{(const char *)(f.toUTF8())};
                   return editor->browser.isShortCircuitFormatFile(pt);
               }
               catch (fs::filesystem_error &e)
               {
               }
               return false;
           });
}

void HeaderRegion::filesDropped(const juce::StringArray &files, int x, int y)
{
    if (files.size() != 1)
        return;
    auto pt = fs::path{(const char *)(files[0].toUTF8())};
    std::string what = "Part into Selected Part";
    std::string ws = "Part";
    auto isPt = true;
    if (extensionMatches(pt, ".scm"))
    {
        what = "Multi into Engine";
        ws = "Engine";
        isPt = false;
    }
    editor->promptOKCancel("Load " + what,
                           "By loading " + what + " you will clear the " + ws +
                               " and replace it with the contents of the file. Continue?",
                           [w = juce::Component::SafePointer(this), pt, isPt]() {
                               if (!w)
                                   return;
                               if (isPt)
                                   w->sendToSerialization(cmsg::LoadPartInto(
                                       {pt.u8string(), w->editor->selectedPart}));
                               else
                                   w->sendToSerialization(cmsg::LoadMulti(pt.u8string()));
                           });
}

void HeaderRegion::doSaveMulti(patch_io::SaveStyles style)
{
    shared::doSaveMulti(this, fileChooser, style);
}

void HeaderRegion::doLoadMulti() { shared::doLoadMulti(this, fileChooser); }

void HeaderRegion::doSaveSelectedPart(patch_io::SaveStyles styles)
{
    shared::doSavePart(this, fileChooser, editor->selectedPart, styles);
}

void HeaderRegion::doLoadIntoSelectedPart()
{
    shared::doLoadPartInto(this, fileChooser, editor->selectedPart);
}

void HeaderRegion::showSaveMenu()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Save");
    p.addSeparator();
    p.addItem("Save Multi Only", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->doSaveMulti(patch_io::SaveStyles::NO_SAMPLES);
    });
    p.addItem("Save Multi as Monolith", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->doSaveMulti(patch_io::SaveStyles::AS_MONOLITH);
    });
    p.addItem("Save Multi with Collected Samples", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->doSaveMulti(patch_io::SaveStyles::COLLECT_SAMPLES);
    });
    p.addSeparator();
    p.addItem("Save Part " + std::to_string(editor->selectedPart + 1) + " Only",
              [w = juce::Component::SafePointer(this)]() {
                  if (w)
                      w->doSaveSelectedPart(patch_io::SaveStyles::NO_SAMPLES);
              });

    p.addItem("Save Part " + std::to_string(editor->selectedPart + 1) + " as Monolith",
              [w = juce::Component::SafePointer(this)]() {
                  if (w)
                      w->doSaveSelectedPart(patch_io::SaveStyles::AS_MONOLITH);
              });
    p.addItem("Save Part " + std::to_string(editor->selectedPart + 1) + " with Collected Samples",
              [w = juce::Component::SafePointer(this)]() {
                  if (w)
                      w->doSaveSelectedPart(patch_io::SaveStyles::COLLECT_SAMPLES);
              });

    p.addSeparator();
    p.addItem("Load Multi", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->doLoadMulti();
    });
    p.addItem("Load Part Into " + std::to_string(editor->selectedPart + 1),
              [w = juce::Component::SafePointer(this)]() {
                  if (w)
                      w->doLoadIntoSelectedPart();
              });
    p.addSeparator();

    addResetMenuItems(p);

    p.showMenuAsync(editor->defaultPopupMenuOptions(saveAsButton.get()));
}

void HeaderRegion::showMultiSelectionMenu()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Multis - Coming Soon");
    p.addSeparator();
    addResetMenuItems(p);
    p.showMenuAsync(editor->defaultPopupMenuOptions(multiMenuButton.get()));
}

void HeaderRegion::addResetMenuItems(juce::PopupMenu &menu)
{
    /*
     * If you want to add an item here then
     *
     * 1. Make the SCM you want
     * 2. Build the init-maker
     * 3. Run it to convert the init-maker to resources/init_states/foo.dat
     * 4. Edit resources/CmakeLists.txt to add init_states/foo.dat
     * 5. Add a menu item here saying 'Reset engine to Foo' which passes 'foo.dat'
     *
     * More or less
     */
    auto p = juce::PopupMenu();
    p.addItem("Empty Part", [w = juce::Component::SafePointer(this)]() {
        if (w)
        {
            w->sendToSerialization(cmsg::ResetEngine("InitSampler.dat"));
        }
    });
    p.addItem("16 Empty Parts", [w = juce::Component::SafePointer(this)]() {
        if (w)
        {
            w->sendToSerialization(cmsg::ResetEngine("InitSamplerMulti.dat"));
        }
    });
    p.addItem("Basic Synth", [w = juce::Component::SafePointer(this)]() {
        if (w)
        {
            w->sendToSerialization(cmsg::ResetEngine("InitSynth.dat"));
        }
    });
    menu.addSubMenu("Reset Engine To", p);
}

} // namespace scxt::ui::app::shared