/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#include "ProcessorPane.h"
#include "app/SCXTEditor.h"
#include "sst/plugininfra/strnatcmp.h"

#include "connectors/JsonLayoutEngineSupport.h"
#include "connectors/SCXTResources.h"

#include "app/shared/UIHelpers.h"

#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/RuledLabel.h"
#include "sst/jucegui/components/JogUpDownButton.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/layouts/ListLayout.h"
#include "sst/waveshapers/WaveshaperConfiguration.h"
#include "sst/filters/FilterConfigurationLabels.h"
#include <sst/jucegui/components/MenuButton.h>

#include "theme/Layout.h"

#include "sst/voice-effects/VoiceEffectsPresetSupport.h"

#include <sstream>
#include <fstream>

namespace scxt::ui::app::edit_screen
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

ProcessorPane::ProcessorPane(SCXTEditor *e, int index, bool fz)
    : HasEditor(e), jcmp::NamedPanel("PROCESSOR " + std::to_string(index + 1)), index(index),
      forZone(fz)
{
    setContentAreaComponent(std::make_unique<juce::Component>());
    setOptionalCursorForNameArea(juce::MouseCursor::DraggingHandCursor);
    hasHamburger = true;
    nameIsSelector = true;

    onHamburger = [safeThis = juce::Component::SafePointer(this)]() {
        if (safeThis)
            safeThis->showPresetsMenu();
    };
    onNameSelected = [safeThis = juce::Component::SafePointer(this)]() {
        if (safeThis)
            safeThis->showProcessorTypeMenu();
    };

    setTogglable(true);
    setupJsonTypeMap();
}

ProcessorPane::~ProcessorPane() { resetControls(); }

void ProcessorPane::resized()
{
    jcmp::NamedPanel::resized();
    rebuildControlsFromDescription();
}

void ProcessorPane::showProcessorTypeMenu()
{
    if (!isEnabled())
        return;

    juce::PopupMenu p;
    p.addSectionHeader("Processors");
    p.addSeparator();
    juce::PopupMenu subMenu{};
    std::string priorGroup = "none";
    for (const auto &pd : editor->allProcessors)
    {
        if (pd.groupOnly && forZone)
            continue;

        if (pd.displayGroup != priorGroup && priorGroup != "none")
        {
            p.addSubMenu(priorGroup, subMenu);
            subMenu = juce::PopupMenu();
        }

        auto &target = pd.displayGroup == "none" ? p : subMenu;

        target.addItem(pd.displayName, [wt = juce::Component::SafePointer(this), type = pd.id]() {
            if (wt)
            {
                wt->sendToSerialization(
                    cmsg::SetSelectedProcessorType({wt->forZone, wt->index, type}));
            }
        });

        priorGroup = pd.displayGroup;
    }
    p.addSubMenu(priorGroup, subMenu);

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

void ProcessorPane::showPresetsMenu()
{
    if (!isEnabled())
        return;

    juce::PopupMenu p;
    p.addSectionHeader("Presets");

    auto ps = sst::voice_effects::presets::factoryPresetPathsFor(
        dsp::processor::getProcessorStreamingName(processorView.type));
    if (!ps.empty())
    {
        p.addSeparator();
        std::sort(ps.begin(), ps.end(),
                  [](auto &a, auto &b) { return strnatcasecmp(a.c_str(), b.c_str()) < 0; });
        for (auto &item : ps)
        {
            auto pt = fs::path(item);
            auto mi = pt.filename().stem().u8string();
            p.addItem(mi, [ic = item, w = juce::Component::SafePointer(this)]() {
                if (!w)
                    return;

                auto xml = sst::voice_effects::presets::factoryPresetContentByPath(ic);
                w->applyPresetXML(xml);
            });
        }
    }

    try
    {
        std::vector<fs::path> userPresets;
        auto ud = editor->browser.voiceFxPresetDirectory;
        std::string dn = dsp::processor::getProcessorName(processorView.type);
        std::string sdn = dsp::processor::getProcessorStreamingName(processorView.type);
        std::string sstdn = dsp::processor::getProcessorSSTVoiceDisplayName(processorView.type);
        if (sstdn == dn)
            sstdn = "SKIPSKIP";
        for (auto &pc : {dn, sdn, sstdn})
        {
            if (pc == "SKIPSKIP")
                continue;
            auto pth = ud / pc;
            if (!fs::exists(pth))
            {
                continue;
            }

            for (auto &d : fs::directory_iterator(pth))
            {
                if (extensionMatches(d.path(), ".vcfx"))
                {
                    userPresets.push_back(d.path());
                }
            }
        }

        if (!userPresets.empty())
        {
            p.addSeparator();
            std::sort(userPresets.begin(), userPresets.end(), [](auto &a, auto &b) {
                return strnatcasecmp(a.filename().u8string().c_str(),
                                     b.filename().u8string().c_str()) < 0;
            });
            for (auto &item : userPresets)
            {
                auto pt = item.filename().stem().u8string();
                p.addItem(pt, [ic = item, w = juce::Component::SafePointer(this)]() {
                    if (!w)
                        return;
                    std::ifstream fst(ic);
                    if (!fst.is_open())
                    {
                        w->editor->displayError("Preset Load Error", "Unable to open preset file");
                        return;
                    }
                    std::stringstream iss;
                    iss << fst.rdbuf();
                    std::string xml((std::istreambuf_iterator<char>(iss)),
                                    std::istreambuf_iterator<char>());
                    w->applyPresetXML(xml);
                });
            }
        }
    }
    catch (fs::filesystem_error &e)
    {
    }
    p.addSeparator();
    p.addItem("Save Preset...", [w = juce::Component::SafePointer(this)] {
        if (w)
            w->savePreset();
    });
    p.addItem("Load Preset...", [w = juce::Component::SafePointer(this)] {
        if (w)
            w->loadPreset();
    });
    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

struct PresetProviderAdapter
{
    const ProcessorPane &p;
    PresetProviderAdapter(ProcessorPane &pn) : p(pn) {}

    std::string provideStreamingName() const
    {
        return dsp::processor::getProcessorStreamingName(p.processorView.type);
    }

    int provideStreamingVersion() const
    {
        return dsp::processor::getProcessorStreamingVersion(p.processorView.type);
    }
    size_t provideFloatParamCount() const
    {
        return dsp::processor::getProcessorFloatParamCount(p.processorView.type);
    }

    size_t provideIntParamCount() const
    {
        return dsp::processor::getProcessorIntParamCount(p.processorView.type);
    }

    float provideFloatParam(size_t idx) const { return p.processorView.floatParams[idx]; }

    bool provideDeactivated(size_t idx) const { return p.processorView.deactivated[idx]; }

    int provideIntParam(size_t idx) const { return p.processorView.intParams[idx]; }

    bool provideTemposync() const { return p.processorView.isTemposynced; }

    bool provideKeytrack() const { return p.processorView.isKeytracked; }
};

struct PresetReceiverAdapter
{
    ProcessorPane &p;
    int streamingVersion{-1};
    PresetReceiverAdapter(ProcessorPane &pn) : p(pn) {}

    bool canReceiveForStreamingName(const std::string &s)
    {
        auto sn = dsp::processor::getProcessorStreamingName(p.processorView.type);
        return sn == s;
    }

    bool receiveStreamingVersion(int v)
    {
        streamingVersion = v;
        return true;
    }

    bool receiveFloatParam(size_t idx, float f)
    {
        if (idx < 0 || idx >= maxProcessorFloatParams)
            return false;
        p.processorView.floatParams[idx] = f;
        return true;
    }
    bool receiveDeactivated(size_t idx, bool b)
    {
        if (idx < 0 || idx >= maxProcessorFloatParams)
            return false;
        p.processorView.deactivated[idx] = b;

        return true;
    }
    bool receiveIntParam(size_t idx, int i)
    {
        if (idx < 0 || idx >= maxProcessorIntParams)
            return false;
        p.processorView.intParams[idx] = i;

        return true;
    }
    bool receiveTemposync(bool b)
    {
        p.processorView.isTemposynced = b;
        return true;
    }
    bool receiveKeytrack(bool b)
    {
        p.processorView.isKeytracked = b;
        return true;
    }

    void onError(const std::string &s) { p.editor->displayError("Preset Load Error", s); }
};

void ProcessorPane::savePreset()
{
    auto dir = editor->browser.voiceFxPresetDirectory;
    auto sfx = dsp::processor::getProcessorName(processorView.type);
    dir = dir / sfx;
    try
    {
        fs::create_directories(dir);
    }
    catch (fs::filesystem_error &)
    {
    }
    editor->fileChooser = std::make_unique<juce::FileChooser>("Save Voice Effect Preset",
                                                              juce::File(dir.u8string()), "*.vcfx");

    editor->fileChooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::warnAboutOverwriting,
        [w = juce::Component::SafePointer(this)](const juce::FileChooser &c) {
            if (!w)
                return;
            auto result = c.getResults();
            if (result.isEmpty() || result.size() > 1)
            {
                return;
            }
            auto ppa = PresetProviderAdapter(*w);
            auto psv = sst::voice_effects::presets::toPreset(ppa);
            if (!psv.empty())
            {
                if (!result[0].replaceWithText(psv))
                {
                    w->editor->displayError("File Save", "Unable to save file");
                }
            }
        });
}

void ProcessorPane::loadPreset()
{
    auto dir = editor->browser.voiceFxPresetDirectory;
    auto sfx = dsp::processor::getProcessorName(processorView.type);
    dir = dir / sfx;
    try
    {
        if (!fs::exists(dir))
            dir = editor->browser.voiceFxPresetDirectory;
    }
    catch (fs::filesystem_error &)
    {
    }
    editor->fileChooser = std::make_unique<juce::FileChooser>("Load Voice Effect Preset",
                                                              juce::File(dir.u8string()), "*.vcfx");

    editor->fileChooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::openMode,
        [w = juce::Component::SafePointer(this)](const juce::FileChooser &c) {
            if (!w)
                return;
            auto result = c.getResults();
            if (result.isEmpty() || result.size() > 1)
            {
                return;
            }
            auto str = result[0].loadFileAsString().toStdString();
            w->applyPresetXML(str);
        });
}

void ProcessorPane::applyPresetXML(const std::string &str)
{
    auto pvcopy = processorView;
    auto pra = PresetReceiverAdapter(*this);
    auto psv = sst::voice_effects::presets::fromPreset(str, pra);
    if (psv)
    {
        // TODO : Streaming version
        if (pra.streamingVersion !=
            dsp::processor::getProcessorStreamingVersion(processorView.type))
        {
            editor->displayError("Preset Load Error", "Preset version mismatch - code this!");
        }
        sendToSerialization(cmsg::SendFullProcessorStorage({forZone, index, processorView}));
        rebuildControlsFromDescription();
    }
    else
    {
        processorView = pvcopy;
    }
}
void ProcessorPane::resetControls()
{
    getContentAreaComponent()->removeAllChildren();
    clearAdditionalHamburgerComponents();

    // we assume the controls clear before attachments so make sure of that
    for (auto &k : floatEditors)
        k.reset(nullptr);
    for (auto &i : intEditors)
        i.reset(nullptr);
    mixEditor.reset(nullptr);
    otherEditors.clear();

    setToggleDataSource(nullptr);
}

void ProcessorPane::setupJsonTypeMap()
{
    auto a = [this](auto x, auto y) { jsonDefinitions[x] = std::string("voicefx-layouts/") + y; };

    a(dsp::processor::proct_VemberClassic, "filters/vember-classic.json");
    a(dsp::processor::proct_K35, "filters/k35.json");
    a(dsp::processor::proct_obx4, "filters/obxd-4pole.json");
    a(dsp::processor::proct_obxpander, "filters/obxd-Xpander.json");
    a(dsp::processor::proct_diodeladder, "filters/linear-ladder.json");
    a(dsp::processor::proct_cutoffwarp, "filters/warp.json");
    a(dsp::processor::proct_reswarp, "filters/warp.json");
    a(dsp::processor::proct_vintageladder, "filters/vintage-ladder.json");
    a(dsp::processor::proct_tripole, "filters/tripole.json");
    a(dsp::processor::proct_snhfilter, "filters/sample-and-hold.json");
    a(dsp::processor::proct_CytomicSVF, "filters/fast-svf.json");
    a(dsp::processor::proct_StaticPhaser, "filters/static-phaser.json");
    a(dsp::processor::proct_fmfilter, "filters/fm-filter.json");
    a(dsp::processor::proct_utilfilt, "filters/utility-filter.json");

    a(dsp::processor::proct_eq_tilt, "eq/tilt-eq.json");

    a(dsp::processor::proct_osc_correlatednoise, "generators/correlated-noise.json");
    a(dsp::processor::proct_osc_sineplus, "generators/sineplus.json");
    a(dsp::processor::proct_osc_tiltnoise, "generators/tilt-noise.json");
    a(dsp::processor::proct_osc_3op, "generators/threeop.json");

    a(dsp::processor::proct_comb, "resonators/comb-filter.json");
    a(dsp::processor::proct_stringResonator, "resonators/string-resonator.json");
    a(dsp::processor::proct_tetradResonator, "resonators/tetrad-resonator.json");

    a(dsp::processor::proct_Tremolo, "modulation/tremolo.json");
    a(dsp::processor::proct_Chorus, "modulation/chorus.json");
    a(dsp::processor::proct_Phaser, "modulation/phaser.json");
    a(dsp::processor::proct_flanger, "modulation/voice-flanger.json");
    a(dsp::processor::proct_shepard, "modulation/shepard.json");

    a(dsp::processor::proct_fx_microgate, "delay/microgate.json");
    a(dsp::processor::proct_lifted_delay, "delay/lifted-delay.json");
    a(dsp::processor::proct_fx_simple_delay, "delay/simple-delay.json");

    a(dsp::processor::proct_gainmatrix, "utility/gain-matrix.json");
    a(dsp::processor::proct_stereotool, "utility/stereo-tool.json");
    a(dsp::processor::proct_volpan, "utility/volume-and-pan.json");
    a(dsp::processor::proct_fx_widener, "utility/widener.json");

    a(dsp::processor::proct_fx_ringmod, "audio-rate-mod/ring-mod.json");
    a(dsp::processor::proct_fx_freqshiftmod, "audio-rate-mod/freq-shift-mod.json");
    a(dsp::processor::proct_noise_am, "audio-rate-mod/noise-am.json");
    a(dsp::processor::proct_osc_phasemod, "audio-rate-mod/phase-mod.json");

    a(dsp::processor::proct_fx_bitcrusher, "distortion/bitcrusher.json");
    a(dsp::processor::proct_fx_waveshaper, "distortion/waveshaper.json");

#if DEBUG
    for (auto &[_, s] : jsonDefinitions)
    {
        auto res = scxt::ui::connectors::jsonlayout::resolveJsonFile(s);
        if (!res.has_value())
        {
            SCLOG_IF(debug, "Unable to load JSON for " << s);
        }
    }
#endif
}

void ProcessorPane::rebuildControlsFromDescription()
{
    resetControls();

    if (!isEnabled() && !multiZone)
    {
        setToggleDataSource(nullptr);
        setName(processorControlDescription.typeDisplayName);
        repaint();
        return;
    }

    if (multiZone)
    {
        // it's a bit hacky to do this inline but I"m sure it will change
        setEnabled(true);
        getContentAreaComponent()->setEnabled(true);
        multiLabel = std::make_unique<sst::jucegui::components::Label>();
        multiLabel->setText("Multiple Types Selected");
        auto b = getContentAreaComponent()->getLocalBounds();
        multiLabel->setBounds(b.withTrimmedBottom(20));
        getContentAreaComponent()->addAndMakeVisible(*multiLabel);

        multiButton = std::make_unique<sst::jucegui::components::TextPushButton>();
        multiButton->setLabel("Copy " + multiName + " to All");
        multiButton->setBounds(b.withTop(b.getCentreY() + 5).withHeight(25).reduced(20, 0));
        multiButton->setOnCallback(
            [this]() { sendToSerialization(cmsg::CopyProcessorLeadToAll({forZone, index})); });
        getContentAreaComponent()->addAndMakeVisible(*multiButton);
        return;
    }

    setName(processorControlDescription.typeDisplayName);

    if (processorControlDescription.type == dsp::processor::proct_none)
    {
        setToggleDataSource(nullptr);
        repaint();
        return;
    }

    for (int i = 0; i < processorControlDescription.numFloatParams; ++i)
    {
        auto at = std::make_unique<attachment_t>(
            processorControlDescription.floatControlDescriptions[i], processorView.floatParams[i]);
        connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorFloatValue, attachment_t>(
            *at, processorView, this, forZone, index);

        floatAttachments[i] = std::move(at);

        if (processorControlDescription.floatControlDescriptions[i].canTemposync)
        {
            floatAttachments[i]->setTemposyncFunction(
                [w = juce::Component::SafePointer(this)](const auto &a) {
                    if (w)
                        return w->processorView.isTemposynced;
                    return false;
                });
        }

        deactivateAttachments[i].reset();
        if (processorControlDescription.floatControlDescriptions[i].canDeactivate)
        {
            auto dat = std::make_unique<bool_attachment_t>(
                "Deactivate " + processorControlDescription.floatControlDescriptions[i].name,
                processorView.deactivated[i]);
            connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorBoolValue,
                                         bool_attachment_t, bool_attachment_t::parent_t>(
                *dat, processorView, this, forZone, index);
            dat->isInverted = true; // on means not deactivated
            deactivateAttachments[i] = std::move(dat);
        }
    }

    for (int i = 0; i < processorControlDescription.numIntParams; ++i)
    {
        auto at = std::make_unique<int_attachment_t>(
            processorControlDescription.intControlDescriptions[i], processorView.intParams[i]);
        connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorInt32TValue, int_attachment_t>(
            *at, processorView, this, forZone, index);
        intAttachments[i] = std::move(at);
    }

    auto at = std::make_unique<attachment_t>(
        datamodel::describeValue(processorView, processorView.mix), processorView.mix);
    connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorFloatValue, attachment_t>(
        *at, processorView, this, forZone, index);
    mixAttachment = std::move(at);

    clearAdditionalHamburgerComponents();
    mixEditor = createWidgetAttachedTo<jcmp::Knob>(mixAttachment, "Mix");
    addAdditionalHamburgerComponent(std::move(mixEditor->item));

    auto jit = jsonDefinitions.find(processorControlDescription.type);
    if (jit != jsonDefinitions.end())
    {
        layoutControlsWithJsonEngine(jit->second);
    }
    else
    {
        switch (processorControlDescription.type)
        {
        case dsp::processor::proct_eq_3band_parametric_A:
            layoutControlsEQNBandParm();
            break;

        case dsp::processor::proct_eq_morph:
            layoutControlsEQMorph();
            break;

        case dsp::processor::proct_eq_6band:
            layoutControlsEQGraphic();
            break;

        case dsp::processor::proct_osc_EBWaveforms:
            layoutControlsEBWaveforms();
            break;

        default:
            layoutControls();
            break;
        }
    }

    if (processorControlDescription.supportsKeytrack)
    {
        keytrackAttackment =
            std::make_unique<bool_attachment_t>("Keytrack", processorView.isKeytracked);
        connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorBoolValue, bool_attachment_t,
                                     bool_attachment_t::onGui_t>(*keytrackAttackment, processorView,
                                                                 this, forZone, index);

        auto kta = std::make_unique<jcmp::ToggleButton>();
        kta->setDrawMode(sst::jucegui::components::ToggleButton::DrawMode::GLYPH);
        kta->setGlyph(sst::jucegui::components::GlyphPainter::KEYBOARD);
        kta->setSource(keytrackAttackment.get());
        setupWidgetForValueTooltip(kta.get(), keytrackAttackment);
        addAdditionalHamburgerComponent(std::move(kta));
    }

    if (processorControlDescription.supportsTemposync)
    {
        temposyncAttachment =
            std::make_unique<bool_attachment_t>("Temposync", processorView.isTemposynced);
        connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorBoolValue, bool_attachment_t,
                                     bool_attachment_t::onGui_t>(
            *temposyncAttachment, processorView, this, forZone, index);

        auto ts = std::make_unique<jcmp::ToggleButton>();
        ts->setDrawMode(sst::jucegui::components::ToggleButton::DrawMode::GLYPH);
        ts->setGlyph(sst::jucegui::components::GlyphPainter::METRONOME);
        ts->setSource(temposyncAttachment.get());
        setupWidgetForValueTooltip(ts.get(), temposyncAttachment);
        addAdditionalHamburgerComponent(std::move(ts));
    }

    setToggleDataSource(nullptr);
    bypassAttachment = std::make_unique<bool_attachment_t>(
        processorControlDescription.typeDisplayName + " Active", processorView.isActive);
    connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorBoolValue, bool_attachment_t,
                                 bool_attachment_t::onGui_t>(*bypassAttachment, processorView, this,
                                                             forZone, index);
    setToggleDataSource(bypassAttachment.get());
    setupWidgetForValueTooltip(toggleButton.get(), bypassAttachment);
    connectors::addGuiStep(*bypassAttachment,
                           [this](auto &a) { editor->processorBypassToggled(index); });

    reapplyStyle();

    repaint();
}

void ProcessorPane::toggleBypass()
{
    if (!bypassAttachment)
        return;
    bypassAttachment->setValueFromGUI(!bypassAttachment->getValue());
}

void ProcessorPane::layoutControls()
{
    auto labelHeight = 18;
    auto rc = getContentAreaComponent()->getLocalBounds();
    auto rcw = rc.getWidth();
    auto kw = rcw * 0.25;

    auto kb = rc.withHeight(kw).withWidth(kw);
    auto lb = kb.translated(0, kw).withHeight(labelHeight);

    for (int i = 0; i < processorControlDescription.numFloatParams; ++i)
    {
        floatEditors[i] = createWidgetAttachedTo(
            floatAttachments[i], processorControlDescription.floatControlDescriptions[i].name);
        floatEditors[i]->item->setBounds(kb.reduced(3));
        floatEditors[i]->label->setBounds(lb);

        auto label = std::make_unique<sst::jucegui::components::Label>();
        label->setText(processorControlDescription.floatControlDescriptions[i].name);
        getContentAreaComponent()->addAndMakeVisible(*label);

        kb = kb.translated(kw, 0);
        lb = lb.translated(kw, 0);
        if (i % 4 == 3)
        {
            kb = kb.translated(-getContentArea().getWidth(), kw + labelHeight);
            lb = lb.translated(-getContentArea().getWidth(), kw + labelHeight);
        }
    }

    for (int i = 0; i < processorControlDescription.numIntParams; ++i)
    {
        auto kn = std::make_unique<sst::jucegui::components::MultiSwitch>();
        kn->setSource(intAttachments[i].get());
        kn->setBounds(kb);
        getContentAreaComponent()->addAndMakeVisible(*kn);

        auto label = std::make_unique<sst::jucegui::components::Label>();
        label->setText(processorControlDescription.intControlDescriptions[i].name);
        label->setBounds(lb);
        getContentAreaComponent()->addAndMakeVisible(*label);

        intEditors[i] = std::make_unique<
            theme::layout::Labeled<sst::jucegui::components::DiscreteParamEditor>>();
        intEditors[i]->item = std::move(kn);
        intEditors[i]->label = std::move(label);

        kb = kb.translated(kw, 0);
        lb = lb.translated(kw, 0);
        if ((i + processorControlDescription.numFloatParams) % 4 == 3)
        {
            kb = kb.translated(-getContentArea().getWidth(), kw + labelHeight);
            lb = lb.translated(-getContentArea().getWidth(), kw + labelHeight);
        }
    }
}

// May want to break this up
void ProcessorPane::layoutControlsEBWaveforms()
{
    namespace jlo = sst::jucegui::layouts;
    createHamburgerStereo(3);

    auto isPulse = intAttachments[0]->getValue() == 2;
    auto isStereo = intAttachments[3]->getValue() == 1;
    auto isUnison = intAttachments[1]->getValue() > 1;

    auto bounds = getContentArea();

    auto lok = [bounds](auto &e, auto &row) {
        auto &item = e->item;
        auto &lab = e->label;
        auto kn = dynamic_cast<jcmp::Knob *>(item.get());
        if (kn)
        {
            kn->drawLabel = false;
        }
        auto vl = jlo::VList().withWidth(bounds.getWidth() / 4);
        vl.add(jlo::Component(*item).withHeight(bounds.getWidth() / 4).insetBy(4));
        vl.add(jlo::Component(*lab).expandToFill());
        row.add(vl);
    };

    auto main = jlo::VList().withHeight(bounds.getWidth()).at(0, 0);

    auto row1 = jlo::HList().withHeight(bounds.getHeight() / 2);

    auto res = createWidgetAttachedTo<jcmp::MultiSwitch>(intAttachments[0]);
    res->direction = jcmp::MultiSwitch::VERTICAL;
    row1.add(jlo::Component(*res).withWidth(bounds.getWidth() / 4));
    intEditors[0] = std::make_unique<intEditor_t>(std::move(res));

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], "Tune");
    lok(floatEditors[0], row1);
    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], "Sync");
    lok(floatEditors[1], row1);
    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], "PWM");
    lok(floatEditors[2], row1);

    if (isPulse)
    {
        floatEditors[2]->item->setEnabled(true);
        floatEditors[2]->label->setEnabled(true);
    }
    else
    {
        floatEditors[2]->item->setEnabled(false);
        floatEditors[2]->label->setEnabled(false);
    }
    main.add(row1);

    auto row2 = jlo::HList().withHeight(bounds.getHeight() / 2);

    auto vl1 = jlo::VList().withWidth(bounds.getWidth() / 4).withAutoGap(2);
    auto ures = createWidgetAttachedTo<jcmp::JogUpDownButton>(intAttachments[1]);
    intEditors[1] = std::make_unique<intEditor_t>(std::move(ures));
    intEditors[1]->label = std::make_unique<jcmp::Label>();
    intEditors[1]->label->setText("Unison");
    getContentAreaComponent()->addAndMakeVisible(*intEditors[1]->label);
    vl1.add(jlo::Component(*(intEditors[1]->label)).withHeight(16));
    vl1.add(jlo::Component(*(intEditors[1]->item)).withHeight(16));

    auto tres = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[4]);
    tres->setDrawMode(jcmp::ToggleButton::DrawMode::LABELED);
    tres->setLabel(std::string("Rand ") + u8"\U000003C6");
    intEditors[4] = std::make_unique<intEditor_t>(std::move(tres));
    vl1.add(jlo::Component(*(intEditors[4]->item)).withHeight(16));

    floatEditors[4] = createWidgetAttachedTo<jcmp::HSliderFilled>(floatAttachments[4], "W");

    auto wsl = jlo::HList().withHeight(16);
    wsl.add(jlo::Component(*(floatEditors[4]->label)).withWidth(16));
    wsl.add(jlo::Component(*(floatEditors[4]->item)).expandToFill().insetBy(0, 3));
    vl1.add(wsl);

    floatEditors[4]->item->setEnabled(isStereo && isUnison);
    floatEditors[4]->label->setEnabled(isStereo && isUnison);

    row2.add(vl1);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], "Detune");
    lok(floatEditors[3], row2);
    floatEditors[3]->item->setEnabled(isUnison);
    floatEditors[3]->label->setEnabled(isUnison);

    auto xtnd = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[2]);
    xtnd->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH);
    xtnd->setGlyph(jcmp::GlyphPainter::PLUS);
    intEditors[2] = std::make_unique<intEditor_t>(std::move(xtnd));
    intEditors[2]->item->setEnabled(isUnison);

    floatEditors[5] = createWidgetAttachedTo(floatAttachments[5], "Drift");
    lok(floatEditors[5], row2);
    floatEditors[6] = createWidgetAttachedTo(floatAttachments[6], "Level");
    lok(floatEditors[6], row2);

    main.add(row2);

    main.doLayout();

    auto detuneLoc = floatEditors[3]->item->getBounds();
    auto xtloc = juce::Rectangle<int>(detuneLoc.getRight() - 10, detuneLoc.getY() - 4, 14, 14);
    intEditors[2]->item->setBounds(xtloc);
}

void ProcessorPane::createHamburgerStereo(int attachmentId)
{
    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[attachmentId]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    setupWidgetForValueTooltip(stereo.get(), intAttachments[attachmentId]);
    addAdditionalHamburgerComponent(std::move(stereo));
    attachRebuildToIntAttachment(attachmentId);
}

void ProcessorPane::attachRebuildToIntAttachment(int idx)
{
    connectors::addGuiStep(*intAttachments[idx], [w = juce::Component::SafePointer(this)](auto &a) {
        if (w)
        {
            juce::Timer::callAfterDelay(0, [w]() {
                if (w)
                {
                    w->rebuildControlsFromDescription();
                }
            });
        }
    });
}

void ProcessorPane::attachRebuildToDeactivateAttachment(int idx)
{
    connectors::addGuiStep(*deactivateAttachments[idx],
                           [w = juce::Component::SafePointer(this)](auto &a) {
                               if (w)
                               {
                                   juce::Timer::callAfterDelay(0, [w]() {
                                       if (w)
                                       {
                                           w->rebuildControlsFromDescription();
                                       }
                                   });
                               }
                           });
}

void ProcessorPane::reapplyStyle()
{
    if (forZone)
    {
        editor->themeApplier.applyZoneMultiScreenTheme(this);
    }
    else
    {
        editor->themeApplier.applyGroupMultiScreenTheme(this);
    }
}
bool ProcessorPane::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (dragSourceDetails.sourceComponent)
    {
        auto pp = dynamic_cast<ProcessorPane *>(dragSourceDetails.sourceComponent.get());
        if (pp && pp != this)
            return true;
    }

    return false;
}

void ProcessorPane::itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    auto pp = dynamic_cast<ProcessorPane *>(dragSourceDetails.sourceComponent.get());
    if (!pp)
        return;

    if (forZone)
    {
        sendToSerialization(cmsg::SwapZoneProcessors({index, pp->index}));
    }
    else
    {
        sendToSerialization(cmsg::SwapGroupProcessors({index, pp->index}));
    }
    setIsAccented(false);
}

void ProcessorPane::mouseDrag(const juce::MouseEvent &e)
{
    if (!isDragging && e.getDistanceFromDragStart() > 2)
    {
        if (auto *container = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            container->startDragging("Processor", this);
            isDragging = true;
        }
    }
    sst::jucegui::components::NamedPanel::mouseDrag(e);
}

void ProcessorPane::mouseUp(const juce::MouseEvent &e)
{
    isDragging = false;
    sst::jucegui::components::NamedPanel::mouseUp(e);
}

void ProcessorPane::itemDragEnter(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    setIsAccented(true);
}
void ProcessorPane::itemDragExit(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    setIsAccented(false);
}

std::string ProcessorPane::resolveJsonPath(const std::string &path) const
{
    auto res = scxt::ui::connectors::jsonlayout::resolveJsonFile(path);
    if (res.has_value())
        return *res;
    editor->displayError("Unable to locate JSON asset", path);
    return {};
}

void ProcessorPane::layoutControlsWithJsonEngine(const std::string &jsonpath)
{
    getContentAreaComponent()->removeAllChildren();
    for (auto &f : jsonFloatEditors)
        f.reset();
    for (auto &f : jsonIntEditors)
        f.reset();
    for (auto &f : jsonDeactEditors)
        f.reset();
    jsonLabels.clear();

    auto eng = sst::jucegui::layouts::JsonLayoutEngine(*this);

    auto res = eng.processJsonPath(jsonpath);
    if (!res)
        editor->displayError("JSON Parsing Error", *res.error);
}

void ProcessorPane::createBindAndPosition(const sst::jucegui::layouts::json_document::Control &ctrl,
                                          const sst::jucegui::layouts::json_document::Class &cls)
{
    auto onError = [this, ctrl](const auto &s) {
        editor->displayError("JSON Error : " + ctrl.name, s);
    };

    auto intAttI = [this](int is) {
        if (intAttachments[is])
            return intAttachments[is]->getValue();
        SCLOG_IF(warnings, "Queried an unimplemented int attachment in intAttI");
        return 0;
    };

    if (ctrl.binding.has_value() && ctrl.binding->type == "float")
    {
        auto idx = ctrl.binding->index;
        if (idx < 0 || idx >= scxt::maxProcessorFloatParams)
        {
            onError("Index " + std::to_string(idx) + " is out of range on " + ctrl.name);
            return;
        }

        if (!floatAttachments[idx])
        {
            onError("Float attachment " + std::to_string(idx) + " is not initialized on " +
                    ctrl.name);
            return;
        }

        auto viz = scxt::ui::connectors::jsonlayout::isVisible(ctrl, intAttI, onError);
        if (!viz)
            return;

        jsonFloatEditor_t ed = connectors::jsonlayout::createContinuousWidget(
            ctrl, cls, processorControlDescription.floatControlDescriptions[idx]);
        if (!ed)
        {
            onError("Could not create widget for " + ctrl.name + " with unknown control type " +
                    cls.controlType);
            return;
        }

        auto &att = floatAttachments[idx];
        connectors::jsonlayout::attachAndPosition(this, ed, att, ctrl, cls);

        getContentAreaComponent()->addAndMakeVisible(*ed);

        bool en = true;
        if (ctrl.enabledIf.has_value())
        {
            en = scxt::ui::connectors::jsonlayout::isEnabled(ctrl, intAttI, onError);
        }

        if (deactivateAttachments[idx])
        {
            en = deactivateAttachments[idx]->getValue();
            auto pt = std::make_unique<jcmp::ToggleButton>();
            pt->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
            pt->setGlyph(jcmp::GlyphPainter::SMALL_POWER_LIGHT);
            pt->setBounds(ctrl.position.x + ctrl.position.w - 4, ctrl.position.y, 10, 10);
            pt->setSource(deactivateAttachments[idx].get());
            getContentAreaComponent()->addAndMakeVisible(*pt);
            jsonDeactEditors[idx] = std::move(pt);
            attachRebuildToDeactivateAttachment(idx);
        }

        ed->setEnabled(en);

        if (auto lab = connectors::jsonlayout::createControlLabel(ctrl, cls, *this))
        {
            getContentAreaComponent()->addAndMakeVisible(*lab);
            jsonLabels.push_back(std::move(lab));
        }

        jsonFloatEditors[idx] = std::move(ed);
    }
    else if (ctrl.binding.has_value() && ctrl.binding->type == "int")
    {
        auto idx = ctrl.binding->index;
        if (idx < 0 || idx >= scxt::maxProcessorIntParams)
        {
            onError("Index " + std::to_string(idx) + " is out of range on " + ctrl.name);
            return;
        }

        if (!intAttachments[idx])
        {
            onError("Int attachment " + std::to_string(idx) + " is not initialized on " +
                    ctrl.name);
            return;
        }

        auto viz = scxt::ui::connectors::jsonlayout::isVisible(ctrl, intAttI, onError);
        if (!viz)
            return;

        if (cls.controlType == "hamburger-stereo")
        {
            createHamburgerStereo(idx);
            return;
        }

        jsonIntEditor_t ed;
        if (cls.controlType == "waveshaper-menu")
        {
            auto jud = std::make_unique<jcmp::JogUpDownButton>();
            editor->configureHasDiscreteMenuBuilder(jud.get());
            jud->popupMenuBuilder->mode =
                sst::jucegui::components::DiscreteParamMenuBuilder::Mode::GROUP_LIST;
            jud->popupMenuBuilder->setGroupList(sst::waveshapers::WaveshaperGroupName());

            ed = std::move(jud);
        }
        else
        {
            ed = connectors::jsonlayout::createDiscreteWidget(ctrl, cls, onError);
        }

        if (!ed)
        {
            onError("Unable to create int editor for " + ctrl.name + " with unknown control type " +
                    cls.controlType);
            return;
        }

        auto en = scxt::ui::connectors::jsonlayout::isEnabled(ctrl, intAttI, onError);
        ed->setEnabled(en);

        getContentAreaComponent()->addAndMakeVisible(*ed);

        auto &att = intAttachments[idx];
        connectors::jsonlayout::attachAndPosition(this, ed, att, ctrl, cls);

        if (auto lab = connectors::jsonlayout::createControlLabel(ctrl, cls, *this))
        {
            getContentAreaComponent()->addAndMakeVisible(*lab);
            jsonLabels.push_back(std::move(lab));
        }

        jsonIntEditors[idx] = std::move(ed);
    }
    else if (auto nw = connectors::jsonlayout::createAndPositionNonDataWidget(ctrl, cls, onError))
    {
        getContentAreaComponent()->addAndMakeVisible(*nw);
        jsonLabels.push_back(std::move(nw));
    }
    else
    {
        onError("Unbound control " + ctrl.name);
    }
}

} // namespace scxt::ui::app::edit_screen
