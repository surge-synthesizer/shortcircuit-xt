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

#include "MacroMappingVariantPane.h"
#include "app/SCXTEditor.h"
#include "app/edit-screen/EditScreen.h"
#include "datamodel/metadata.h"
#include "selection/selection_manager.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/TabbedComponent.h"
#include "sst/jucegui/components/Viewport.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/ZoomContainer.h"
#include "sst/jucegui/components/ButtonPainter.h"
#include "browser/browser.h"
#include "sample/sample.h"
#include "connectors/PayloadDataAttachment.h"
#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"

#include "engine/part.h"

#include "app/edit-screen/components/mapping-pane/VariantDisplay.h"
#include "app/edit-screen/components/mapping-pane/MacroDisplay.h"
#include "app/edit-screen/components/mapping-pane/MappingDisplay.h"
#include "app/edit-screen/components/mapping-pane/SampleWaveform.h"

namespace scxt::ui::app::edit_screen
{

namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

struct MappingDisplay;

MacroMappingVariantPane::MacroMappingVariantPane(SCXTEditor *e)
    : sst::jucegui::components::NamedPanel(""), HasEditor(e)
{
    hasHamburger = true;
    isTabbed = true;
    tabNames = {"MACROS", "MAPPING", "SAMPLE"};
    selectTab(1);

    mappingDisplay = std::make_unique<MappingDisplay>(this);
    addAndMakeVisible(*mappingDisplay);

    sampleDisplay = std::make_unique<VariantDisplay>(this);
    addChildComponent(*sampleDisplay);

    macroDisplay = std::make_unique<MacroDisplay>(editor);
    addChildComponent(*macroDisplay);

    resetTabState();

    onTabSelected = [wt = juce::Component::SafePointer(this)](int i) {
        if (wt)
        {
            wt->setSelectedTab(i);
        }
    };

    onHamburger = [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->showHamburgerMenu();
    };
}

MacroMappingVariantPane::~MacroMappingVariantPane() {}

void MacroMappingVariantPane::setSelectedTab(int i)
{
    if (i < 0 || i > 2)
        return;
    selectedTab = i;
    sampleDisplay->setVisible(i == 2);
    mappingDisplay->setVisible(i == 1);
    macroDisplay->setVisible(i == 0);

    hasHamburger = (i == 1 || i == 2);

    repaint();
    editor->setTabSelection(editor->editScreen->tabKey("multi.mapping"), std::to_string(i));
}

void MacroMappingVariantPane::showHamburgerMenu()
{
    switch (selectedTab)
    {
    case 0:
        break;
    case 1:
        mappingDisplay->showHamburgerMenu();
        break;
    case 2:
        sampleDisplay->showHamburgerMenu();
    }
}

void MacroMappingVariantPane::resized()
{
    mappingDisplay->setBounds(getContentArea());
    sampleDisplay->setBounds(getContentArea());
    macroDisplay->setBounds(getContentArea());
}

void MacroMappingVariantPane::setMappingData(const engine::Zone::ZoneMappingData &m)
{
    mappingView = m;
    mappingDisplay->updatePitchBendEnablement();
    mappingDisplay->repaint();
}

void MacroMappingVariantPane::setSampleData(const engine::Zone::Variants &m)
{
    sampleView = m;
    sampleDisplay->rebuildForSelectedVariation(sampleDisplay->selectedVariation, true);
    mappingDisplay->zoneHeader->refreshFileNavigator();
}

void MacroMappingVariantPane::setActive(bool b)
{
    mappingDisplay->setActive(b);
    mappingDisplay->setVisible(selectedTab == 1);
    sampleDisplay->setActive(b);
    sampleDisplay->setVisible(selectedTab == 2);
    macroDisplay->setVisible(selectedTab == 0);
    mappingDisplay->zoneHeader->refreshFileNavigator();
}

void MacroMappingVariantPane::setGroupZoneMappingSummary(
    const engine::Part::zoneMappingSummary_t &d)
{
    mappingDisplay->setGroupZoneMappingSummary(d);
}

void MacroMappingVariantPane::setMappingLockFromModel(bool b)
{
    mappingDisplay->zoneHeader->lockButton->setValueFromModel(b);
}

void MacroMappingVariantPane::setMappingHeaderState(int32_t midiZoneAction, bool zoneSolo)
{
    mappingDisplay->zoneHeader->setStateFromModel(midiZoneAction, zoneSolo);
}

void MacroMappingVariantPane::setEditAllFromModel(bool b)
{
    sampleDisplay->editAllButton->setValueFromModel(b);
}

void MacroMappingVariantPane::setShowZoneNamesFromModel(bool b)
{
    mappingDisplay->setShowZoneNames(b);
}

void MacroMappingVariantPane::editorSelectionChanged()
{
    if (editor->currentLeadZoneSelection.has_value())
        mappingDisplay->setLeadSelection(*(editor->currentLeadZoneSelection));

    sampleDisplay->rebuildForSelectedVariation(sampleDisplay->selectedVariation, true);
    repaint();
}

void MacroMappingVariantPane::addSamplePlaybackPosition(size_t sampleIndex, int64_t samplePos)
{
    if (sampleDisplay->isVisible() && sampleIndex == sampleDisplay->selectedVariation)
        sampleDisplay->waveforms[sampleDisplay->selectedVariation]
            .waveform->addSamplePlaybackPosition(samplePos);
}

void MacroMappingVariantPane::clearSamplePlaybackPositions()
{
    sampleDisplay->waveforms[sampleDisplay->selectedVariation]
        .waveform->clearSamplePlaybackPositions();
}

void MacroMappingVariantPane::selectedPartChanged() { macroDisplay->selectedPartChanged(); }
void MacroMappingVariantPane::macroDataChanged(int part, int index)
{
    assert(part == editor->selectedPart);
    macroDisplay->macroDataChanged(part, index);
}

void GlyphToggleButton::paint(juce::Graphics &g)
{
    namespace jcmp = sst::jucegui::components;
    auto v = data && data->getValue();
    if (v)
        jcmp::paintButtonOnValueBG(this, g);
    else
        jcmp::paintButtonBG(this, g);

    auto col = getColour(Styles::labelcolor).withAlpha(0.5f);
    if (isEnabled())
    {
        if (v)
            col = getColour(isHovered ? Styles::value_hover : Styles::value);
        else
            col = getColour(isHovered ? Styles::labelcolor_hover : Styles::labelcolor);
    }

    auto square = [](const juce::Rectangle<int> &b) {
        auto d = std::abs(b.getWidth() - b.getHeight()) / 2;
        return b.getWidth() > b.getHeight() ? b.reduced(d, 0) : b.reduced(0, d);
    };
    if (secondGlyph.has_value())
    {
        auto zw = glyphZeroWidth > 0 ? glyphZeroWidth : getWidth() / 2;
        auto lb = getLocalBounds();
        jcmp::GlyphPainter::paintGlyph(g, square(lb.withWidth(zw)), glyph, col);
        jcmp::GlyphPainter::paintGlyph(g, square(lb.withTrimmedLeft(zw)), *secondGlyph, col);
    }
    else
    {
        jcmp::GlyphPainter::paintGlyph(g, square(getLocalBounds()), glyph, col);
    }
}

MappingZoneHeader::MappingZoneHeader(MappingDisplay *d) : HasEditor(d->editor), display(d)
{
    using mza = engine::Engine::MidiZoneAction;
    autoMap = std::make_unique<jcmp::TextPushButton>();
    autoMap->setLabel("AUTO-MAP");
    autoMap->setOnCallback(editor->makeComingSoon());
    addAndMakeVisible(*autoMap);

    auto makeMidiToggle = [this](bool &value, mza action,
                                 std::optional<jcmp::GlyphPainter::GlyphType> second) {
        auto res = std::make_unique<toggle_t<GlyphToggleButton>>(value);
        res->widget->secondGlyph = second;
        res->widget->glyphZeroWidth = 16;
        res->onValueChanged = [w = juce::Component::SafePointer(this), action](bool v) {
            if (w)
                w->initiateMidiZoneAction(v ? action : mza::NONE);
        };
        addAndMakeVisible(*res->widget);
        return res;
    };
    midiButton = makeMidiToggle(midiSelectOn, mza::SELECT, std::nullopt);
    midiButton->widget->setTitle("MIDI Select");
    midiLRButton = makeMidiToggle(keyLearnOn, mza::SET_KEY_BOUNDS_START,
                                  jcmp::GlyphPainter::GlyphType::LEFT_RIGHT);
    midiLRButton->widget->setTitle("Learn Key Range");
    midiUDButton = makeMidiToggle(velLearnOn, mza::SET_VEL_BOUNDS_START,
                                  jcmp::GlyphPainter::GlyphType::UP_DOWN);
    midiUDButton->widget->setTitle("Learn Velocity Range");

    // no zones means the selection
    auto batchButton = [this](const std::string &label, engine::Engine::ZoneBatchOp op) {
        auto res = std::make_unique<jcmp::TextPushButton>();
        res->setLabel(label);
        res->setOnCallback([w = juce::Component::SafePointer(this), op]() {
            if (w)
                w->sendToSerialization(cmsg::ApplyZoneBatchOp({{}, (int32_t)op}));
        });
        addAndMakeVisible(*res);
        return res;
    };
    fixOverlap = batchButton("FIX OVERLAP", engine::Engine::FIX_OVERLAPS);
    fadeOverlap = batchButton("FADE OVERLAP", engine::Engine::AUTO_CROSSFADES);

    zoneSolo = std::make_unique<toggle_t<jcmp::ToggleButton>>(zoneSoloOn);
    zoneSolo->widget->setLabel("ZONE SOLO");
    zoneSolo->onValueChanged = [w = juce::Component::SafePointer(this)](bool v) {
        if (w)
            w->sendToSerialization(cmsg::SetZoneSolo(v));
    };
    addAndMakeVisible(*zoneSolo->widget);

    lockButton = std::make_unique<toggle_t<jcmp::ToggleButton>>(d->mappingLocked);
    lockButton->widget->setGlyph(jcmp::GlyphPainter::GlyphType::LOCK);
    lockButton->widget->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH_WITH_BG);
    lockButton->onValueChanged = [w = juce::Component::SafePointer(this)](bool v) {
        if (w)
            w->editor->setTabSelection("mapping.lock", v ? "1" : "0");
    };
    addAndMakeVisible(*(lockButton->widget));

    fileLabel = std::make_unique<jcmp::Label>();
    fileLabel->setText("FILE");
    addAndMakeVisible(*fileLabel);

    prevFileButton = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::GlyphType::JOG_LEFT);
    prevFileButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->stepFile(false);
    });
    addAndMakeVisible(*prevFileButton);

    nextFileButton = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::GlyphType::JOG_RIGHT);
    nextFileButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->stepFile(true);
    });
    addAndMakeVisible(*nextFileButton);

    fileMenu = std::make_unique<jcmp::MenuButton>();
    fileMenu->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showFileMenu();
    });
    fileMenu->onIdleHover = [w = juce::Component::SafePointer(this)]() {
        if (!w || w->fileFullPath.empty())
            return;
        w->editor->setTooltipContents("Sample", w->fileFullPath);
        w->editor->showTooltip(*w->fileMenu);
    };
    fileMenu->onIdleHoverEnd = [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->editor->hideTooltip();
    };
    addAndMakeVisible(*fileMenu);

    refreshFileNavigator();
}

void MappingZoneHeader::initiateMidiZoneAction(engine::Engine::MidiZoneAction a)
{
    sendToSerialization(cmsg::InitiateMidiZoneAction{(int)a});
}

void MappingZoneHeader::setStateFromModel(int32_t action, bool solo)
{
    using mza = engine::Engine::MidiZoneAction;
    auto a = (mza)action;
    midiButton->setValueFromModel(a == mza::SELECT);
    midiLRButton->setValueFromModel(a == mza::SET_KEY_BOUNDS_START ||
                                    a == mza::SET_KEY_BOUNDS_SECOND);
    midiUDButton->setValueFromModel(a == mza::SET_VEL_BOUNDS_START ||
                                    a == mza::SET_VEL_BOUNDS_SECOND);
    zoneSolo->setValueFromModel(solo);
}

namespace
{
std::string variantFileName(const sample::Sample &s)
{
    auto res = s.getPath().empty() ? s.getDisplayName() : s.getPath().filename().u8string();
    if (!s.compoundSourceDetails.empty())
        res += " (" + s.compoundSourceDetails + ")";
    return res;
}

// a sample from a multisample container has no folder of siblings to step through
bool variantHasFolder(const sample::Sample &s)
{
    return !s.getPath().empty() && s.compoundSourceDetails.empty();
}
} // namespace

void MappingZoneHeader::refreshFileNavigator()
{
    const auto &lead = editor->currentLeadZoneSelection;
    auto hasZone = display->active && lead.has_value() && lead->zone >= 0;
    if (!hasZone || fileNavigatorZone != lead)
        fileVariant = 0;
    fileNavigatorZone = hasZone ? lead : std::nullopt;

    const auto &vars = display->parentPane->sampleView.variants;
    int activeCount{0};
    for (const auto &v : vars)
        activeCount += v.active ? 1 : 0;
    if (fileVariant < 0 || fileVariant >= (int)vars.size() || !vars[fileVariant].active)
    {
        fileVariant = 0;
        for (int i = 0; i < (int)vars.size(); ++i)
        {
            if (vars[i].active)
            {
                fileVariant = i;
                break;
            }
        }
    }

    std::string label;
    fileFullPath.clear();
    auto navigable{false};
    if (hasZone)
    {
        auto samp = vars[fileVariant].active
                        ? editor->sampleManager.getSample(vars[fileVariant].sampleID)
                        : nullptr;
        if (!samp)
        {
            label = "<Blank Zone>";
        }
        else
        {
            label = variantFileName(*samp);
            if (activeCount > 1)
                label += " [" + std::to_string(fileVariant + 1) + "]";
            fileFullPath = samp->getPath().u8string();
            navigable = variantHasFolder(*samp);
        }
    }
    fileMenu->setLabel(label);
    fileMenu->setEnabled(hasZone);
    prevFileButton->setEnabled(navigable);
    nextFileButton->setEnabled(navigable);
    fileMenu->repaint();
}

void MappingZoneHeader::stepFile(bool forward)
{
    if (fileNavigatorZone.has_value())
        display->parentPane->sampleDisplay->selectNextFile(forward, fileVariant);
}

void MappingZoneHeader::loadFileIntoVariant(const fs::path &p, int variant)
{
    auto za = editor->currentLeadZoneSelection;
    if (!za.has_value() || za->zone < 0)
        return;
    fileVariant = variant;
    sendToSerialization(
        cmsg::AddSampleInZone({p.u8string(), za->part, za->group, za->zone, variant}));
}

void MappingZoneHeader::showFileMenu()
{
    if (!fileNavigatorZone.has_value())
        return;

    const auto &vars = display->parentPane->sampleView.variants;
    std::vector<std::pair<int, std::shared_ptr<sample::Sample>>> samples;
    for (int i = 0; i < (int)vars.size(); ++i)
    {
        if (!vars[i].active)
            continue;
        if (auto s = editor->sampleManager.getSample(vars[i].sampleID))
            samples.emplace_back(i, s);
    }

    auto addItems = [w = juce::Component::SafePointer(this)](juce::PopupMenu &m, int variant,
                                                             const fs::path &current,
                                                             const std::vector<fs::path> &files) {
        for (const auto &f : files)
        {
            m.addItem(f.filename().u8string(), true, f == current, [w, f, variant]() {
                if (w)
                    w->loadFileIntoVariant(f, variant);
            });
        }
    };
    // the sample's folder, then each folder below it as a submenu
    auto addFolder = [&addItems](juce::PopupMenu &m, int variant, const sample::Sample &s) {
        if (!variantHasFolder(s))
        {
            m.addItem("No folder to browse", false, false, []() {});
            return;
        }
        auto dir = s.getPath().parent_path();
        addItems(m, variant, s.getPath(), browser::Browser::singleSamplesIn(dir));

        std::vector<fs::path> subdirs;
        try
        {
            for (const auto &e : fs::directory_iterator(dir))
                if (e.is_directory())
                    subdirs.push_back(e.path());
        }
        catch (const fs::filesystem_error &)
        {
        }
        std::sort(subdirs.begin(), subdirs.end(), [](const auto &a, const auto &b) {
            return juce::String(a.filename().u8string())
                       .compareNatural(juce::String(b.filename().u8string())) < 0;
        });
        auto first{true};
        for (const auto &sd : subdirs)
        {
            auto files = browser::Browser::singleSamplesIn(sd);
            if (files.empty())
                continue;
            if (first)
                m.addSeparator();
            first = false;
            juce::PopupMenu sub;
            addItems(sub, variant, s.getPath(), files);
            m.addSubMenu(sd.filename().u8string(), sub);
        }
    };

    juce::PopupMenu p;
    if (samples.empty())
    {
        p.addSectionHeader("<Blank Zone>");
        p.addItem("No sample to browse from", false, false, []() {});
    }
    else if (samples.size() == 1)
    {
        p.addSectionHeader(samples[0].second->getPath().parent_path().filename().u8string());
        addFolder(p, samples[0].first, *samples[0].second);
    }
    else
    {
        // a plain pick chooses which variant the arrows step
        p.addSectionHeader("Variants");
        for (const auto &[i, s] : samples)
        {
            auto name = variantFileName(*s) + " [" + std::to_string(i + 1) + "]";
            p.addItem(name, true, i == fileVariant,
                      [w = juce::Component::SafePointer(this), v = i]() {
                          if (!w)
                              return;
                          w->fileVariant = v;
                          w->refreshFileNavigator();
                      });
        }
        p.addSeparator();
        for (const auto &[i, s] : samples)
        {
            juce::PopupMenu sub;
            addFolder(sub, i, *s);
            p.addSubMenu("Browse [" + std::to_string(i + 1) + "] " + variantFileName(*s), sub);
        }
    }
    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

} // namespace scxt::ui::app::edit_screen
