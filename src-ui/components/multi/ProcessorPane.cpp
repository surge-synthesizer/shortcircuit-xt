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

    switch (processorControlDescription.type)
    {
    case dsp::processor::proct_SurgeFilters:
        layoutControlsSurgeFilters();
        break;

    case dsp::processor::proct_fx_microgate:
        layoutControlsMicroGate();
        break;

    case dsp::processor::proct_CytomicSVF:
    case dsp::processor::proct_SurgeBiquads:
        layoutControlsCytomicSVFAndBiquads();
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

    case dsp::processor::proct_stringResonator:
        layoutControlsStringResonator();
        break;

    case dsp::processor::proct_StaticPhaser:
        layoutControlsStaticPhaser();
        break;

    case dsp::processor::proct_Tremolo:
        LayoutControlsTremolo();
        break;

    case dsp::processor::proct_Phaser:
        layoutControlsPhaser();
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
    {
        clearAdditionalHamburgerComponents();
        mixEditor = createWidgetAttachedTo<jcmp::Knob>(mixAttachment, "Mix");
        addAdditionalHamburgerComponent(std::move(mixEditor->item));
    }

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

// May want to break this up
void ProcessorPane::layoutControlsSurgeFilters()
{
    // OK so we know we have 2 controls (cutoff and resonance), a mix, and two ints
    assert(processorControlDescription.numFloatParams == 2);
    assert(processorControlDescription.numIntParams == 2);

    // FIXME
    namespace lo = theme::layout;
    namespace locon = lo::constants;

    clearAdditionalHamburgerComponents();
    mixEditor = createWidgetAttachedTo<jcmp::Knob>(mixAttachment, "Mix");
    addAdditionalHamburgerComponent(std::move(mixEditor->item));

    auto cols = lo::columns(getContentAreaComponent()->getLocalBounds(), 2);
    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], "Cutoff");
    lo::knobCX<locon::largeKnob>(*floatEditors[0], cols[0].getCentreX(), 5);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], "Resonance");
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

void ProcessorPane::layoutControlsWaveshaper()
{
    // Drive/Bias/Gain in the floats
    // Type/OS in the ints
    assert(processorControlDescription.numFloatParams == 3);
    assert(processorControlDescription.numIntParams == 1);

    namespace lo = theme::layout;
    namespace locon = lo::constants;

    clearAdditionalHamburgerComponents();
    mixEditor = createWidgetAttachedTo<jcmp::Knob>(mixAttachment, "Mix");
    addAdditionalHamburgerComponent(std::move(mixEditor->item));

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], "Drive");
    lo::knob<locon::extraLargeKnob>(*floatEditors[0], 5, 10);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], "Bias");
    auto biasPos = lo::knob<locon::mediumKnob>(*floatEditors[1], 95, 0);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], "Gain");
    lo::labeledAt(*floatEditors[2], lo::belowLabel(biasPos));

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

    clearAdditionalHamburgerComponents();
    mixEditor = createWidgetAttachedTo<jcmp::Knob>(mixAttachment, "Mix");
    addAdditionalHamburgerComponent(std::move(mixEditor->item));

    auto bounds = getContentAreaComponent()->getLocalBounds();
    auto filt = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[0]);
    filt->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH);
    filt->setGlyph(jcmp::GlyphPainter::BIG_PLUS);
    auto plusBounds = bounds.withLeft(148).withRight(160).withTop(75).withBottom(87);
    filt->setBounds(plusBounds);
    intEditors[0] = std::make_unique<intEditor_t>(std::move(filt));
    attachRebuildToIntAttachment(0);

    bool filterSwitch = intAttachments[0]->getValue();

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], "Samplerate");
    lo::knob<50>(*floatEditors[0], 10, 5);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], "Bit Depth");
    lo::knob<50>(*floatEditors[1], 70, 5);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], "Zero Point");
    lo::knob<50>(*floatEditors[2], 130, 5);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], "Cutoff");
    lo::knob<50>(*floatEditors[3], 40, 80);
    floatEditors[3]->item->setEnabled(filterSwitch);

    floatEditors[4] = createWidgetAttachedTo(floatAttachments[4], "Resonance");
    lo::knob<50>(*floatEditors[4], 100, 80);
    floatEditors[4]->item->setEnabled(filterSwitch);
}

void ProcessorPane::layoutControlsCorrelatedNoiseGen()
{
    assert(processorControlDescription.numFloatParams == 3);
    assert(processorControlDescription.numIntParams == 1);

    namespace lo = theme::layout;
    namespace locon = lo::constants;

    clearAdditionalHamburgerComponents();
    mixEditor = createWidgetAttachedTo<jcmp::Knob>(mixAttachment, "Mix");
    addAdditionalHamburgerComponent(std::move(mixEditor->item));

    // Rather than std::move to intEditors[0] we hand the toggle button to the
    // main pane as an additional hamburger component
    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[0]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));
    attachRebuildToIntAttachment(0);
    bool stereoSwitch = intAttachments[0]->getValue();

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], "Color");
    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], "Level");
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

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], "Width");
    lo::knob<50>(*floatEditors[2], 70, 75);
    floatEditors[2]->setVisible(intAttachments[0]->getValue());
}

void ProcessorPane::layoutControlsStringResonator()
{
    assert(processorControlDescription.numFloatParams == 8);
    assert(processorControlDescription.numIntParams == 2);

    namespace lo = theme::layout;
    namespace locon = lo::constants;

    clearAdditionalHamburgerComponents();
    mixEditor = createWidgetAttachedTo<jcmp::Knob>(mixAttachment, "Mix");
    addAdditionalHamburgerComponent(std::move(mixEditor->item));

    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[0]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));
    attachRebuildToIntAttachment(0);

    auto dual = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[1]);
    dual->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    dual->setGlyph(jcmp::GlyphPainter::STEREO);
    dual->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(dual));
    attachRebuildToIntAttachment(1);

    bool stereoSwitch = intAttachments[0]->getValue();
    bool dualSwitch = intAttachments[1]->getValue();

    std::string justLevel = "Vol";
    std::string justTune = "Tune";
    std::string justPan = "Pan";
    if (dualSwitch)
    {
        justLevel += " 1";
        justTune += " 1";
        justPan += " 1";
    }

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], justLevel);
    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], "Vol 2");
    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], justTune);
    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], "Tune 2");
    floatEditors[4] = createWidgetAttachedTo(floatAttachments[4], justPan);
    floatEditors[5] = createWidgetAttachedTo(floatAttachments[5], "Pan 2");

    floatEditors[1]->setVisible(dualSwitch);
    floatEditors[3]->setVisible(dualSwitch);
    floatEditors[4]->setVisible(stereoSwitch);
    floatEditors[5]->setVisible(stereoSwitch && dualSwitch);

    if (dualSwitch && stereoSwitch)
    {

        lo::knob<40>(*floatEditors[0], 5, 10);
        lo::knob<40>(*floatEditors[1], 5, 70);
        lo::knob<40>(*floatEditors[2], 50, 10);
        lo::knob<40>(*floatEditors[3], 50, 70);
        lo::knob<40>(*floatEditors[4], 95, 10);
        lo::knob<40>(*floatEditors[5], 95, 70);
    }
    else if (dualSwitch && !stereoSwitch)
    {
        lo::knob<40>(*floatEditors[0], 5, 10);
        lo::knob<40>(*floatEditors[1], 5, 70);
        lo::knob<40>(*floatEditors[2], 72, 10);
        lo::knob<40>(*floatEditors[3], 72, 70);
    }
    else if (!dualSwitch && stereoSwitch)
    {
        lo::knob<40>(*floatEditors[0], 5, 40);
        lo::knob<40>(*floatEditors[2], 50, 40);
        lo::knob<40>(*floatEditors[4], 95, 40);
    }
    else
    {
        lo::knob<55>(*floatEditors[0], 5, 35);
        lo::knob<55>(*floatEditors[2], 72, 35);
    }

    floatEditors[6] = createWidgetAttachedTo(floatAttachments[6], "Decay");
    lo::knob<40>(*floatEditors[6], 140, 10);

    floatEditors[7] = createWidgetAttachedTo(floatAttachments[7], "Stiffness");
    lo::knob<40>(*floatEditors[7], 140, 70);
}

void ProcessorPane::layoutControlsStaticPhaser()
{
    assert(processorControlDescription.numFloatParams == 5);
    assert(processorControlDescription.numIntParams == 2);

    namespace lo = theme::layout;
    namespace locon = lo::constants;

    clearAdditionalHamburgerComponents();
    mixEditor = createWidgetAttachedTo<jcmp::Knob>(mixAttachment, "Mix");
    addAdditionalHamburgerComponent(std::move(mixEditor->item));

    auto stereo = createWidgetAttachedTo<jcmp::ToggleButton>(intAttachments[1]);
    stereo->setDrawMode(jcmp::ToggleButton::DrawMode::DUAL_GLYPH);
    stereo->setGlyph(jcmp::GlyphPainter::STEREO);
    stereo->setOffGlyph(jcmp::GlyphPainter::MONO);
    addAdditionalHamburgerComponent(std::move(stereo));
    attachRebuildToIntAttachment(1);

    bool stereoSwitch = intAttachments[1]->getValue();

    std::string justFreq = "Freq";
    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], justFreq);
    if (stereoSwitch == true)
    {
        justFreq += " L";
        lo::knob<45>(*floatEditors[0], 5, 10);
    }
    else
    {
        lo::knob<45>(*floatEditors[0], 5, 43);
    }

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], "Freq R");
    lo::knob<45>(*floatEditors[1], 5, 85);

    floatEditors[1]->setVisible(stereoSwitch);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], "Resonance");
    lo::knob<45>(*floatEditors[3], 65, 10);

    floatEditors[4] = createWidgetAttachedTo(floatAttachments[4], "Feedback");
    lo::knob<45>(*floatEditors[4], 125, 10);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], "Spacing");
    lo::knob<45>(*floatEditors[2], 65, 85);

    auto bounds = getContentAreaComponent()->getLocalBounds();

    auto stageSwitch = createWidgetAttachedTo<jcmp::MultiSwitch>(intAttachments[0]);
    auto switchBounds = bounds.withLeft(125).withTop(78).withRight(180).withBottom(145);
    stageSwitch->setBounds(switchBounds);
    intEditors[0] = std::make_unique<intEditor_t>(std::move(stageSwitch));
}

void ProcessorPane::LayoutControlsTremolo()
{
    assert(processorControlDescription.numFloatParams == 4);
    assert(processorControlDescription.numIntParams == 3);

    namespace lo = theme::layout;
    namespace locon = lo::constants;

    clearAdditionalHamburgerComponents();
    mixEditor = createWidgetAttachedTo<jcmp::Knob>(mixAttachment, "Mix");
    addAdditionalHamburgerComponent(std::move(mixEditor->item));

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
    harmonic->setGlyph(jcmp::GlyphPainter::BIG_PLUS);
    auto plusBounds = bounds.withLeft(148).withRight(160).withTop(0).withBottom(12);
    harmonic->setBounds(plusBounds);
    intEditors[0] = std::make_unique<intEditor_t>(std::move(harmonic));
    attachRebuildToIntAttachment(0);

    bool harmonicSwitch = intAttachments[0]->getValue();

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], "Volume");
    lo::knob<50>(*floatEditors[0], 35, 5);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], "Crossover");
    lo::knob<50>(*floatEditors[3], 100, 5);
    floatEditors[3]->item->setEnabled(harmonicSwitch);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], "LFO Rate");
    lo::knob<50>(*floatEditors[1], 5, 80);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], "LFO Depth");
    lo::knob<50>(*floatEditors[2], 65, 80);
}

void ProcessorPane::layoutControlsPhaser()
{
    assert(processorControlDescription.numFloatParams == 6);
    assert(processorControlDescription.numIntParams == 2);

    namespace lo = theme::layout;
    namespace locon = lo::constants;

    clearAdditionalHamburgerComponents();
    mixEditor = createWidgetAttachedTo<jcmp::Knob>(mixAttachment, "Mix");
    addAdditionalHamburgerComponent(std::move(mixEditor->item));

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

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], "Feedback");
    lo::knob<40>(*floatEditors[0], 5, 0);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], "Resonance");
    lo::knob<40>(*floatEditors[2], 50, 20);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], "Spacing");
    lo::knob<40>(*floatEditors[1], 95, 0);

    floatEditors[5] = createWidgetAttachedTo(floatAttachments[5], "Center Freq");
    lo::knob<40>(*floatEditors[5], 140, 20);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], "LFO Rate");
    lo::knob<50>(*floatEditors[3], 5, 80);

    floatEditors[4] = createWidgetAttachedTo(floatAttachments[4], "LFO Depth");
    lo::knob<50>(*floatEditors[4], 65, 80);
}

void ProcessorPane::layoutControlsMicroGate()
{
    assert(processorControlDescription.numFloatParams == 4);
    assert(processorControlDescription.numIntParams == 0);

    namespace lo = theme::layout;
    namespace locon = lo::constants;

    clearAdditionalHamburgerComponents();
    mixEditor = createWidgetAttachedTo<jcmp::Knob>(mixAttachment, "Mix");
    addAdditionalHamburgerComponent(std::move(mixEditor->item));

    floatEditors[0] = createWidgetAttachedTo(floatAttachments[0], "Hold");
    lo::knob<55>(*floatEditors[0], 25, 0);

    floatEditors[1] = createWidgetAttachedTo(floatAttachments[1], "Loop");
    lo::knob<55>(*floatEditors[1], 105, 0);

    floatEditors[2] = createWidgetAttachedTo(floatAttachments[2], "Threshold");
    lo::knob<55>(*floatEditors[2], 25, 75);

    floatEditors[3] = createWidgetAttachedTo(floatAttachments[3], "Reduction");
    lo::knob<55>(*floatEditors[3], 105, 75);
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
