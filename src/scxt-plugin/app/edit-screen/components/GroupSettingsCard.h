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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_GROUPSETTINGSCARD_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_GROUPSETTINGSCARD_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/TextPushButton.h"

#include "messaging/messaging.h"
#include "connectors/PayloadDataAttachment.h"

#include "app/HasEditor.h"

namespace scxt::ui::app::edit_screen
{
struct GroupSettingsCard : juce::Component, HasEditor
{
    GroupSettingsCard(SCXTEditor *e);
    void paint(juce::Graphics &g) override;
    void resized() override;
    void rebuildFromInfo();

    std::unique_ptr<sst::jucegui::components::GlyphPainter> midiGlyph, outputGlyph, polyGlygh,
        prioGlyph, glideGlpyh, volGlyph, panGlyph, tuneGlyph;
    std::unique_ptr<sst::jucegui::components::Label> pbLabel, SRCLabel;
    std::unique_ptr<sst::jucegui::components::TextPushButton> polyMenu, polyModeMenu;
    std::unique_ptr<sst::jucegui::components::TextPushButton> midiMenu, outputMenu, prioMenu,
        glideMenu, srcMenu;
    std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue> pbDnVal, pbUpVal,
        glideDrag, volDrag, panDrag, tuneDrag;

    using floatMsg_t = scxt::messaging::client::UpdateGroupOutputFloatValue;
    using intMsg_t = scxt::messaging::client::UpdateGroupOutputInt16TValue;
    typedef connectors::PayloadDataAttachment<engine::Group::GroupOutputInfo> attachment_t;
    typedef connectors::PayloadDataAttachment<engine::Group::GroupOutputInfo, int16_t>
        iattachment_t;

    std::unique_ptr<attachment_t> volAttachment, panAttachment, tuneAttachment;
    std::unique_ptr<iattachment_t> pbDnA, pbUpA;

    engine::Group::GroupOutputInfo &info;

    void showPolyMenu();
    void showPolyModeMenu();
    void showNotePrioMenu();
    void showMidiChannelMenu();
};
} // namespace scxt::ui::app::edit_screen
#endif // GROUPSETTINGSCARD_H
