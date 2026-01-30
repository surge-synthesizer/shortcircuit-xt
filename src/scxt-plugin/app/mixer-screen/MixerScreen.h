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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_MIXER_SCREEN_MIXERSCREEN_H
#define SCXT_SRC_SCXT_PLUGIN_APP_MIXER_SCREEN_MIXERSCREEN_H

#include "configuration.h"
#include "app/HasEditor.h"
#include "app/browser-ui/BrowserPane.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "engine/engine.h"
#include "datamodel/metadata.h"

namespace scxt::ui::app
{
namespace browser_ui
{
struct BrowserPane;
}
namespace shared
{
template <bool forBus> struct PartEffectsPane;
}
namespace mixer_screen
{

struct BusPane;
struct MixerScreen : juce::Component, HasEditor
{
    static constexpr int sideWidths = 196; // copied from multi for now

    MixerScreen(SCXTEditor *e);
    ~MixerScreen();
    void visibilityChanged() override;
    void resized() override;

    void selectBus(int index);

    void onBusEffectFullData(
        int bus, int slot,
        const std::array<datamodel::pmd, engine::BusEffectStorage::maxBusEffectParams> &,
        const engine::BusEffectStorage &);

    std::unique_ptr<browser_ui::BrowserPane> browser;
    std::array<std::unique_ptr<shared::PartEffectsPane<true>>, maxEffectsPerBus> partPanes;
    std::unique_ptr<mixer_screen::BusPane> busPane;

    using partFXMD_t = std::array<datamodel::pmd, engine::BusEffectStorage::maxBusEffectParams>;
    using partFXStorage_t = std::pair<partFXMD_t, engine::BusEffectStorage>;
    using busPartsFX_t = std::array<partFXStorage_t, engine::Bus::maxEffectsPerBus>;
    using busFX_t = std::array<busPartsFX_t, scxt::maxOutputs>;

    busFX_t busEffectsData;

    using busSend_t = std::array<engine::Bus::BusSendStorage, scxt::maxBusses>;
    busSend_t busSendData;

    void onBusSendData(int bus, const engine::Bus::BusSendStorage &s);

    void setFXSlotToType(int bus, int slot, engine::AvailableBusEffects t);
    void sendBusSendStorage(int bus);

    void swapEffects(int bus1, int slot1, int bus2, int slot2, bool swapVsMove = true);

    void setAllBussesToMain();
    void setAllBussesToUniqueOutput();

    void setVULevelForBusses(
        const std::array<std::array<std::atomic<float>, 2>, engine::Patch::Busses::busCount> &x);

    void onOtherTabSelection();

    void adjustChannelStripSoloMute();
};
} // namespace mixer_screen
} // namespace scxt::ui::app
#endif // SHORTCIRCUIT_SENDFXSCREEN_H
