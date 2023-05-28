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

ProcessorPane::ProcessorPane(SCXTEditor *e, int index)
    : HasEditor(e), sst::jucegui::components::NamedPanel("PROCESSOR " + std::to_string(index + 1)),
      index(index)
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

void ProcessorPane::updateTooltip(const attachment_t &at)
{
    editor->setTooltipContents(at.label, at.description.valueToString(at.value).value_or("Error"));
}

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
    for (const auto &pd : editor->allProcessors)
    {
        p.addItem(pd.displayName, [wt = juce::Component::SafePointer(this), type = pd.id]() {
            if (wt)
            {
                wt->sendToSerialization(cmsg::SetSelectedProcessorType({wt->index, type}));
            }
        });
    }

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
            this, processorControlDescription.floatControlDescriptions[i],
            [this](const auto &at) { this->processorChangedFromGui(at); },
            [idx = i](const auto &pl) { return pl.floatParams[idx]; },
            processorView.floatParams[i]);

        floatAttachments[i] = std::move(at);
    }

    for (int i = 0; i < processorControlDescription.numIntParams; ++i)
    {
        auto at = std::make_unique<int_attachment_t>(
            this, processorControlDescription.intControlDescriptions[i],
            [this](const auto &at) { this->processorChangedFromGui(at); },
            [idx = i](const auto &pl) { return pl.intParams[idx]; }, processorView.intParams[i]);
        intAttachments[i] = std::move(at);
    }

    auto at = std::make_unique<attachment_t>(
        this, datamodel::pmd().asPercent().withName("Mix").withDefault(1.0),
        [this](const auto &at) { this->processorChangedFromGui(at); },
        [](const auto &pl) { return pl.mix; }, processorView.mix);
    mixAttachment = std::move(at);

    switch (processorControlDescription.type)
    {
    case dsp::processor::proct_SuperSVF:
        layoutControlsSuperSVF();
        break;
    default:
        layoutControls();
        break;
    }

    setToggleDataSource(nullptr);
    bypassAttachment = std::make_unique<bool_attachment_t>(
        this, "Bypass",
        [this](const auto &at) {
            sendToSerialization(cmsg::SetSelectedProcessorStorage({index, processorView}));
        },
        [](const auto &pl) { return pl.isActive; }, processorView.isActive);
    setToggleDataSource(bypassAttachment.get());

    repaint();
}

template <typename T>
std::unique_ptr<T> ProcessorPane::attachContinuousTo(const std::unique_ptr<attachment_t> &at)
{
    auto kn = std::make_unique<T>();
    kn->setSource(at.get());
    kn->onBeginEdit = [this, &knRef = *kn, &atRef = *at]() {
        editor->showTooltip(knRef);
        updateTooltip(atRef);
    };
    kn->onEndEdit = [this]() { editor->hideTooltip(); };

    getContentAreaComponent()->addAndMakeVisible(*kn);
    return std::move(kn);
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

void ProcessorPane::processorChangedFromGui(const attachment_t &at)
{
    updateTooltip(at);
    sendToSerialization(cmsg::SetSelectedProcessorStorage({index, processorView}));
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

    SCLOG("Swapping processors " << SCD(index) << SCD(pp->index));
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

} // namespace scxt::ui::multi