/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_SHARED_PARTSIDEBARCARD_H
#define SCXT_SRC_SCXT_PLUGIN_APP_SHARED_PARTSIDEBARCARD_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/TextEditor.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "app/HasEditor.h"
#include "connectors/PayloadDataAttachment.h"
#include "engine/part.h"

namespace scxt::ui::app::shared
{
struct PartSidebarCard : juce::Component,
                         HasEditor,
                         sst::jucegui::components::WithIdleTimer,
                         juce::TextEditor::Listener,
                         juce::FileDragAndDropTarget
{
    int part;

    using boolattachment_t =
        scxt::ui::connectors::BooleanPayloadDataAttachment<engine::Part::PartConfiguration>;
    using attachment_t =
        scxt::ui::connectors::PayloadDataAttachment<engine::Part::PartConfiguration>;
    using tsposeattachment_t =
        scxt::ui::connectors::PayloadDataAttachment<engine::Part::PartConfiguration, int32_t>;

    static constexpr int row0{2}, rowHeight{20}, rowMargin{2}, tuningTransposeSplit{38};

    static constexpr int height{88}, tallHeight{176}, width{172};

    std::unique_ptr<sst::jucegui::components::ToggleButton> solo, mute;
    std::unique_ptr<boolattachment_t> muteAtt, soloAtt;
    std::unique_ptr<attachment_t> levelAtt, panAtt, tuningAtt;
    std::unique_ptr<tsposeattachment_t> transposeAtt;
    std::unique_ptr<sst::jucegui::components::MenuButton> patchName;
    std::unique_ptr<sst::jucegui::components::TextPushButton> midiMode, outBus, polyCount;
    std::unique_ptr<sst::jucegui::components::HSliderFilled> level, pan;
    std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue> tuning, transpose;
    std::unique_ptr<sst::jucegui::components::TextEditor> partBlurb;

    std::unique_ptr<juce::FileChooser> fileChooser;

    bool selfAccent{true};
    PartSidebarCard(int p, SCXTEditor *e);
    void paint(juce::Graphics &g) override;

    void showMidiModeMenu();
    void showRoutingMenu();
    void showPartIOMenu();
    void showPolyMenu();

    bool tallMode{false};
    void setTallMode(bool b)
    {
        tallMode = b;
        partBlurb->setVisible(tallMode);
    }

    void showPartBlurbTooltip();
    void hidePartBlurbTooltip();

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseEnter(const juce::MouseEvent &event) override { beginTimer(); }
    void mouseExit(const juce::MouseEvent &event) override { endTimer(); }

    void resized() override;

    void textEditorTextChanged(juce::TextEditor &) override;

    void resetFromEditorCache();

    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;
};
} // namespace scxt::ui::app::shared
#endif // SHORTCIRCUITXT_PARTSIDEBARCARD_H
