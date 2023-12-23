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

#include "ProcessorPane.h"
#include "components/SCXTEditor.h"

#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"
#include "connectors/SCXTStyleSheetCreator.h"
#include "sst/jucegui/components/Label.h"

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
    for (auto &i : intSwitches)
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
            processorControlDescription.floatControlDescriptions[i],
            [this, i](const auto &at) { this->processorElementChangedFromGui(at, i); },
            processorView.floatParams[i]);

        floatAttachments[i] = std::move(at);
    }

    for (int i = 0; i < processorControlDescription.numIntParams; ++i)
    {
        auto at = std::make_unique<int_attachment_t>(
            processorControlDescription.intControlDescriptions[i],
            [this](const auto &at) { this->processorChangedFromGui(at); },
            processorView.intParams[i]);
        intAttachments[i] = std::move(at);
    }

    auto at = std::make_unique<attachment_t>(
        datamodel::pmd().asPercent().withName("Mix").withDefault(1.0),
        [this](const auto &at) { this->processorElementChangedFromGui(at, -1); },
        processorView.mix);
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
        // break;
    case dsp::processor::proct_fx_waveshaper:
        // layoutControlsWaveshaper();
        // break;
    default:
        layoutControls();
        break;
    }

    setToggleDataSource(nullptr);
    bypassAttachment = std::make_unique<bool_attachment_t>(
        "Bypass",
        [this](const auto &at) {
            sendToSerialization(cmsg::SetSelectedProcessorStorage({index, processorView}));
        },
        processorView.isActive);
    setToggleDataSource(bypassAttachment.get());

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

        intSwitches[i] = std::move(kn);
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
        intSwitches[i] = std::make_unique<sst::jucegui::components::MultiSwitch>(
            sst::jucegui::components::MultiSwitch::HORIZONTAL);
        intSwitches[i]->setSource(intAttachments[i].get());
        intSwitches[i]->setBounds(okb);
        getContentAreaComponent()->addAndMakeVisible(*intSwitches[i]);
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
        intSwitches[i] = std::make_unique<sst::jucegui::components::MultiSwitch>(
            sst::jucegui::components::MultiSwitch::HORIZONTAL);
        intSwitches[i]->setSource(intAttachments[i].get());
        intSwitches[i]->setBounds(okb);
        getContentAreaComponent()->addAndMakeVisible(*intSwitches[i]);
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

void ProcessorPane::processorChangedFromGui(const attachment_t &at)
{
    updateValueTooltip(at);
    sendToSerialization(cmsg::SetSelectedProcessorStorage({index, processorView}));
}

void ProcessorPane::processorElementChangedFromGui(const attachment_t &at, int whichIdx)
{
    updateValueTooltip(at);
    // sendToSerialization(cmsg::SetSelectedProcessorStorage({index, processorView}));
    if (whichIdx == -1)
    {
        sendToSerialization(cmsg::SetSelectedProcessorSingleValue(
            {forZone, index, (int)cmsg::ProcessorValueIndex::mix, at.value}));
    }
    else
    {
        sendToSerialization(cmsg::SetSelectedProcessorSingleValue(
            {forZone, index, (int)cmsg::ProcessorValueIndex::param0 + whichIdx, at.value}));
    }
}

void ProcessorPane::processorChangedFromGui(const int_attachment_t &at)
{
    sendToSerialization(cmsg::SetSelectedProcessorStorage({index, processorView}));
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