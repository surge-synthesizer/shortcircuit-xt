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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_VARIANTDISPLAY_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_VARIANTDISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/ZoomContainer.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/TabbedComponent.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/NamedPanelDivider.h"

#include "sst/jucegui/component-adapters/DiscreteToReference.h"
#include "app/HasEditor.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"
#include "connectors/PayloadDataAttachment.h"

namespace scxt::ui::app::edit_screen
{
struct SampleWaveform;

struct VariantDisplay : juce::Component, HasEditor
{
    static constexpr int sidePanelWidth{136};
    static constexpr int sidePanelDividerPad{10};
    enum Ctrl
    {
        startP,
        endP,
        startL,
        endL,
        fadeL,
        curve,
        src,
        volume,
        trak,
        tune
    };

    using floatAttachment_t =
        scxt::ui::connectors::PayloadDataAttachment<engine::Zone::SingleVariant>;

    std::unordered_map<Ctrl, std::unique_ptr<connectors::SamplePointDataAttachment>>
        sampleAttachments;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue>>
        sampleEditors;
    std::unordered_map<Ctrl, std::unique_ptr<floatAttachment_t>> sampleFloatAttachments;

    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::Label>> labels;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::GlyphPainter>> glyphLabels;

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

    std::unique_ptr<sst::jucegui::components::NamedPanelDivider> divider;

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
        return getLocalBounds()
            .withTrimmedRight(sidePanelWidth + sidePanelDividerPad)
            .withTrimmedTop(5);
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
    void showSRCMenu();

    void showVariantTabMenu(int variantIdx);

    // Header section
    using boolToggle_t = sst::jucegui::component_adapters::DiscreteToValueReference<
        sst::jucegui::components::ToggleButton, bool>;
    bool fileInfoShowing{false};
    std::unique_ptr<sst::jucegui::components::MenuButton> variantPlaymodeButton, fileButton;
    std::unique_ptr<sst::jucegui::components::Label> variantPlayModeLabel, fileLabel;
    std::unique_ptr<boolToggle_t> fileInfoButton;
    std::unique_ptr<sst::jucegui::components::TextPushButton> editAllButton;
    std::unique_ptr<sst::jucegui::components::GlyphButton> nextFileButton, prevFileButton;

    // sidebar section
    std::unique_ptr<sst::jucegui::components::Label> playModeLabel;
    std::unique_ptr<sst::jucegui::components::MenuButton> playModeButton, loopModeButton;
    std::unique_ptr<sst::jucegui::components::MenuButton> srcButton;
    std::unique_ptr<sst::jucegui::components::GlyphButton> zoomButton;

    std::unique_ptr<juce::Component> noSelectionOverlay;

    struct FileInfos : juce::Component, HasEditor
    {
        FileInfos(HasEditor *e) : HasEditor(e->editor) { setInterceptsMouseClicks(false, false); }

        void paint(juce::Graphics &g) override;

        double sampleRate{1};
        std::string bd;
        size_t sampleLength{0};
        int channels{0};
    };

    std::unique_ptr<FileInfos> fileInfos;

    engine::Zone::Variants &variantView;
    MacroMappingVariantPane *parentPane{nullptr};
};

} // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUITXT_SAMPLEDISPLAY_H
