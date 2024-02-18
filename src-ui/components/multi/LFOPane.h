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

#ifndef SCXT_SRC_UI_COMPONENTS_MULTI_LFOPANE_H
#define SCXT_SRC_UI_COMPONENTS_MULTI_LFOPANE_H

#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/components/JogUpDownButton.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/Label.h"

#include "components/HasEditor.h"
#include "connectors/PayloadDataAttachment.h"
#include "modulation/modulators/steplfo.h"
#include "engine/zone.h"
#include <memory>

namespace scxt::ui::multi
{

struct StepLFOPane;
struct CurveLFOPane;
struct ENVLFOPane;
struct MSEGLFOPane;

struct LfoPane : sst::jucegui::components::NamedPanel, HasEditor
{
    typedef connectors::PayloadDataAttachment<modulation::ModulatorStorage> attachment_t;
    typedef connectors::DiscretePayloadDataAttachment<modulation::ModulatorStorage,
                                                      modulation::ModulatorStorage::ModulatorShape>
        shapeAttachment_t;
    typedef connectors::DiscretePayloadDataAttachment<modulation::ModulatorStorage,
                                                      modulation::ModulatorStorage::TriggerMode>
        triggerAttachment_t;

    typedef connectors::DiscretePayloadDataAttachment<modulation::ModulatorStorage, bool>
        boolBaseAttachment_t;
    typedef connectors::BooleanPayloadDataAttachment<modulation::ModulatorStorage> boolAttachment_t;

    typedef connectors::DiscretePayloadDataAttachment<modulation::ModulatorStorage, int16_t>
        int16Attachment_t;

    LfoPane(SCXTEditor *, bool forZone);
    ~LfoPane();

    std::unique_ptr<StepLFOPane> stepLfoPane;
    std::unique_ptr<CurveLFOPane> curveLfoPane;
    std::unique_ptr<ENVLFOPane> envLfoPane;
    std::unique_ptr<MSEGLFOPane> msegLfoPane;

    bool forZone{true};

    void tabChanged(int i);

    void setSubPaneVisibility();

    void resized() override;
    void rebuildPanelComponents(); // entirely new components

    void setActive(int index, bool active);
    void setModulatorStorage(int index, const modulation::ModulatorStorage &mod);

    void repositionContentAreaComponents();

    std::unique_ptr<shapeAttachment_t> modulatorShapeA;
    std::unique_ptr<sst::jucegui::components::JogUpDownButton> modulatorShape;

    std::unique_ptr<triggerAttachment_t> triggerModeA;
    std::unique_ptr<sst::jucegui::components::MultiSwitch> triggerMode;

    std::array<modulation::ModulatorStorage, engine::lfosPerZone> modulatorStorageData;

    void pickPresets();
};
} // namespace scxt::ui::multi

#endif // SHORTCIRCUIT_LFOPANE_H
