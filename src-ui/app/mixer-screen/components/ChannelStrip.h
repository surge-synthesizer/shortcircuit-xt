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

#ifndef SCXT_SRC_UI_APP_MIXER_SCREEN_COMPONENTS_CHANNELSTRIP_H
#define SCXT_SRC_UI_APP_MIXER_SCREEN_COMPONENTS_CHANNELSTRIP_H

#include "engine/bus.h"
#include "app/HasEditor.h"
#include "app/mixer-screen/MixerScreen.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/VUMeter.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/GlyphButton.h"

#include "connectors/PayloadDataAttachment.h"

namespace scxt::ui::app::mixer_screen
{
struct ChannelStrip : public HasEditor, sst::jucegui::components::NamedPanel
{
    using attachment_t = scxt::ui::connectors::PayloadDataAttachment<engine::Bus::BusSendStorage>;
    using boolattachment_t =
        scxt::ui::connectors::BooleanPayloadDataAttachment<engine::Bus::BusSendStorage>;

    MixerScreen *mixer{nullptr};
    int16_t busIndex{-1};
    enum struct BusType
    {
        PART,
        AUX,
        MAIN
    } type{BusType::PART};
    ChannelStrip(SCXTEditor *e, MixerScreen *m, int busIndex, BusType type);
    ~ChannelStrip();

    void resized() override;

    std::array<std::unique_ptr<sst::jucegui::components::MenuButton>,
               scxt::engine::Bus::maxEffectsPerBus>
        fxMenu;
    std::unique_ptr<sst::jucegui::components::Label> fxLabel;
    std::array<std::unique_ptr<sst::jucegui::components::ToggleButton>,
               scxt::engine::Bus::maxEffectsPerBus>
        fxToggle;
    std::array<std::unique_ptr<boolattachment_t>, scxt::engine::Bus::maxEffectsPerBus> fxToggleAtt;

    std::array<std::unique_ptr<sst::jucegui::components::HSlider>, scxt::numAux> auxSlider;
    std::array<std::unique_ptr<attachment_t>, scxt::numAux> auxAttachments;
    std::unique_ptr<sst::jucegui::components::Label> auxLabel;
    std::array<std::unique_ptr<sst::jucegui::components::ToggleButton>, scxt::numAux> auxToggle;
    std::array<std::unique_ptr<sst::jucegui::components::GlyphButton>, scxt::numAux> auxPrePost;

    std::unique_ptr<attachment_t> vcaAttachment;
    std::unique_ptr<sst::jucegui::components::VSlider> vcaSlider;

    std::unique_ptr<attachment_t> panAttachment;
    std::unique_ptr<sst::jucegui::components::Knob> panKnob;

    std::unique_ptr<boolattachment_t> soloAtt, muteAtt;
    std::unique_ptr<sst::jucegui::components::ToggleButton> soloButton, muteButton;
    std::unique_ptr<sst::jucegui::components::MenuButton> outputMenu;

    void mouseDown(const juce::MouseEvent &) override;
    void showAuxRouting(int idx);
    void resetAuxRoutingGlyph(int idx);
    void labelPluginOutput();
    void showPluginOutput();

    void onDataChanged()
    {
        labelPluginOutput();
        for (int i = 0; i < numAux; ++i)
        {
            if (auxPrePost[i])
                resetAuxRoutingGlyph(i);
        }

        repaint();
    }

    void effectsChanged();
    float vuL{0.f}, vuR{0.f};

    std::unique_ptr<sst::jucegui::components::VUMeter> vuMeter;
    void setVULevel(float L, float R)
    {
        if (vuL != L || vuR != R)
        {
            vuMeter->setLevels(L, R);
        }
        vuL = L;
        vuR = R;
    }
};
} // namespace scxt::ui::app::mixer_screen
#endif // SHORTCIRCUITXT_CHANNELSTRIP_H
