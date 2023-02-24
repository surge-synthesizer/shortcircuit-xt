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
    hasHamburger = true;
    cmsg::clientSendToSerialization(cmsg::ProcessorMetadataAndData(index), e->msgCont);

    onHamburger = [safeThis = juce::Component::SafePointer(this)]() {
        if (safeThis)
            safeThis->showHamburgerMenu();
    };
}

ProcessorPane::~ProcessorPane() { resetControls(); }

void ProcessorPane::updateTooltip(const attachment_t &at)
{
    editor->setTooltipContents(at.label + " = " + at.description.valueToString(at.value));
}

void ProcessorPane::resized() { rebuildControlsFromDescription(); }

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
                cmsg::clientSendToSerialization(cmsg::SetSelectedProcessorType({wt->index, type}),
                                                wt->editor->msgCont);
            }
        });
    }

    p.showMenuAsync(juce::PopupMenu::Options());
}

void ProcessorPane::resetControls()
{
    removeAllChildren();

    // we assume the controls clear before attachments so make sure of that
    for (auto &k : floatKnobs)
        k.reset(nullptr);
    for (auto &i : intSwitches)
        i.reset(nullptr);
    mixKnob.reset(nullptr);
}

void ProcessorPane::rebuildControlsFromDescription()
{
    resetControls();
    // TODO: Am I actually an off type? Then bail.

    if (!isEnabled())
    {
        setName("PROCESSOR " + std::to_string(index + 1));
        repaint();
        return;
    }

    setName(processorControlDescription.typeDisplayName);

    if (processorControlDescription.type == dsp::processor::proct_none)
    {
        repaint();
        return;
    }

    auto labelHeight = 18;
    auto rc = getContentArea();
    auto rcw = rc.getWidth();
    auto kw = rcw * 0.25;

    auto kb = rc.withHeight(kw).withWidth(kw);
    auto lb = kb.translated(0, kw).withHeight(labelHeight);

    for (int i = 0; i < processorControlDescription.numFloatParams; ++i)
    {
        auto at = std::make_unique<attachment_t>(
            this, processorControlDescription.floatControlDescriptions[i],
            processorControlDescription.floatControlNames[i],
            [this](const auto &at) { this->processorChangedFromGui(at); },
            [idx = i](const auto &pl) { return pl.floatParams[idx]; },
            processorView.floatParams[i]);
        auto kn = std::make_unique<sst::jucegui::components::Knob>();
        kn->setSource(at.get());
        kn->onBeginEdit = [this, &knRef = *kn, &atRef = *at]() {
            editor->showTooltip(knRef);
            updateTooltip(atRef);
        };
        kn->onEndEdit = [this]() { editor->hideTooltip(); };
        kn->setBounds(kb);
        addAndMakeVisible(*kn);
        floatAttachments[i] = std::move(at);
        floatKnobs[i] = std::move(kn);

        auto label = std::make_unique<sst::jucegui::components::Label>();
        label->setText(processorControlDescription.floatControlNames[i]);
        label->setBounds(lb);
        addAndMakeVisible(*label);

        floatLabels[i] = std::move(label);

        kb = kb.translated(kw, 0);
        lb = lb.translated(kw, 0);
        if (i % 4 == 3)
        {
            kb = kb.translated(-getWidth(), kw + labelHeight);
            lb = lb.translated(-getWidth(), kw + labelHeight);
        }
    }

    for (int i = 0; i < processorControlDescription.numIntParams; ++i)
    {
        auto at = std::make_unique<int_attachment_t>(
            this, processorControlDescription.intControlDescriptions[i],
            processorControlDescription.intControlNames[i],
            [this](const auto &at) { this->processorChangedFromGui(at); },
            [idx = i](const auto &pl) { return pl.intParams[idx]; },
            processorView.intParams[i]);
        auto kn = std::make_unique<sst::jucegui::components::MultiSwitch>();
        kn->setSource(at.get());
        kn->setBounds(kb);
        addAndMakeVisible(*kn);

        auto label = std::make_unique<sst::jucegui::components::Label>();
        label->setText(processorControlDescription.intControlNames[i]);
        label->setBounds(lb);
        addAndMakeVisible(*label);

        intAttachments[i] = std::move(at);
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
        addAndMakeVisible(*label);
        mixLabel = std::move(label);

        auto at = std::make_unique<attachment_t>(
            this, datamodel::cdPercent, "Mix",
            [this](const auto &at) { this->processorChangedFromGui(at); },
            [](const auto &pl) { return pl.mix; }, processorView.mix);
        auto kn = std::make_unique<sst::jucegui::components::Knob>();
        kn->setSource(at.get());
        kn->onBeginEdit = [this, &knRef = *kn, &atRef = *at]() {
            editor->showTooltip(knRef);
            updateTooltip(atRef);
        };
        kn->onEndEdit = [this]() { editor->hideTooltip(); };
        kn->setBounds(kb);
        addAndMakeVisible(*kn);
        mixAttachment = std::move(at);
        mixKnob = std::move(kn);
    }
    repaint();
}

void ProcessorPane::processorChangedFromGui(const attachment_t &at)
{
    updateTooltip(at);
    cmsg::clientSendToSerialization(cmsg::SetSelectedProcessorStorage({index, processorView}),
                                    editor->msgCont);
}

void ProcessorPane::processorChangedFromGui(const int_attachment_t &at)
{
    cmsg::clientSendToSerialization(cmsg::SetSelectedProcessorStorage({index, processorView}),
                                    editor->msgCont);
}

} // namespace scxt::ui::multi