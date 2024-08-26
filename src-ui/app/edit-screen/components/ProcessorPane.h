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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_PROCESSORPANE_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_PROCESSORPANE_H

#include <unordered_map>
#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/LabeledItem.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/layouts/ExplicitLayout.h"
#include "sst/jucegui/data/Continuous.h"
#include "dsp/processor/processor.h"
#include "app/HasEditor.h"
#include "connectors/PayloadDataAttachment.h"
#include "engine/zone.h"

namespace scxt::ui::app::edit_screen
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

    void setAsMultiZone(int32_t primaryType, const std::string &nm,
                        const std::set<int32_t> &otherTypes);

    void rebuildControlsFromDescription();
    void attachRebuildToIntAttachment(int idx);
    void attachRebuildToDeactivateAttachment(int idx);

    void layoutControls();
    void layoutControlsMicroGate();
    void layoutControlsSurgeFilters();
    void layoutControlsFastSVF();
    void layoutControlsWaveshaper();
    void layoutControlsEQNBandParm();
    void layoutControlsEQMorph();
    void layoutControlsEQGraphic();
    void layoutControlsCorrelatedNoiseGen();
    void layoutControlsVAOsc();
    void layoutControlsStringResonator();
    void layoutControlsStaticPhaser();
    void LayoutControlsTremolo();
    void layoutControlsPhaser();
    void layoutControlsChorus();
    void layoutControlsFMFilter();
    void layoutControlsRingMod();

    bool layoutControlsFromJSON(const std::string &jsonpath);
    bool layoutControlsFromJSON(const std::string &jsonpath,
                                sst::jucegui::layout::ExplicitLayout &elo);
    void layoutControlsFromJSONOrDefault(const std::string &jsonpath)
    {
        if (!layoutControlsFromJSON(jsonpath))
            layoutControls();
    }

    template <typename T = sst::jucegui::components::Knob>
    std::unique_ptr<
        sst::jucegui::components::Labeled<sst::jucegui::components::ContinuousParamEditor>>
    createWidgetAttachedTo(const std::unique_ptr<attachment_t> &at, const std::string &label)
    {
        auto res = std::make_unique<
            sst::jucegui::components::Labeled<sst::jucegui::components::ContinuousParamEditor>>();
        auto kn = std::make_unique<T>();
        kn->setSource(at.get());
        setupWidgetForValueTooltip(kn, at);
        kn->setTitle(at->getLabel());
        kn->setDescription(at->getLabel());
        getContentAreaComponent()->addAndMakeVisible(*kn);
        auto lb = std::make_unique<sst::jucegui::components::Label>();
        lb->setText(label);
        lb->setTitle(label);
        lb->setDescription(label);
        lb->setAccessible(false);
        getContentAreaComponent()->addAndMakeVisible(*lb);
        res->item = std::move(kn);
        res->label = std::move(lb);

        return std::move(res);
    }

    template <typename T = sst::jucegui::components::Knob>
    std::unique_ptr<T> createWidgetAttachedTo(const std::unique_ptr<int_attachment_t> &at)
    {
        auto kn = std::make_unique<T>();
        kn->setSource(at.get());
        kn->setTitle(at->getLabel());
        kn->setDescription(at->getLabel());

        getContentAreaComponent()->addAndMakeVisible(*kn);

        return std::move(kn);
    }

    template <typename T = sst::jucegui::components::ToggleButton>
    std::unique_ptr<T> createWidgetAttachedTo(const std::unique_ptr<bool_attachment_t> &at)
    {
        auto kn = std::make_unique<T>();
        kn->setSource(at.get());
        kn->setTitle(at->getLabel());
        kn->setDescription(at->getLabel());

        getContentAreaComponent()->addAndMakeVisible(*kn);

        return std::move(kn);
    }

    template <typename T = sst::jucegui::components::Knob>
    std::unique_ptr<
        sst::jucegui::components::Labeled<sst::jucegui::components::DiscreteParamEditor>>
    createWidgetAttachedTo(const std::unique_ptr<int_attachment_t> &at, const std::string &label)
    {
        auto res = std::make_unique<
            sst::jucegui::components::Labeled<sst::jucegui::components::DiscreteParamEditor>>();
        res->item = createWidgetAttachedTo<T>(at);
        auto lb = std::make_unique<sst::jucegui::components::Label>();
        lb->setText(label);
        getContentAreaComponent()->addAndMakeVisible(*lb);
        res->label = std::move(lb);

        return std::move(res);
    }

    template <typename T = sst::jucegui::components::Label>
    std::unique_ptr<T> createLabel(const std::string &txt)
    {
        auto res = std::make_unique<T>();
        res->setText(txt);
        getContentAreaComponent()->addAndMakeVisible(*res);
        return std::move(res);
    }

    void createHamburgerStereo(int attachmentId);

    void resetControls();

    void resized() override;

    bool isDragging{false};
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

    void showHamburgerMenu();

    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
    void itemDropped(const SourceDetails &dragSourceDetails) override;
    void itemDragEnter(const SourceDetails &dragSourceDetails) override;
    void itemDragExit(const SourceDetails &dragSourceDetails) override;
    void reapplyStyle();

    bool multiZone{false};

    using floatEditor_t =
        sst::jucegui::components::Labeled<sst::jucegui::components::ContinuousParamEditor>;
    using intEditor_t =
        sst::jucegui::components::Labeled<sst::jucegui::components::DiscreteParamEditor>;

    std::array<std::unique_ptr<floatEditor_t>, dsp::processor::maxProcessorFloatParams>
        floatEditors;
    std::array<std::unique_ptr<intEditor_t>, dsp::processor::maxProcessorFloatParams>
        floatDeactivateEditors;
    std::array<std::unique_ptr<attachment_t>, dsp::processor::maxProcessorFloatParams>
        floatAttachments;

    std::array<std::unique_ptr<intEditor_t>, dsp::processor::maxProcessorIntParams> intEditors;
    std::array<std::unique_ptr<int_attachment_t>, dsp::processor::maxProcessorFloatParams>
        intAttachments;
    std::array<std::unique_ptr<bool_attachment_t>, dsp::processor::maxProcessorFloatParams>
        deactivateAttachments;

    std::unique_ptr<bool_attachment_t> bypassAttachment, keytrackAttackment, temposyncAttachment;

    std::vector<std::unique_ptr<juce::Component>> otherEditors;

    std::unique_ptr<
        sst::jucegui::components::Labeled<sst::jucegui::components::ContinuousParamEditor>>
        mixEditor;
    std::unique_ptr<attachment_t> mixAttachment;

    std::unique_ptr<sst::jucegui::components::Label> multiLabel;
    std::unique_ptr<sst::jucegui::components::TextPushButton> multiButton;

    std::string multiName;

    // Layout helpers
    std::unique_ptr<intEditor_t>
    createAndLayoutPowerButton(const sst::jucegui::layout::ExplicitLayout &elo,
                               const std::string &lotag,
                               const std::unique_ptr<int_attachment_t> &at)
    {
        namespace jcmp = sst::jucegui::components;
        auto res = createWidgetAttachedTo<jcmp::ToggleButton>(at);
        res->setDrawMode(jcmp::ToggleButton::DrawMode::GLYPH);
        res->setGlyph(jcmp::GlyphPainter::SMALL_POWER_LIGHT);
        res->setBounds(elo.powerButtonPositionFor(lotag));
        return std::make_unique<intEditor_t>(std::move(res));
    }

    template <typename T = sst::jucegui::components::Knob>
    std::unique_ptr<floatEditor_t> createAndLayoutLabeledFloatWidget(
        const sst::jucegui::layout::ExplicitLayout &elo, const std::string &lotag,
        const std::unique_ptr<attachment_t> &at, const std::string &label = "")
    {
        auto res = createWidgetAttachedTo<T>(at, label.empty() ? at->getLabel() : label);
        res->item->setBounds(elo.positionFor(lotag));
        res->label->setBounds(elo.labelPositionFor(lotag));

        return res;
    }
};
} // namespace scxt::ui::app::edit_screen

#endif // SHORTCIRCUIT_ADSRPANE_H
