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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_PARTEDITSCREEN_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_PARTEDITSCREEN_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "app/HasEditor.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "configuration.h"
#include "engine/bus_effect.h"

namespace scxt::ui::app
{
namespace shared
{
template <bool forBus> struct PartEffectsPane;
}
namespace edit_screen
{
struct MacroDisplay;
struct PartSettingsDisplay;

struct PartEditScreen : juce::Component, HasEditor
{
    PartEditScreen(HasEditor *e);
    ~PartEditScreen();

    void resized() override;
    void selectedPartChanged();
    void macroDataChanged(int part, int index);

    std::unique_ptr<MacroDisplay> macroDisplay;
    std::unique_ptr<PartSettingsDisplay> partSettingsDisplay;
    std::unique_ptr<sst::jucegui::components::NamedPanel> topPanel;

    void setFXSlotToType(int part, int slot, engine::AvailableBusEffects t);

    std::array<std::unique_ptr<shared::PartEffectsPane<false>>, maxEffectsPerPart> partPanes;

    using partFXMD_t = std::array<datamodel::pmd, engine::BusEffectStorage::maxBusEffectParams>;
    using partFXStorage_t = std::pair<partFXMD_t, engine::BusEffectStorage>;
    using partPartsFX_t = std::array<partFXStorage_t, maxEffectsPerPart>;
    using partFX_t = std::array<partPartsFX_t, scxt::numParts>;

    partFX_t partsEffectsData;

    void onPartEffectFullData(
        int part, int slot,
        const std::array<datamodel::pmd, engine::BusEffectStorage::maxBusEffectParams> &pmd,
        const engine::BusEffectStorage &bes);

    void swapEffects(int bus1, int slot1, int bus2, int slot2, bool swapVsMove = true);
};
} // namespace edit_screen
} // namespace scxt::ui::app
#endif // PARTPANE_H
