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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_PARTEDITSCREEN_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_PARTEDITSCREEN_H

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
