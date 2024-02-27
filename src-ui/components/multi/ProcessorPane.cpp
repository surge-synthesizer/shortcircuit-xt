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
#include <sst/jucegui/components/MenuButton.h>

namespace scxt::ui::multi
{
namespace cmsg = scxt::messaging::client;

ProcessorPane::ProcessorPane(SCXTEditor *e, int index, bool fz)
    : HasEditor(e), sst::jucegui::components::NamedPanel("PROCESSOR " + std::to_string(index + 1)),
      index(index), forZone(fz)
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
    sst::jucegui::components::NamedPanel::resized();
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
    case dsp::processor::proct_SuperSVF:
        layoutControlsSuperSVF();
        break;
    case dsp::processor::proct_eq_1band_parametric_A:
    case dsp::processor::proct_eq_2band_parametric_A:
    case dsp::processor::proct_eq_3band_parametric_A:
        // layoutControlsEQNBandParm();
        layoutControls();
        break;
    case dsp::processor::proct_fx_waveshaper:
        layoutControlsWaveshaper();
        break;
    default:
        layoutControls();
        break;
    }

    setToggleDataSource(nullptr);
    bypassAttachment = std::make_unique<bool_attachment_t>("Bypass", processorView.isActive);
    connectors::configureUpdater<cmsg::UpdateZoneOrGroupProcessorBoolValue, bool_attachment_t,
                                 bool_attachment_t::onGui_t>(*bypassAttachment, processorView, this,
                                                             forZone, index);
    setToggleDataSource(bypassAttachment.get());

    repaint();

    if (forZone)
    {
        editor->themeApplier.applyZoneMultiScreenTheme(this);
    }
    else
    {
        editor->themeApplier.applyGroupMultiScreenTheme(this);
    }
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
        floatEditors[i] = attachContinuousTo<sst::jucegui::components::Knob>(floatAttachments[i]);
        floatEditors[i]->setBounds(kb);

        auto label = std::make_unique<sst::jucegui::components::Label>();
        label->setText(processorControlDescription.floatControlDescriptions[i].name);
        getContentAreaComponent()->addAndMakeVisible(*label);

        floatLabels[i] = std::move(label);
        floatLabels[i]->setBounds(lb);

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

        intEditors[i] = std::move(kn);
        intLabels[i] = std::move(label);

        kb = kb.translated(kw, 0);
        lb = lb.translated(kw, 0);
        if ((i + processorControlDescription.numFloatParams) % 4 == 3)
        {
            kb = kb.translated(-getContentArea().getWidth(), kw + labelHeight);
            lb = lb.translated(-getContentArea().getWidth(), kw + labelHeight);
        }
    }

    {
        auto label = std::make_unique<sst::jucegui::components::Label>();
        label->setText("mix");
        label->setBounds(lb);
        getContentAreaComponent()->addAndMakeVisible(*label);
        mixLabel = std::move(label);

        mixEditor = attachContinuousTo<sst::jucegui::components::Knob>(mixAttachment);
        mixEditor->setBounds(kb);
    }
}

// May want to break this up
void ProcessorPane::layoutControlsSuperSVF()
{
    // OK so we know we have 2 controls (cutoff and resonance), a mix, and two ints
    assert(processorControlDescription.numFloatParams == 2);
    assert(processorControlDescription.numIntParams == 2);

    auto labelHeight = 18;
    auto rc = getContentAreaComponent()->getLocalBounds();
    auto rcw = rc.getWidth();
    auto pct = 0.35;
    auto kw = rcw * pct;

    auto kb = rc.withHeight(kw).withWidth(kw).translated(rcw * (0.5 - pct) * 0.5, 0);
    floatEditors[0] = attachContinuousTo(floatAttachments[0]);
    floatEditors[0]->setBounds(kb);

    floatLabels[0] = std::make_unique<sst::jucegui::components::Label>();
    floatLabels[0]->setText("Cutoff");
    floatLabels[0]->setBounds(kb.translated(0, kb.getHeight()).withHeight(18));
    getContentAreaComponent()->addAndMakeVisible(*floatLabels[0]);

    kb = kb.translated(rcw * 0.5, 0);
    floatEditors[1] = attachContinuousTo(floatAttachments[1]);
    floatEditors[1]->setBounds(kb);
    floatLabels[1] = std::make_unique<sst::jucegui::components::Label>();
    floatLabels[1]->setText("Resonance");
    floatLabels[1]->setBounds(kb.translated(0, kb.getHeight()).withHeight(18));
    getContentAreaComponent()->addAndMakeVisible(*floatLabels[1]);

    auto okb = floatLabels[0]->getBounds().expanded((0.5 - pct) * 0.5 * rcw, 0);

    okb = okb.translated(0, 25).withHeight(25);
    for (int i = 0; i < 2; ++i)
    {
        intEditors[i] = std::make_unique<sst::jucegui::components::MultiSwitch>(
            sst::jucegui::components::MultiSwitch::HORIZONTAL);
        intEditors[i]->setSource(intAttachments[i].get());
        intEditors[i]->setBounds(okb);
        getContentAreaComponent()->addAndMakeVisible(*intEditors[i]);
        okb = okb.translated(0, 28);
    }

    auto mixr = floatLabels[1]->getBounds().translated(0, 25).reduced(pct * 0.2 * rcw, 0);
    mixr = mixr.withHeight(mixr.getWidth());
    mixEditor = attachContinuousTo(mixAttachment);
    mixEditor->setBounds(mixr);
    getContentAreaComponent()->addAndMakeVisible(*mixEditor);

    mixLabel = std::make_unique<sst::jucegui::components::Label>();
    mixLabel->setText("Mix");
    mixLabel->setBounds(mixr.translated(0, mixr.getHeight()).withHeight(18));
    getContentAreaComponent()->addAndMakeVisible(*mixLabel);
}

void ProcessorPane::layoutControlsWaveshaper()
{
    // DriveBiasGain in the floats
    // TypeOS in the ints
    // and mix of course
    assert(processorControlDescription.numFloatParams == 3);
    assert(processorControlDescription.numIntParams == 2);

    auto labelHeight = 18;
    auto rc = getContentAreaComponent()->getLocalBounds();
    auto rcw = rc.getWidth();
    auto pct = 0.43;
    auto kw = rcw * pct;

    auto kb = rc.withHeight(kw).withWidth(kw).translated(rcw * (0.5 - pct) * 0.5, 0);
    floatEditors[0] = attachContinuousTo(floatAttachments[0]);
    floatEditors[0]->setBounds(kb);

    floatLabels[0] = std::make_unique<sst::jucegui::components::Label>();
    floatLabels[0]->setText("Drive");
    floatLabels[0]->setBounds(kb.translated(0, kb.getHeight()).withHeight(18));
    getContentAreaComponent()->addAndMakeVisible(*floatLabels[0]);

    kb =
        kb.translated(rcw * 0.5, 0).withWidth(kb.getWidth() * 0.5).withHeight(kb.getHeight() * 0.5);
    floatEditors[1] = attachContinuousTo(floatAttachments[1]);
    floatEditors[1]->setBounds(kb.reduced(1));
    floatLabels[1] = std::make_unique<sst::jucegui::components::Label>();
    floatLabels[1]->setText("Bias");
    floatLabels[1]->setBounds(kb.translated(0, kb.getHeight()).withHeight(18));
    getContentAreaComponent()->addAndMakeVisible(*floatLabels[1]);
    auto mkb = kb;

    kb = kb.translated(0, kb.getHeight() + 20);
    floatEditors[2] = attachContinuousTo(floatAttachments[2]);
    floatEditors[2]->setBounds(kb.reduced(1));
    floatLabels[2] = std::make_unique<sst::jucegui::components::Label>();
    floatLabels[2]->setText("Gain");
    floatLabels[2]->setBounds(kb.translated(0, kb.getHeight()).withHeight(18));
    getContentAreaComponent()->addAndMakeVisible(*floatLabels[2]);

    /*
    okb = okb.translated(0, 25).withHeight(25);
    for (int i = 0; i < 2; ++i)
    {
        intEditors[i] = std::make_unique<sst::jucegui::components::MultiSwitch>(
            sst::jucegui::components::MultiSwitch::HORIZONTAL);
        intEditors[i]->setSource(intAttachments[i].get());
        intEditors[i]->setBounds(okb);
        getContentAreaComponent()->addAndMakeVisible(*intEditors[i]);
        okb = okb.translated(0, 28);
    }
     */

    mkb = mkb.translated(mkb.getWidth(), 0);
    mixEditor = attachContinuousTo(mixAttachment);
    mixEditor->setBounds(mkb.reduced(1));
    getContentAreaComponent()->addAndMakeVisible(*mixEditor);

    mixLabel = std::make_unique<sst::jucegui::components::Label>();
    mixLabel->setText("Mix");
    mixLabel->setBounds(mkb.translated(0, mkb.getHeight()).withHeight(18));
    getContentAreaComponent()->addAndMakeVisible(*mixLabel);

    auto osl = std::make_unique<sst::jucegui::components::ToggleButton>();
    osl->setDrawMode(sst::jucegui::components::ToggleButton::DrawMode::LABELED);
    osl->setSource(intAttachments[1].get());
    osl->setLabel("o/s");
    osl->setBounds(mkb.translated(0, mkb.getHeight() + 18).reduced(3, 9));
    getContentAreaComponent()->addAndMakeVisible(*osl);
    intEditors[1] = std::move(osl);

    auto ja = getContentAreaComponent()->getLocalBounds();
    ja = ja.withTop(ja.getBottom() - 22).translated(0, -3).reduced(3, 0);

    auto wss = std::make_unique<sst::jucegui::components::JogUpDownButton>();
    // auto wss = std::make_unique<sst::jucegui::components::MenuButtonDiscreteEditor>();
    editor->configureHasDiscreteMenuBuilder(wss.get());

    wss->popupMenuBuilder->mode =
        sst::jucegui::components::DiscreteParamMenuBuilder::Mode::GROUP_LIST;
    wss->popupMenuBuilder->groupList = sst::waveshapers::WaveshaperGroupName();

    wss->setSource(intAttachments[0].get());
    wss->setBounds(ja);
    setupWidgetForValueTooltip(wss, intAttachments[0]);

    getContentAreaComponent()->addAndMakeVisible(*wss);
    intEditors[0] = std::move(wss);
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

    sendToSerialization(cmsg::SwapZoneProcessors({index, pp->index}));
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