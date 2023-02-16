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

#ifndef SHORTCIRCUIT_ADSRPANE_H
#define SHORTCIRCUIT_ADSRPANE_H

#include <unordered_map>
#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/data/Continuous.h"
#include "datamodel/adsr_storage.h"
#include "components/HasEditor.h"
#include "connectors/PayloadDataAttachment.h"

namespace scxt::ui::multi
{
struct AdsrPane : sst::jucegui::components::NamedPanel, HasEditor
{
    // ToDo: shapes of course
    typedef connectors::PayloadDataAttachment<AdsrPane, datamodel::AdsrStorage> attachment_t;

    enum Ctrl
    {
        A,
        H,
        D,
        S,
        R,
        Ash,
        Dsh,
        Rsh
    };

    std::unordered_map<Ctrl, std::unique_ptr<attachment_t>> attachments;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::VSlider>> sliders;
    std::unordered_map<Ctrl, std::unique_ptr<juce::Component>> labels;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::Knob>> knobs;

    datamodel::AdsrStorage adsrView;
    int index{0};
    AdsrPane(SCXTEditor *, int index);

    void resized() override;

    void adsrChangedFromModel(const datamodel::AdsrStorage &);
    void adsrDeactivated();
    void adsrChangedFromGui();
};
} // namespace scxt::ui::multi

#endif // SHORTCIRCUIT_ADSRPANE_H
