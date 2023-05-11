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

#ifndef SCXT_SRC_UI_COMPONENTS_MIXERSCREEN_H
#define SCXT_SRC_UI_COMPONENTS_MIXERSCREEN_H

#include "HasEditor.h"
#include "SCXTEditor.h"
#include "browser/BrowserPane.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace scxt::ui
{
namespace browser
{
struct BrowserPane;
}
namespace mixer
{
struct PartEffectsPane;
struct BusPane;
} // namespace mixer
struct MixerScreen : juce::Component, HasEditor
{
    static constexpr int sideWidths = 196; // copied from multi for now

    MixerScreen(SCXTEditor *e);
    ~MixerScreen();
    void paint(juce::Graphics &g) override
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(60, juce::Font::plain));
        g.drawText("Mixer", getLocalBounds().withTrimmedBottom(40), juce::Justification::centred);
        g.setFont(juce::Font(20, juce::Font::plain));
        g.drawText("Coming Soon", getLocalBounds().withTrimmedTop(40),
                   juce::Justification::centred);
    }
    void visibilityChanged() override;
    void resized() override;

    void onBusEffectFullData(
        int bus, int slot,
        const std::array<datamodel::pmd, engine::BusEffectStorage::maxBusEffectParams> &,
        const engine::BusEffectStorage &);

    std::unique_ptr<browser::BrowserPane> browser;
    std::array<std::unique_ptr<mixer::PartEffectsPane>, engine::Bus::maxEffectsPerBus> partPanes;
    std::unique_ptr<mixer::BusPane> busPane;

    using partFXMD_t = std::array<datamodel::pmd, engine::BusEffectStorage::maxBusEffectParams>;
    using partFXStorage_t = std::pair<partFXMD_t, engine::BusEffectStorage>;
    using busPartsFX_t = std::array<partFXStorage_t, engine::Bus::maxEffectsPerBus>;
    using busFX_t = std::array<busPartsFX_t, scxt::maxOutputs>;

    busFX_t busEffectsData;
};
} // namespace scxt::ui
#endif // SHORTCIRCUIT_SENDFXSCREEN_H
