/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_VARIANTDISPLAY_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_VARIANTDISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/ZoomContainer.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/DraggableTextEditableDiscreteValue.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/TabbedComponent.h"
#include "sst/jucegui/components/ToggleButton.h"
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
        pan,
        tune
    };

    using floatAttachment_t =
        scxt::ui::connectors::PayloadDataAttachment<engine::Zone::SingleVariant>;

    std::unordered_map<Ctrl, std::unique_ptr<connectors::SamplePointDataAttachment>>
        sampleAttachments;
    // continuous (float/dummy) text editors: curve, volume, pan, tune
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue>>
        sampleEditors;
    // discrete (integer frame) text editors: the sample-point controls
    std::unordered_map<
        Ctrl, std::unique_ptr<sst::jucegui::components::DraggableTextEditableDiscreteValue>>
        discreteSampleEditors;
    std::unordered_map<Ctrl, std::unique_ptr<floatAttachment_t>> sampleFloatAttachments;

    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::Label>> labels;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::GlyphPainter>> glyphLabels;

    typedef connectors::DiscretePayloadDataAttachment<engine::Zone::Variants, int>
        sample_attachment_t;

    std::unique_ptr<sample_attachment_t> loopCntAttachment;
    std::unique_ptr<sst::jucegui::components::DraggableTextEditableDiscreteValue> loopCnt;

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

    /*
     * selectedVariation outlives the zone it was chosen in - the pane is not rebuilt when
     * the selection changes - so a remembered index can point past the variants the new
     * zone actually has. Restoring lands on the last loaded variant; only clicking the "+"
     * tab selects the empty slot.
     */
    enum struct EmptySlot
    {
        Clamp,
        Allow
    };
    void rebuildForSelectedVariation(size_t sel, bool rebuildTabs = true,
                                     EmptySlot es = EmptySlot::Clamp);

    ~VariantDisplay() { reset(); }

    void reset()
    {
        for (auto &[k, c] : sampleEditors)
            c.reset();
        for (auto &[k, c] : discreteSampleEditors)
            c.reset();
    }

    /*
     * Send an edit, naming the field by the reference that was just written. The offset
     * into SingleVariant is what edit-all propagates by, the same way the int and float
     * update messages address their targets, so nothing here enumerates the field set.
     */
    template <typename T> void onVariantFieldChanged(const T &field)
    {
        auto &v = variantView.variants[selectedVariation];
        auto off = (const uint8_t *)&field - (const uint8_t *)&v;
        onSamplePointChangedFromGUI(off, sizeof(T));
    }
    void onSamplePointChangedFromGUI(ptrdiff_t off, size_t sz);

    /*
     * Continuous edits (a waveform hot-zone drag, a draggable value) bracket
     * themselves so the whole gesture is one undo entry rather than one per
     * mouse move. The begin snapshot covers everything until the end.
     */
    void beginVariantGesture();
    void endVariantGesture();

    // wire a draggable widget's edit gesture to the bracket above
    template <typename W> void bracketGesture(W &w)
    {
        w.onBeginEdit = [this]() { beginVariantGesture(); };
        w.onEndEdit = [this]() { endVariantGesture(); };
    }

    // key into SCXTEditor::otherTabSelection, so edit-all outlives zone changes and sessions
    static constexpr const char *editAllTabKey{"variant.editall"};
    bool editAll{false};
    // the engine's fan-out replayed on the local view, so the other tabs read right at once
    void propagateEditAll(ptrdiff_t off, size_t sz);
    // is any active variant normalized, for the edit-all clear menu item
    bool anyVariantNormalized() const;

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
        for (const auto &[k, p] : discreteSampleEditors)
            p->setVisible(b);
        for (const auto &[k, l] : labels)
            l->setVisible(b);

        /*
         * With no sample there is nothing in the waveform to look at, so zooming
         * it is meaningless. This hides the scrollbars and the magnifier and
         * stops the wheel and middle drag acting on an empty view.
         */
        for (auto &w : waveforms)
            if (w.waveformViewport)
                w.waveformViewport->setZoomEnabled(b);

        if (active)
            rebuild();
        repaint();
    }

    /*
     * A reversed variant draws mirrored, so the visible window mirrors with it and the same audio
     * stays on screen rather than a zoomed in view jumping head to tail. The window lives in the
     * ZoomContainer's scrollbar, which is the one piece of the flip the waveform cannot do itself.
     */
    void mirrorWaveformViewports();

    void rebuild();
    void selectNextFile(bool selectForward);
    void showFileBrowser();
    void showFileInfos()
    {
        auto show{fileInfoShowing};
        fileInfos->setVisible(show);
    }

    // one shot is the zone AEG's sample gated mode now, and it governs what looping can do
    bool aegIsSampleGated() const;

    void showPlayModeMenu();
    void showLoopModeMenu();
    void showVariantPlaymodeMenu();
    void showSRCMenu();

    void showVariantTabMenu(int variantIdx);

    void showHamburgerMenu();

    // Header section
    using boolToggle_t = sst::jucegui::component_adapters::DiscreteToValueReference<
        sst::jucegui::components::ToggleButton, bool>;
    bool fileInfoShowing{false};
    std::unique_ptr<sst::jucegui::components::MenuButton> variantPlaymodeButton, fileButton;
    std::unique_ptr<sst::jucegui::components::Label> variantPlayModeLabel, fileLabel;
    std::unique_ptr<boolToggle_t> fileInfoButton;
    std::unique_ptr<boolToggle_t> editAllButton;
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
