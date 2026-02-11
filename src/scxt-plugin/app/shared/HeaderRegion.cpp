/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
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

struct ActivityDisplay : juce::Component, HasEditor
{
    ActivityDisplay(SCXTEditor *e) : HasEditor(e) {}

    void setMessage(const std::string &m)
    {
        msg = m;
        repaint();
    }
    void paint(juce::Graphics &g)
    {
        float rectCorner = 1.5;

        auto b = getLocalBounds().reduced(1).toFloat();

        auto bg = editor->themeColor(theme::ColorMap::bg_2);
        g.setColour(bg);
        g.fillRoundedRectangle(b, rectCorner);

        g.setColour(editor->themeColor(theme::ColorMap::generic_content_high));
        g.setFont(editor->themeApplier.interMediumFor(14));
        g.drawText(msg, getLocalBounds(), juce::Justification::centred);
    }
    std::string msg{};
};

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
    scMenu->setTitle("Main Menu");
    scMenu->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->editor->showMainMenu();
    });
    addAndMakeVisible(*scMenu);

    if (hasFeature::undoRedo)
    {
        undoButton = std::make_unique<jcmp::TextPushButton>();
        undoButton->setLabel("Undo");
        undoButton->setOnCallback(editor->makeComingSoon());
        addAndMakeVisible(*undoButton);

        redoButton = std::make_unique<jcmp::TextPushButton>();
        redoButton->setLabel("Redo");
        redoButton->setOnCallback(editor->makeComingSoon());
        addAndMakeVisible(*redoButton);
    }

    tuningButton = std::make_unique<jcmp::TextPushButton>();
    tuningButton->setLabel("Tune");
    tuningButton->setTitle("Tuning");
    tuningButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        juce::PopupMenu p;
        w->editor->addTuningMenu(p, true);
        p.showMenuAsync(w->editor->defaultPopupMenuOptions(w->tuningButton.get()));
    });
    addAndMakeVisible(*tuningButton);

    omniButton = std::make_unique<jcmp::TextPushButton>();
    std::string on;
    switch (editor->currentOmniFlavor)
    {
    case 0:
        on = "OMNI";
        break;
    case 1:
        on = "MPE";
        break;
    case 2:
        on = "Ch/Oct";
    }
    omniButton->setLabel(on);
    omniButton->setTitle(on);

    omniButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        juce::PopupMenu p;
        w->editor->addOmniFlavorMenu(p);
        p.showMenuAsync(w->editor->defaultPopupMenuOptions(w->omniButton.get()));
    });
    addAndMakeVisible(*omniButton);

    if (hasFeature::memoryUsageExplanation)
    {
        chipButton = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::MEMORY);
        chipButton->setOnCallback(editor->makeComingSoon());
        addAndMakeVisible(*chipButton);
    }

    saveAsButton = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::SAVE);
    saveAsButton->setTitle("Save and Load");
    saveAsButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;

        w->showSaveMenu();
        /// used to do stuff with auto mod = juce::ModifierKeys::currentModifiers;
    });
    addAndMakeVisible(*saveAsButton);

    multiMenuButton = std::make_unique<jcmp::MenuButton>();
    multiMenuButton->setLabel("Multi and Instrument IO");
    multiMenuButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showMultiSelectionMenu();
    });
    addAndMakeVisible(*multiMenuButton);

    activityDisplay = std::make_unique<ActivityDisplay>(editor);
    addChildComponent(*activityDisplay);

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

    if (hasFeature::undoRedo)
    {
        undoButton->setBounds(b.withTrimmedLeft(246).withWidth(48));
        redoButton->setBounds(b.withTrimmedLeft(246 + 50).withWidth(48));
    }

    tuningButton->setBounds(b.withTrimmedLeft(823).withWidth(48));
    omniButton->setBounds(b.withTrimmedLeft(823 + 50).withWidth(48));

    if (hasFeature::memoryUsageExplanation)
    {
        chipButton->setBounds(b.withTrimmedLeft(393).withWidth(24));
    }
    saveAsButton->setBounds(b.withTrimmedLeft(755).withWidth(24));

    multiMenuButton->setBounds(b.withTrimmedLeft(421).withWidth(330));
    activityDisplay->setBounds(multiMenuButton->getBounds());

    scMenu->setBounds(b.withTrimmedLeft(1248).withWidth(24));

    cpuLabel->setBounds(b.withTrimmedLeft(1022).withWidth(25).withHeight(14));
    ramLabel->setBounds(b.withTrimmedLeft(1022).withWidth(25).withHeight(14).translated(0, 14));

    cpuLevel->setBounds(b.withTrimmedLeft(1052).withWidth(48).withHeight(14));
    ramLevel->setBounds(b.withTrimmedLeft(1052).withWidth(48).withHeight(14).translated(0, 14));

    vuMeter->setBounds(b.withTrimmedLeft(1108).withWidth(136).withHeight(28));
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
    p.addSectionHeader("Save and Load");
    p.addSeparator();

    populateSaveMenu(p);
    p.addSeparator();

    addResetMenuItems(p);

    p.showMenuAsync(editor->defaultPopupMenuOptions(saveAsButton.get()));
}

void HeaderRegion::populateSaveMenu(juce::PopupMenu &p)
{
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
}

void HeaderRegion::showMultiSelectionMenu()
{
    auto p = juce::PopupMenu();
    juce::PopupMenu sm;
    populateSaveMenu(sm);
    p.addSubMenu("Save and Load", sm);
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
    p.addSectionHeader("Built-in Templates");
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

    try
    {
        auto userTemplatesPath = editor->browser.patchIODirectory / "Templates";
        if (fs::exists(userTemplatesPath))
        {
            std::vector<fs::path> tempCands;
            for (auto &p : fs::directory_iterator(userTemplatesPath))
            {
                tempCands.push_back(p.path());
            }
            for (auto &[pfx, title] : {std::make_pair(".scm", "Multi"), {".scp", "Part"}})
            {
                bool first{true};
                for (auto &cand : tempCands)
                {
                    if (extensionMatches(cand, pfx))
                    {
                        if (first)
                        {
                            p.addSeparator();
                            p.addSectionHeader(juce::String() + "User " + title + " Templates");
                            first = false;
                        }
                        auto dcand = cand;

                        p.addItem(dcand.replace_extension("").filename().u8string(),
                                  [w = juce::Component::SafePointer(this), pfx, cand]() {
                                      if (!w)
                                          return;
                                      if (std::string(pfx) == ".scm")
                                          w->sendToSerialization(cmsg::LoadMulti(cand.u8string()));
                                      if (std::string(pfx) == ".scp")
                                          w->sendToSerialization(cmsg::LoadPartInto(
                                              {cand.u8string(), w->editor->selectedPart}));
                                  });
                    }
                }
            }
        }
    }
    catch (fs::filesystem_error &e)
    {
        editor->displayError("Filesystem Error",
                             std::string() + "Unable to traverse user templates " + e.what());
    }
    menu.addSubMenu("Reset Engine To", p);
}

void HeaderRegion::onActivityNotification(int idx, const std::string &msg)
{
    if (idx == 0)
    {
        activityDisplay->setVisible(false);
        editor->setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }
    if (idx == 1)
    {
        activityDisplay->setVisible(true);
        activityDisplay->toFront(false);
        editor->setMouseCursor(juce::MouseCursor::WaitCursor);
    }
    activityDisplay->setMessage(msg);
}

} // namespace scxt::ui::app::shared