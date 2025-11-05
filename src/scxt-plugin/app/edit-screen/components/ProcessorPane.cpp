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

#include "connectors/JsonLayoutEngineSupport.h"
#include "connectors/SCXTResources.h"

#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/RuledLabel.h"
#include "sst/jucegui/components/JogUpDownButton.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/layouts/ExplicitLayout.h"
#include "sst/jucegui/layouts/ListLayout.h"
#include "sst/waveshapers/WaveshaperConfiguration.h"
#include "sst/filters/FilterConfigurationLabels.h"
#include <sst/jucegui/components/MenuButton.h>

#include "theme/Layout.h"

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

    onHamburger = [safeThis = juce::Component::SafePointer(this)]() {
        if (safeThis)
            safeThis->showHamburgerMenu();
    };

    setTogglable(true);
}

ProcessorPane::~ProcessorPane() { resetControls(); }

void ProcessorPane::resized()
{
    jcmp::NamedPanel::resized();
    rebuildControlsFromDescription();
}

void ProcessorPane::showHamburgerMenu()
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

        if (pd.id == scxt::dsp::processor::proct_osc_VA)
            continue;

        if (pd.id == scxt::dsp::processor::proct_SurgeFilters)
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

void ProcessorPane::rebuildControlsFromDescription()
{
    resetControls();
    // TODO: Am I actually an off type? Then bail.

    if (!isEnabled())
    {
        setToggleDataSource(nullptr);
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
        multiButton->setOnCallback([this]() {
            sendToSerialization(cmsg::CopyProcessorLeadToAll({forZone, index}));
        });
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

    switch (processorControlDescription.type)
    {
    case dsp::processor::proct_SurgeFilters:
        layoutControlsSurgeFilters();
        break;
    case dsp::processor::proct_VemberClassic:
        layoutControlsWithJsonEngine("voicefx-layouts/filters/vember-classic.json");
        break;

    case dsp::processor::proct_K35:
        layoutControlsWithJsonEngine("voicefx-layouts/filters/k35.json");
        break;

    case dsp::processor::proct_obx4:
        layoutControlsWithJsonEngine("voicefx-layouts/filters/obxd-4pole.json");
        break;
    case dsp::processor::proct_diodeladder:
        layoutControlsWithJsonEngine("voicefx-layouts/filters/linear-ladder.json");
        break;

    case dsp::processor::proct_cutoffwarp:
    case dsp::processor::proct_reswarp:
        layoutControlsWithJsonEngine("voicefx-layouts/filters/warp.json");
        break;

    case dsp::processor::proct_vintageladder:
        layoutControlsWithJsonEngine("voicefx-layouts/filters/vintage-ladder.json");
        break;

    case dsp::processor::proct_tripole:
        layoutControlsWithJsonEngine("voicefx-layouts/filters/tripole.json");
        break;

    case dsp::processor::proct_snhfilter:
        layoutControlsWithJsonEngine("voicefx-layouts/filters/sample-and-hold.json");
        break;

    case dsp::processor::proct_fx_microgate:
        layoutControlsMicroGate();
        break;

    case dsp::processor::proct_stereotool:
        layoutControlsStereoTool();
        break;

    case dsp::processor::proct_gainmatrix:
        layoutControlsWithJsonEngine("voicefx-layouts/utility/gain-matrix.json");
        break;

    case dsp::processor::proct_CytomicSVF:
        layoutControlsWithJsonEngine("voicefx-layouts/filters/fast-svf.json");
        break;

    case dsp::processor::proct_eq_1band_parametric_A:
    case dsp::processor::proct_eq_2band_parametric_A:
    case dsp::processor::proct_eq_3band_parametric_A:
        layoutControlsEQNBandParm();
        break;

    case dsp::processor::proct_eq_tilt:
        layoutControlsWithJsonEngine("voicefx-layouts/eq/tilt-eq.json");
        break;

    case dsp::processor::proct_eq_morph:
        layoutControlsEQMorph();
        break;

    case dsp::processor::proct_eq_6band:
        layoutControlsEQGraphic();
        break;

    case dsp::processor::proct_osc_correlatednoise:
        layoutControlsCorrelatedNoiseGen();
        break;

    case dsp::processor::proct_osc_VA:
        layoutControlsVAOsc();
        break;

    case dsp::processor::proct_stringResonator:
        layoutControlsStringResonator();
        break;

    case dsp::processor::proct_StaticPhaser:
        layoutControlsStaticPhaser();
        break;

    case dsp::processor::proct_fmfilter:
        layoutControlsFMFilter();
        break;

    case dsp::processor::proct_Tremolo:
        layoutControlsWithJsonEngine("voicefx-layouts/modulation/tremolo.json");
        break;

    case dsp::processor::proct_Phaser:
        layoutControlsPhaser();
        break;

    case dsp::processor::proct_Chorus:
        layoutControlsChorus();
        break;

    case dsp::processor::proct_fx_ringmod:
        layoutControlsRingMod();
        break;

    case dsp::processor::proct_osc_EBWaveforms:
        layoutControlsEBWaveforms();
        break;

    case dsp::processor::proct_fx_freqshiftmod:
        layoutControlsWithJsonEngine("voicefx-layouts/audio-rate-mod/freq-shift-mod.json");
        break;

    case dsp::processor::proct_lifted_delay:
        layoutControlsWithJsonEngine("voicefx-layouts/delay/lifted-delay.json");
        break;

    case dsp::processor::proct_fx_bitcrusher:
        layoutControlsWithJsonEngine("voicefx-layouts/distortion/bitcrusher.json");
        break;

    case dsp::processor::proct_noise_am:
        layoutControlsWithJsonEngine("voicefx-layouts/audio-rate-mod/noise-am.json");
        break;

    case dsp::processor::proct_osc_phasemod:
        layoutControlsWithJsonEngine("voicefx-layouts/audio-rate-mod/phase-mod.json");
        break;

    case dsp::processor::proct_shepard:
        layoutControlsWithJsonEngine("voicefx-layouts/modulation/shepard.json");
        break;

    case dsp::processor::proct_fx_simple_delay:
        layoutControlsWithJsonEngine("voicefx-layouts/delay/simple-delay.json");
        break;

    case dsp::processor::proct_fx_waveshaper:
        // layoutControlsWaveshaper();
        layoutControlsWithJsonEngine("voicefx-layouts/distortion/waveshaper.json");
        break;

    case dsp::processor::proct_utilfilt:
        layoutControlsWithJsonEngine("voicefx-layouts/filters/utility-filter.json");
        break;

    default:
        layoutControls();
        break;
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
        addAdditionalHamburgerComponent(std::move(ts));
    }

    setToggleDataSource(nullptr);
    bypassAttachment = std::make_unique<bool_attachment_t>("Bypass", processorView.isActive);
    connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorBoolValue, bool_attachment_t,
                                 bool_attachment_t::onGui_t>(*bypassAttachment, processorView, this,
                                                             forZone, index);
    setToggleDataSource(bypassAttachment.get());
    connectors::addGuiStep(*bypassAttachment,
                           [this](auto &a) { editor->processorBypassToggled(index); });

    reapplyStyle();

    repaint();
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
            sst::jucegui::components::Labeled<sst::jucegui::components::DiscreteParamEditor>>();
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

// We broke this up, but let's keep it around for a while
// so folks have a chance to transfer settings.
void ProcessorPane::layoutControlsSurgeFilters()
{
    // FIXME
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    auto cols = lo::columns(getContentAreaComponent()->getLocalBounds(), 2);
    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    lo::knobCX<locon::largeKnob>(*floatEditors[0], cols[0].getCentreX(), 5);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    lo::knobCX<locon::largeKnob>(*floatEditors[1], cols[1].getCentreX(), 5);

    auto col0 = cols[0]
                    .withTrimmedTop(floatEditors[0]->label->getBottom() + 15)
                    .withHeight(18)
                    .withWidth(cols[0].getWidth() + 15);

    for (int i = 0; i < 2; ++i)
    {
        auto ms = createWidgetAttachedTo<jcmp::JogUpDownButton>(intAttachments[i]);
        ms->setBounds(col0);
        col0 = col0.translated(0, 25);

        if (i == 0)
        {
            editor->configureHasDiscreteMenuBuilder(ms.get());

            ms->popupMenuBuilder->mode =
                sst::jucegui::components::DiscreteParamMenuBuilder::Mode::GROUP_LIST;
            ms->popupMenuBuilder->setGroupList(sst::filters::filterGroupName());
        }

        intEditors[i] = std::make_unique<intEditor_t>(std::move(ms));
        if (intAttachments[i]->getMin() == intAttachments[i]->getMax())
        {
            intEditors[i]->item->setEnabled(false);
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

void ProcessorPane::layoutControlsCorrelatedNoiseGen()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    createHamburgerStereo(0);
    bool stereoSwitch = intAttachments[0]->getValue();

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    if (stereoSwitch)
    {
        lo::knob<50>(*floatEditors[0], 20, 10);
        lo::knob<50>(*floatEditors[1], 120, 10);
    }
    else
    {
        lo::knob<70>(*floatEditors[0], 10, 20);
        lo::knob<70>(*floatEditors[1], 110, 20);
    }

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    lo::knob<50>(*floatEditors[2], 70, 75);
    floatEditors[2]->setVisible(intAttachments[0]->getValue());
}

void ProcessorPane::layoutControlsStringResonator()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    createHamburgerStereo(0);

    auto bounds = getContentAreaComponent()->getLocalBounds();
    auto dual = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[1]);
    dual->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH);
    dual->setGlyph(jcmp::GlyphPainter::SMALL_POWER_LIGHT);
    auto dualBounds = bounds.withLeft(1).withRight(11).withTop(63).withBottom(74);
    dual->setBounds(dualBounds);
    intEditors[1] = std::make_unique<intEditor_t>(std::move(dual));
    attachRebuildToIntAttachment(1);

    bool stereoSwitch = intAttachments[0]->getValue();
    bool dualSwitch = intAttachments[1]->getValue();

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    floatEditors[4] = createWidgetAttachedTo(floatAttachments[4], floatAttachments[4]->getLabel());
    floatEditors[5] = createWidgetAttachedTo(floatAttachments[5], floatAttachments[5]->getLabel());

    floatEditors[4]->setVisible(stereoSwitch);
    floatEditors[5]->setVisible(stereoSwitch);

    if (stereoSwitch)
    {
        lo::knob<40>(*floatEditors[0], 5, 10);
        lo::knob<40>(*floatEditors[1], 5, 70);
        lo::knob<40>(*floatEditors[2], 50, 10);
        lo::knob<40>(*floatEditors[3], 50, 70);
        lo::knob<40>(*floatEditors[4], 95, 10);
        lo::knob<40>(*floatEditors[5], 95, 70);
    }
    else
    {
        lo::knob<40>(*floatEditors[0], 5, 10);
        lo::knob<40>(*floatEditors[1], 5, 70);
        lo::knob<40>(*floatEditors[2], 72, 10);
        lo::knob<40>(*floatEditors[3], 72, 70);
    }

    floatEditors[1]->item->setEnabled(dualSwitch);
    floatEditors[3]->item->setEnabled(dualSwitch);
    floatEditors[5]->item->setEnabled(dualSwitch);

    floatEditors[6] = createWidgetAttachedTo(floatAttachments[6], floatAttachments[6]->getLabel());
    lo::knob<40>(*floatEditors[6], 140, 10);

    floatEditors[7] = createWidgetAttachedTo(floatAttachments[7], floatAttachments[7]->getLabel());
    lo::knob<40>(*floatEditors[7], 140, 70);
}

void ProcessorPane::layoutControlsStaticPhaser()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    createHamburgerStereo(1);
    bool stereoSwitch = intAttachments[1]->getValue();

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    if (stereoSwitch == true)
    {
        lo::knob<45>(*floatEditors[0], 5, 10);
    }
    else
    {

        lo::knob<45>(*floatEditors[0], 5, 43);
    }

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    lo::knob<45>(*floatEditors[1], 5, 85);

    floatEditors[1]->setVisible(stereoSwitch);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    lo::knob<45>(*floatEditors[3], 65, 10);

    floatEditors[4] = createWidgetAttachedTo(floatAttachments[4], floatAttachments[4]->getLabel());
    lo::knob<45>(*floatEditors[4], 125, 10);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    lo::knob<45>(*floatEditors[2], 65, 85);

    auto bounds = getContentAreaComponent()->getLocalBounds();

    auto stageSwitch = createWidgetAttachedTo<jcmp::MultiSwitch>(intAttachments[0]);
    auto switchBounds = bounds.withLeft(125).withTop(78).withRight(180).withBottom(145);
    stageSwitch->setBounds(switchBounds);
    intEditors[0] = std::make_unique<intEditor_t>(std::move(stageSwitch));
}

void ProcessorPane::layoutControlsPhaser()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    createHamburgerStereo(1);

    auto bounds = getContentAreaComponent()->getLocalBounds();
    auto shapeSwitch = createWidgetAttachedTo<jcmp::MultiSwitch>(intAttachments[0]);
    auto switchBounds = bounds.withLeft(130).withRight(185).withTop(75).withBottom(145);
    shapeSwitch->setBounds(switchBounds);
    intEditors[0] = std::make_unique<intEditor_t>(std::move(shapeSwitch));

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    lo::knob<40>(*floatEditors[0], 5, 0);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    lo::knob<40>(*floatEditors[2], 50, 20);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    lo::knob<40>(*floatEditors[1], 95, 0);

    floatEditors[5] = createWidgetAttachedTo(floatAttachments[5], floatAttachments[5]->getLabel());
    lo::knob<40>(*floatEditors[5], 140, 20);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    lo::knob<50>(*floatEditors[3], 5, 80);

    floatEditors[4] = createWidgetAttachedTo(floatAttachments[4], floatAttachments[4]->getLabel());
    lo::knob<50>(*floatEditors[4], 65, 80);
}

void ProcessorPane::layoutControlsMicroGate()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    lo::knob<55>(*floatEditors[0], 25, 0);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    lo::knob<55>(*floatEditors[1], 105, 0);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    lo::knob<55>(*floatEditors[2], 25, 75);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    lo::knob<55>(*floatEditors[3], 105, 75);
}

void ProcessorPane::layoutControlsStereoTool()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    lo::knob<55>(*floatEditors[0], 25, 0);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    lo::knob<55>(*floatEditors[1], 105, 0);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    lo::knob<55>(*floatEditors[2], 25, 75);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    lo::knob<55>(*floatEditors[3], 105, 75);
}

void ProcessorPane::layoutControlsVAOsc()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    auto bounds = getContentAreaComponent()->getLocalBounds();
    auto wave = createWidgetAttachedTo<jcmp::MultiSwitch>(intAttachments[0]);
    auto switchBounds = bounds.withLeft(130).withRight(185).withTop(75).withBottom(145);
    wave->setBounds(switchBounds);
    intEditors[0] = std::make_unique<intEditor_t>(std::move(wave));
    attachRebuildToIntAttachment(0);

    int waveSwitch = intAttachments[0]->getValue();

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    lo::knob<50>(*floatEditors[0], 35, 5);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    lo::knob<50>(*floatEditors[1], 100, 5);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    lo::knob<50>(*floatEditors[2], 5, 80);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    lo::knob<50>(*floatEditors[3], 65, 80);

    floatEditors[4] = createWidgetAttachedTo(floatAttachments[4], floatAttachments[4]->getLabel());
    lo::knob<50>(*floatEditors[4], 5, 80);

    floatEditors[5] = createWidgetAttachedTo(floatAttachments[5], floatAttachments[5]->getLabel());
    lo::knob<50>(*floatEditors[5], 65, 80);

    floatEditors[2]->setVisible(false);
    floatEditors[3]->setVisible(false);
    floatEditors[4]->setVisible(false);
    floatEditors[5]->setVisible(false);

    if (waveSwitch == 2)
    {
        floatEditors[2]->setVisible(true);
        floatEditors[3]->setVisible(true);
    }
    else if (waveSwitch == 1)
    {
        floatEditors[4]->setVisible(true);
        floatEditors[5]->setVisible(true);
    }
}

void ProcessorPane::layoutControlsChorus()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    createHamburgerStereo(1);

    auto bounds = getContentAreaComponent()->getLocalBounds();
    auto shapeSwitch = createWidgetAttachedTo<jcmp::MultiSwitch>(intAttachments[0]);
    auto switchBounds = bounds.withLeft(130).withRight(185).withTop(75).withBottom(145);
    shapeSwitch->setBounds(switchBounds);
    intEditors[0] = std::make_unique<intEditor_t>(std::move(shapeSwitch));

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    lo::knob<50>(*floatEditors[0], 35, 5);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    lo::knob<50>(*floatEditors[1], 100, 5);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    lo::knob<50>(*floatEditors[2], 5, 80);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    lo::knob<50>(*floatEditors[3], 65, 80);
}

void ProcessorPane::layoutControlsFMFilter()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    createHamburgerStereo(0);

    bool stereoSwitch = intAttachments[0]->getValue();

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    if (stereoSwitch == true)
    {
        lo::knob<42>(*floatEditors[0], 5, 5);
    }
    else
    {
        lo::knob<42>(*floatEditors[0], 5, 40);
    }

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    lo::knob<42>(*floatEditors[1], 33, 65);

    floatEditors[1]->setVisible(stereoSwitch);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    lo::knob<42>(*floatEditors[2], 68, 5);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    lo::knob<42>(*floatEditors[3], 98, 65);

    auto bounds = getContentAreaComponent()->getLocalBounds();

    auto numBounds = bounds.withTop(5).withBottom(27).withLeft(130).withRight(180);
    auto num = createWidgetAttachedTo<jcmp::JogUpDownButton>(intAttachments[2]);
    num->setBounds(numBounds);
    intEditors[2] = std::make_unique<intEditor_t>(std::move(num));

    auto denomBounds = bounds.withTop(32).withBottom(54).withLeft(130).withRight(180);
    auto denom = createWidgetAttachedTo<jcmp::JogUpDownButton>(intAttachments[3]);
    denom->setBounds(denomBounds);
    intEditors[3] = std::make_unique<intEditor_t>(std::move(denom));

    auto modeBounds = bounds.withTop(bounds.getBottom() - 22).translated(0, -3).reduced(3, 0);
    auto filterMode = createWidgetAttachedTo<jcmp::JogUpDownButton>(intAttachments[1]);
    filterMode->setBounds(modeBounds);
    intEditors[1] = std::make_unique<intEditor_t>(std::move(filterMode));
}

void ProcessorPane::layoutControlsRingMod()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    lo::knob<locon::extraLargeKnob>(*floatEditors[0], 30, 15);

    auto bounds = getContentAreaComponent()->getLocalBounds();

    auto numBounds = bounds.withTop(35).withBottom(57).withLeft(130).withRight(180);
    auto num = createWidgetAttachedTo<jcmp::JogUpDownButton>(intAttachments[0]);
    num->setBounds(numBounds);
    intEditors[0] = std::make_unique<intEditor_t>(std::move(num));

    auto denomBounds = bounds.withTop(62).withBottom(84).withLeft(130).withRight(180);
    auto denom = createWidgetAttachedTo<jcmp::JogUpDownButton>(intAttachments[1]);
    denom->setBounds(denomBounds);
    intEditors[1] = std::make_unique<intEditor_t>(std::move(denom));
}

void ProcessorPane::createHamburgerStereo(int attachmentId)
{
    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[attachmentId]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
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
        SCLOG("Warning: Queried an unimplemented int attachment in intAttI");
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

        jsonFloatEditor_t ed = connectors::jsonlayout::createContinuousWidget(ctrl, cls);
        if (!ed)
        {
            onError("Could not create widget for " + ctrl.name + " with unknown control type " +
                    cls.controlType);
            return;
        }

        auto &att = floatAttachments[idx];
        connectors::jsonlayout::attachAndPosition(this, ed, att, ctrl);

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
        connectors::jsonlayout::attachAndPosition(this, ed, att, ctrl);

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
