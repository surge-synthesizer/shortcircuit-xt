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

#include "ProcessorPane.h"
#include "components/SCXTEditor.h"

#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/JogUpDownButton.h"
#include "sst/jucegui/layouts/ExplicitLayout.h"
#include "sst/waveshapers/WaveshaperConfiguration.h"
#include "sst/filters/FilterConfigurationLabels.h"
#include <sst/jucegui/components/MenuButton.h>

#include "theme/Layout.h"

namespace scxt::ui::multi
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

ProcessorPane::ProcessorPane(SCXTEditor *e, int index, bool fz)
    : HasEditor(e), jcmp::NamedPanel("PROCESSOR " + std::to_string(index + 1)), index(index),
      forZone(fz)
{
    setContentAreaComponent(std::make_unique<juce::Component>());
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
        multiButton->setOnCallback(
            [this]() { sendToSerialization(cmsg::CopyProcessorLeadToAll(index)); });
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

    case dsp::processor::proct_fx_microgate:
        layoutControlsMicroGate();
        break;

    case dsp::processor::proct_fx_simple_delay:
        layoutControlsSimpleDelay();
        break;

    case dsp::processor::proct_CytomicSVF:
        layoutControlsFastSVF();
        break;

    case dsp::processor::proct_eq_1band_parametric_A:
    case dsp::processor::proct_eq_2band_parametric_A:
    case dsp::processor::proct_eq_3band_parametric_A:
        layoutControlsEQNBandParm();
        break;

    case dsp::processor::proct_fx_waveshaper:
        layoutControlsWaveshaper();
        break;

    case dsp::processor::proct_fx_bitcrusher:
        layoutControlsBitcrusher();
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
        LayoutControlsTremolo();
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

    case dsp::processor::proct_osc_phasemod:
        layoutControlsPhaseMod();
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

void ProcessorPane::layoutControlsSimpleDelay()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[0]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));
    attachRebuildToIntAttachment(0);
    bool stereoSwitch = intAttachments[0]->getValue();

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    floatEditors[4] = createWidgetAttachedTo(floatAttachments[4], floatAttachments[4]->getLabel());
    floatEditors[5] = createWidgetAttachedTo(floatAttachments[5], floatAttachments[5]->getLabel());

    if (stereoSwitch)
    {
        lo::knob<45>(*floatEditors[0], 5, 5);
        lo::knob<45>(*floatEditors[1], 70, 5);
        floatEditors[3]->item->setEnabled(true);
    }
    else
    {
        lo::knob<45>(*floatEditors[0], 38, 5);
        floatEditors[3]->item->setEnabled(false);
    }

    lo::knob<45>(*floatEditors[2], 140, 5);
    lo::knob<45>(*floatEditors[3], 140, 85);
    lo::knob<45>(*floatEditors[4], 5, 85);
    lo::knob<45>(*floatEditors[5], 70, 85);
}

// May want to break this up
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

void ProcessorPane::layoutControlsFastSVF()
{
    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[1]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));
    attachRebuildToIntAttachment(1);
    bool stereoSwitch = intAttachments[1]->getValue();

    namespace lo = theme::layout;
    namespace locon = lo::constants;

    auto bd = getContentArea();
    auto rest = bd.withTrimmedLeft(60).withHeight(55);

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());

    if (stereoSwitch)
    {
        lo::knob<locon::largeKnob>(*floatEditors[0], 5, 5);
        lo::knob<locon::largeKnob>(*floatEditors[1], 70, 45);
    }
    else
    {
        lo::knob<locon::extraLargeKnob>(*floatEditors[0], 35, 15);
    }

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    lo::knob<locon::mediumKnob>(*floatEditors[2], 135, 5);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    lo::knob<locon::mediumKnob>(*floatEditors[3], 135, 60);
    floatEditors[3]->item->setEnabled(false);

    auto bounds = getContentAreaComponent()->getLocalBounds();

    auto slopeSwitch = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[2]);
    slopeSwitch->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH);
    slopeSwitch->setGlyph(jcmp::GlyphPainter::POWER_LIGHT);
    auto slopeBounds = bounds.withLeft(175).withRight(185).withTop(5).withBottom(15);
    slopeSwitch->setBounds(slopeBounds);
    intEditors[2] = std::make_unique<intEditor_t>(std::move(slopeSwitch));

    bounds = bounds.withTop(bounds.getBottom() - 22).translated(0, -3).reduced(3, 0);

    auto filterMode = createWidgetAttachedTo<jcmp::JogUpDownButton>(intAttachments[0]);
    filterMode->setBounds(bounds);
    intEditors[0] = std::make_unique<intEditor_t>(std::move(filterMode));
    attachRebuildToIntAttachment(0);
    auto modeSwitch = intAttachments[0]->getValue();

    if (modeSwitch > 5)
    {
        floatEditors[3]->item->setEnabled(true);
    }
}

void ProcessorPane::layoutControlsWaveshaper()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    lo::knob<locon::extraLargeKnob>(*floatEditors[0], 5, 15);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    lo::knob<locon::mediumKnob>(*floatEditors[1], 87, 5);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    lo::knob<locon::mediumKnob>(*floatEditors[2], 87, 65);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    lo::knob<locon::mediumKnob>(*floatEditors[3], 140, 5);

    floatEditors[4] = createWidgetAttachedTo(floatAttachments[4], floatAttachments[4]->getLabel());
    lo::knob<locon::mediumKnob>(*floatEditors[4], 140, 65);

    auto bounds = getContentAreaComponent()->getLocalBounds();

    auto hpLight = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[1]);
    hpLight->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH);
    hpLight->setGlyph(jcmp::GlyphPainter::POWER_LIGHT);
    auto hpBounds = bounds.withLeft(175).withRight(185).withTop(0).withBottom(10);
    hpLight->setBounds(hpBounds);
    intEditors[1] = std::make_unique<intEditor_t>(std::move(hpLight));

    attachRebuildToIntAttachment(1);
    bool hpOn = intAttachments[1]->getValue();
    floatEditors[3]->item->setEnabled(hpOn);

    auto lpLight = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[2]);
    lpLight->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH);
    lpLight->setGlyph(jcmp::GlyphPainter::POWER_LIGHT);
    auto lpBounds = bounds.withLeft(175).withRight(185).withTop(60).withBottom(70);
    lpLight->setBounds(lpBounds);
    intEditors[2] = std::make_unique<intEditor_t>(std::move(lpLight));

    attachRebuildToIntAttachment(2);
    bool lpOn = intAttachments[2]->getValue();
    floatEditors[4]->item->setEnabled(lpOn);

    auto ja = getContentAreaComponent()->getLocalBounds();
    ja = ja.withTop(ja.getBottom() - 22).translated(0, -3).reduced(3, 0);

    auto wss = createWidgetAttachedTo<jcmp::JogUpDownButton>(intAttachments[0]);
    editor->configureHasDiscreteMenuBuilder(wss.get());
    wss->popupMenuBuilder->mode =
        sst::jucegui::components::DiscreteParamMenuBuilder::Mode::GROUP_LIST;
    wss->popupMenuBuilder->setGroupList(sst::waveshapers::WaveshaperGroupName());
    wss->setBounds(ja);
    intEditors[0] = std::make_unique<intEditor_t>(std::move(wss));
}

void ProcessorPane::layoutControlsBitcrusher()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;
    namespace jlay = sst::jucegui::layout;
    using np = jlay::ExplicitLayout::NamedPosition;

    auto bounds = getContentAreaComponent()->getLocalBounds();
    auto elo = jlay::ExplicitLayout();
    elo.addNamedPositionAndLabel(np("sr").at({10, 5, 50, 50}));
    elo.addNamedPositionAndLabel(np("bd").at({70, 5, 50, 50}));
    elo.addNamedPositionAndLabel(np("zp").at({130, 5, 50, 50}));
    elo.addNamedPositionAndLabel(np("co").at({40, 80, 50, 50}));
    elo.addNamedPositionAndLabel(np("re").at({100, 80, 50, 50}));
    elo.addPowerButtonPositionTo("re", 8);

    bool filterSwitch = intAttachments[0]->getValue();

    floatEditors[0] = createAndLayoutLabeledFloatWidget(elo, "sr", floatAttachments[0]);
    floatEditors[1] = createAndLayoutLabeledFloatWidget(elo, "bd", floatAttachments[1]);
    floatEditors[2] = createAndLayoutLabeledFloatWidget(elo, "zp", floatAttachments[2]);
    floatEditors[3] = createAndLayoutLabeledFloatWidget(elo, "co", floatAttachments[3]);
    floatEditors[4] = createAndLayoutLabeledFloatWidget(elo, "re", floatAttachments[4]);

    floatEditors[3]->item->setEnabled(filterSwitch);
    floatEditors[4]->item->setEnabled(filterSwitch);

    intEditors[0] = createAndLayoutPowerButton(elo, "re", intAttachments[0]);
    attachRebuildToIntAttachment(0);
}

void ProcessorPane::layoutControlsCorrelatedNoiseGen()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    // Rather than std::move to intEditors[0] we hand the toggle button to the
    // main pane as an additional hamburger component
    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[0]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));
    attachRebuildToIntAttachment(0);
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

    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[0]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));
    attachRebuildToIntAttachment(0);

    auto bounds = getContentAreaComponent()->getLocalBounds();
    auto dual = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[1]);
    dual->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH);
    dual->setGlyph(jcmp::GlyphPainter::POWER_LIGHT);
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

    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[1]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));
    attachRebuildToIntAttachment(1);

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

void ProcessorPane::LayoutControlsTremolo()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[2]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));

    auto bounds = getContentAreaComponent()->getLocalBounds();
    auto shapeSwitch = createWidgetAttachedTo<jcmp::MultiSwitch>(intAttachments[1]);
    auto switchBounds = bounds.withLeft(130).withRight(185).withTop(75).withBottom(145);
    shapeSwitch->setBounds(switchBounds);
    intEditors[1] = std::make_unique<intEditor_t>(std::move(shapeSwitch));

    auto harmonic = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[0]);
    harmonic->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH);
    harmonic->setGlyph(jcmp::GlyphPainter::POWER_LIGHT);
    auto plusBounds = bounds.withLeft(148).withRight(160).withTop(0).withBottom(12);
    harmonic->setBounds(plusBounds);
    intEditors[0] = std::make_unique<intEditor_t>(std::move(harmonic));
    attachRebuildToIntAttachment(0);

    bool harmonicSwitch = intAttachments[0]->getValue();

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    lo::knob<50>(*floatEditors[0], 35, 5);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], floatAttachments[3]->getLabel());
    lo::knob<50>(*floatEditors[3], 100, 5);
    floatEditors[3]->item->setEnabled(harmonicSwitch);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    lo::knob<50>(*floatEditors[1], 5, 80);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], floatAttachments[2]->getLabel());
    lo::knob<50>(*floatEditors[2], 65, 80);
}

void ProcessorPane::layoutControlsPhaser()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[1]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));

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

    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[1]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));

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

    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[0]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));
    attachRebuildToIntAttachment(0);

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

void ProcessorPane::layoutControlsPhaseMod()
{
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], floatAttachments[0]->getLabel());
    lo::knob<locon::largeKnob>(*floatEditors[0], 5, 5);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], floatAttachments[1]->getLabel());
    lo::knob<locon::largeKnob>(*floatEditors[1], 60, 60);

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
}

void ProcessorPane::mouseDrag(const juce::MouseEvent &e)
{
    if (!isDragging && e.getDistanceFromDragStart() > 1.5f)
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

void ProcessorPane::setAsMultiZone(int32_t primaryType, const std::string &nm,
                                   const std::set<int32_t> &otherTypes)
{
    multiZone = true;
    multiName = nm;
    setEnabled(true);
    setName("Multiple");
    resetControls();
    setToggleDataSource(nullptr);
    resized();
    repaint();
}

} // namespace scxt::ui::multi
