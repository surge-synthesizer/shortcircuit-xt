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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_GROUPSETTINGSCARD_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_GROUPSETTINGSCARD_H

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
    std::unique_ptr<sst::jucegui::components::MenuButton> midiMenu, outputMenu, prioMenu, glideMenu,
        srcMenu;
    std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue> pbDnVal, pbUpDrag,
        glideDrag, volDrag, panDrag, tuneDrag;

    using floatMsg_t = scxt::messaging::client::UpdateGroupOutputFloatValue;
    typedef connectors::PayloadDataAttachment<engine::Group::GroupOutputInfo> attachment_t;

    std::unique_ptr<attachment_t> volAttachment, panAttachment;

    engine::Group::GroupOutputInfo &info;

    void showPolyMenu();
    void showPolyModeMenu();
};
} // namespace scxt::ui::app::edit_screen
#endif // GROUPSETTINGSCARD_H
