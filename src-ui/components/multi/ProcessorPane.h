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

#ifndef SCXT_SRC_UI_COMPONENTS_MULTI_PROCESSORPANE_H
#define SCXT_SRC_UI_COMPONENTS_MULTI_PROCESSORPANE_H

#include <unordered_map>
#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/data/Continuous.h"
#include "dsp/processor/processor.h"
#include "components/HasEditor.h"
#include "connectors/PayloadDataAttachment.h"
#include "engine/zone.h"

namespace scxt::ui::multi
{
struct ProcessorPane : sst::jucegui::components::NamedPanel, HasEditor, juce::DragAndDropTarget
{
    // ToDo: shapes of course
    typedef connectors::PayloadDataAttachment<dsp::processor::ProcessorStorage> attachment_t;
    typedef connectors::DiscretePayloadDataAttachment<dsp::processor::ProcessorStorage>
        int_attachment_t;
    typedef connectors::BooleanPayloadDataAttachment<dsp::processor::ProcessorStorage>
        bool_attachment_t;

    dsp::processor::ProcessorStorage processorView;
    dsp::processor::ProcessorControlDescription processorControlDescription;
    int index{0};
    bool forZone{true};
    ProcessorPane(SCXTEditor *, int index, bool);
    ~ProcessorPane();

    void
    setProcessorControlDescriptionAndStorage(const dsp::processor::ProcessorControlDescription &pcd,
                                             const dsp::processor::ProcessorStorage &ps)
    {
        multiZone = false;
        processorControlDescription = pcd;
        processorView = ps;
        rebuildControlsFromDescription();
    }
    void setProcessorStorage(const dsp::processor::ProcessorStorage &p)
    {
        multiZone = false;
        processorView = p;
        repaint();
    }

    void setAsMultiZone(int32_t primaryType, const std::string &nm,
                        const std::set<int32_t> &otherTypes);

    void processorChangedFromGui(const attachment_t &at);
    void processorElementChangedFromGui(const attachment_t &at, int whichItem);
    void processorChangedFromGui(const int_attachment_t &at);

    void rebuildControlsFromDescription();

    void layoutControls();
    void layoutControlsSuperSVF();

    template <typename T = sst::jucegui::components::Knob>
    std::unique_ptr<T> attachContinuousTo(const std::unique_ptr<attachment_t> &);

    void resetControls();

    void resized() override;

    bool isDragging{false};
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

    void showHamburgerMenu();

    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
    void itemDropped(const SourceDetails &dragSourceDetails) override;

    bool multiZone{false};

    std::array<std::unique_ptr<sst::jucegui::components::ContinuousParamEditor>,
               dsp::processor::maxProcessorFloatParams>
        floatEditors;
    std::array<std::unique_ptr<sst::jucegui::components::Label>,
               dsp::processor::maxProcessorFloatParams>
        floatLabels;
    std::array<std::unique_ptr<attachment_t>, dsp::processor::maxProcessorFloatParams>
        floatAttachments;

    std::array<std::unique_ptr<sst::jucegui::components::MultiSwitch>,
               dsp::processor::maxProcessorIntParams>
        intSwitches;
    std::array<std::unique_ptr<sst::jucegui::components::Label>,
               dsp::processor::maxProcessorIntParams>
        intLabels;
    std::array<std::unique_ptr<int_attachment_t>, dsp::processor::maxProcessorFloatParams>
        intAttachments;
    std::unique_ptr<bool_attachment_t> bypassAttachment;

    std::unique_ptr<sst::jucegui::components::Knob> mixEditor;
    std::unique_ptr<sst::jucegui::components::Label> mixLabel;
    std::unique_ptr<attachment_t> mixAttachment;

    std::unique_ptr<sst::jucegui::components::Label> multiLabel;
    std::unique_ptr<sst::jucegui::components::TextPushButton> multiButton;
    std::string multiName;
};
} // namespace scxt::ui::multi

#endif // SHORTCIRCUIT_ADSRPANE_H
