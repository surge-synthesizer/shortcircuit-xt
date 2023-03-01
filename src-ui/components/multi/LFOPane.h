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

#ifndef SHORTCIRCUIT_LFOPANE_H
#define SHORTCIRCUIT_LFOPANE_H

#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/Label.h"

#include "components/HasEditor.h"
#include "connectors/PayloadDataAttachment.h"
#include "modulation/modulators/steplfo.h"
#include "engine/zone.h"

namespace scxt::ui::multi
{
struct LfoPane : sst::jucegui::components::NamedPanel, HasEditor
{
    typedef connectors::PayloadDataAttachment<LfoPane, modulation::modulators::StepLFOStorage>
        attachment_t;
    typedef connectors::DiscretePayloadDataAttachment<LfoPane,
                                                      modulation::modulators::StepLFOStorage>
        intAttachment_t;
    typedef connectors::BooleanPayloadDataAttachment<LfoPane,
                                                     modulation::modulators::StepLFOStorage>
        boolAttachment_t;

    LfoPane(SCXTEditor *);
    ~LfoPane();

    void tabChanged(int i);

    void resized() override;
    void rebuildLfo(); // entirely new components
    void resetAllComponents();

    void setActive(int index, bool active);
    void setLfo(int index, const modulation::modulators::StepLFOStorage &);

    std::unique_ptr<sst::jucegui::components::ToggleButton> oneshotB, tempoSyncB, cycleB;
    std::unique_ptr<boolAttachment_t> oneshotA, tempoSyncA, cycleA;

    std::unique_ptr<sst::jucegui::components::Knob> rateK, deformK, stepsK;
    std::unique_ptr<attachment_t> rateA, deformA, stepsA;


    std::array<modulation::modulators::StepLFOStorage, engine::lfosPerZone> lfoData;

    void pushCurrentLfoUpdate();
    void pickPresets();
};
} // namespace scxt::ui::multi

#endif // SHORTCIRCUIT_LFOPANE_H
