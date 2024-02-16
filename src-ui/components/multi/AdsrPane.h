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

#ifndef SCXT_SRC_UI_COMPONENTS_MULTI_ADSRPANE_H
#define SCXT_SRC_UI_COMPONENTS_MULTI_ADSRPANE_H

#include <unordered_map>
#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/data/Continuous.h"
#include "datamodel/adsr_storage.h"
#include "components/HasEditor.h"
#include "connectors/PayloadDataAttachment.h"

#include "messaging/messaging.h"

namespace scxt::ui::multi
{
struct AdsrZoneTraits
{
    using floatMsg_t = scxt::messaging::client::UpdateZoneEGFloatValue;
};
struct AdsrGroupTraits
{
    using floatMsg_t = scxt::messaging::client::UpdateGroupEGFloatValue;
};

template <typename GZTrait> struct AdsrPane : sst::jucegui::components::NamedPanel, HasEditor
{
    typedef connectors::PayloadDataAttachment<datamodel::AdsrStorage> attachment_t;

    template <typename T> struct UIStore
    {
        std::array<std::unique_ptr<T>, 8> members;
        std::unique_ptr<T> &A{members[0]}, &H{members[1]}, &D{members[2]}, &S{members[3]},
            &R{members[4]}, &Ash{members[5]}, &Dsh{members[6]}, &Rsh{members[7]};
    };

    UIStore<attachment_t> attachments;
    UIStore<sst::jucegui::components::VSlider> sliders;
    UIStore<sst::jucegui::components::Label> labels;
    UIStore<sst::jucegui::components::Knob> knobs;

    datamodel::AdsrStorage adsrView;
    size_t index{0};
    bool forZone{true};
    AdsrPane(SCXTEditor *, int index, bool forZone);

    void resized() override;

    void adsrChangedFromModel(const datamodel::AdsrStorage &);
    void adsrDeactivated();

    void showHamburgerMenu();
};
} // namespace scxt::ui::multi

#endif // SHORTCIRCUIT_ADSRPANE_H
