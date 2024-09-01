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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_SAMPLEDISPLAY_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_SAMPLEDISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/ZoomContainer.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/TabbedComponent.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/TextPushButton.h"

#include "sst/jucegui/component-adapters/DiscreteToReference.h"
#include "app/HasEditor.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"
#include "connectors/PayloadDataAttachment.h"

namespace scxt::ui::app::edit_screen
{
struct SampleWaveform;

struct VariantDisplay : juce::Component, HasEditor
{
    static constexpr int sidePanelWidth{150};
    enum Ctrl
    {
        startP,
        endP,
        startL,
        endL,
        fadeL
    };

    std::unordered_map<Ctrl, std::unique_ptr<connectors::SamplePointDataAttachment>>
        sampleAttachments;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue>>
        sampleEditors;

    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::Label>> labels;

    typedef connectors::PayloadDataAttachment<engine::Zone::Variants, int> sample_attachment_t;

    std::unique_ptr<sample_attachment_t> loopCntAttachment;
    std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue> loopCnt;

    std::unique_ptr<connectors::BooleanPayloadDataAttachment<engine::Zone::Variants>>
        loopAttachment, reverseAttachment;
    std::unique_ptr<sst::jucegui::components::ToggleButton> loopActive, reverseActive;

    struct ZoomableWaveform
    {
        std::unique_ptr<sst::jucegui::components::ZoomContainer> waveformViewport;
        SampleWaveform *waveform{nullptr};
    };
    std::array<ZoomableWaveform, maxVariantsPerZone> waveforms;

    struct MyTabbedComponent : sst::jucegui::components::TabbedComponent,
                               HasEditor,
                               juce::FileDragAndDropTarget,
                               juce::DragAndDropTarget
    {
        MyTabbedComponent(VariantDisplay *d)
            : sst::jucegui::components::TabbedComponent(juce::TabbedButtonBar::TabsAtTop),
              HasEditor(d->editor), display(d)
        {
        }
        void currentTabChanged(int newCurrentTabIndex,
                               const juce::String &newCurrentTabName) override;

        std::optional<std::string>
        sourceDetailsDragAndDropSample(const SourceDetails &dragSourceDetails);

        bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;

        bool isInterestedInFileDrag(const juce::StringArray &files) override;

        void filesDropped(const juce::StringArray &files, int x, int y) override;

        void itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override;

        int getTabIndexFromPosition(int x, int y);
        VariantDisplay *display;
    };

    std::unique_ptr<MyTabbedComponent> waveformsTabbedGroup;
    size_t selectedVariation{0};

    std::unique_ptr<juce::FileChooser> fileChooser;

    VariantDisplay(MacroMappingVariantPane *p);

    void rebuildForSelectedVariation(size_t sel, bool rebuildTabs = true);

    ~VariantDisplay() { reset(); }

    void reset()
    {
        for (auto &[k, c] : sampleEditors)
            c.reset();
    }

    void onSamplePointChangedFromGUI();

    juce::Rectangle<int> sampleDisplayRegion()
    {
        return getLocalBounds().withTrimmedRight(sidePanelWidth);
    }

    void resized() override;

    bool active{false};
    void setActive(bool b)
    {
        active = b;
        playModeButton->setVisible(b);
        loopActive->setVisible(b);
        loopModeButton->setVisible(b);
        reverseActive->setVisible(b);
        loopDirectionButton->setVisible(b);
        for (const auto &[k, p] : sampleEditors)
            p->setVisible(b);
        for (const auto &[k, l] : labels)
            l->setVisible(b);

        if (active)
            rebuild();
        repaint();
    }

    void rebuild();
    void selectNextFile(bool selectForward);
    void showFileBrowser();
    void showFileInfos()
    {
        auto show{fileInfoShowing};
        fileInfos->setVisible(show);
    }

    void showPlayModeMenu();
    void showLoopModeMenu();
    void showVariantPlaymodeMenu();
    void showLoopDirectionMenu();

    using boolToggle_t = sst::jucegui::component_adapters::DiscreteToValueReference<
        sst::jucegui::components::ToggleButton, bool>;
    bool fileInfoShowing{false};

    std::unique_ptr<sst::jucegui::components::Label> playModeLabel, variantPlayModeLabel, fileLabel;
    std::unique_ptr<boolToggle_t> fileInfoButton;

    std::unique_ptr<sst::jucegui::components::TextPushButton> variantPlaymodeButton;
    std::unique_ptr<juce::TextButton> playModeButton, loopModeButton, loopDirectionButton,
        fileButton, nextFileButton, prevFileButton;

    struct FileInfos : juce::Component
    {
        FileInfos()
        {
            auto createComp = [this](auto &comp, std::string text) {
                comp = std::make_unique<sst::jucegui::components::Label>();
                comp->setText(text);
                addAndMakeVisible(*comp);
            };

            createComp(srLabel, "SR: ");
            createComp(bdLabel, "BD: ");
            createComp(sizeLabel, "SMP: ");
            createComp(lengthLabel, "LEN: ");

            createComp(srVal, "Sample Rate Val");
            createComp(bdVal, "Bit Depth Val");
            createComp(sizeVal, "Size Val");
            createComp(lengthVal, "Length Val");
        }

        void resized()
        {
            auto p{getLocalBounds().withWidth(40)};
            srLabel->setBounds(p.withWidth(30));
            p.translate(srLabel->getWidth(), 0);
            srVal->setBounds(p.withWidth(60));
            p.translate(srVal->getWidth(), 0);

            bdLabel->setBounds(p.withWidth(30));
            p.translate(bdLabel->getWidth(), 0);
            bdVal->setBounds(p.withWidth(60));
            p.translate(bdVal->getWidth(), 0);

            sizeLabel->setBounds(p.withWidth(30));
            p.translate(sizeLabel->getWidth(), 0);
            sizeVal->setBounds(p.withWidth(60));
            p.translate(sizeVal->getWidth(), 0);

            lengthLabel->setBounds(p.withWidth(30));
            p.translate(lengthLabel->getWidth(), 0);
            lengthVal->setBounds(p.withWidth(60));
            p.translate(lengthVal->getWidth(), 0);
        }

        std::unique_ptr<sst::jucegui::components::Label> srLabel, bdLabel, sizeLabel, lengthLabel;
        std::unique_ptr<sst::jucegui::components::Label> srVal, bdVal, sizeVal, lengthVal;
    };

    std::unique_ptr<FileInfos> fileInfos;

    engine::Zone::Variants &variantView;
    MacroMappingVariantPane *parentPane{nullptr};
};

} // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUITXT_SAMPLEDISPLAY_H
