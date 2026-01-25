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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_ADSRPANE_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_ADSRPANE_H

#include <unordered_map>
#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/data/Continuous.h"
#include "app/HasEditor.h"
#include "connectors/PayloadDataAttachment.h"

#include "messaging/messaging.h"

namespace scxt::ui::app::edit_screen
{
struct AdsrPane : sst::jucegui::components::NamedPanel, HasEditor
{
    typedef connectors::PayloadDataAttachment<modulation::modulators::AdsrStorage> attachment_t;
    typedef connectors::BooleanPayloadDataAttachment<modulation::modulators::AdsrStorage>
        boolAttachment_t;

    template <typename T> struct UIStore
    {
        std::array<std::unique_ptr<T>, 9> members;
        std::unique_ptr<T> &A{members[0]}, &H{members[1]}, &D{members[2]}, &S{members[3]},
            &R{members[4]}, &Ash{members[5]}, &Dsh{members[6]}, &Rsh{members[7]}, &dly{members[8]};
    };

    UIStore<attachment_t> attachments;
    UIStore<sst::jucegui::components::VSlider> sliders;
    UIStore<sst::jucegui::components::Label> labels;
    UIStore<sst::jucegui::components::Knob> knobs;

    // used by group
    std::unique_ptr<sst::jucegui::components::ToggleButton> gateToggle;
    std::unique_ptr<boolAttachment_t> gateToggleA;

    modulation::modulators::AdsrStorage adsrView;
    std::array<modulation::modulators::AdsrStorage, scxt::egsPerZone - 1> zoneAdsrCache;
    size_t displayedTabIndex{0};
    size_t index{0};
    bool forZone{true};
    AdsrPane(SCXTEditor *, int index, bool forZone);

    void resized() override;

    void adsrChangedFromModel(const modulation::modulators::AdsrStorage &);
    void adsrChangedFromModel(const modulation::modulators::AdsrStorage &, int index);
    void adsrDeactivated();
    void tabChanged(int newIndex, bool updateState);

    void rebuildPanelComponents(int newIndex);

    void showHamburgerMenu();
};
} // namespace scxt::ui::app::edit_screen

#endif // SHORTCIRCUIT_ADSRPANE_H
