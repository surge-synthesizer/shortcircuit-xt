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

#ifndef SCXT_SRC_UI_COMPONENTS_HEADERREGION_H
#define SCXT_SRC_UI_COMPONENTS_HEADERREGION_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/components/ToggleButton.h>
#include <sst/jucegui/components/TextPushButton.h>
#include <sst/jucegui/components/MenuButton.h>
#include <sst/jucegui/components/GlyphButton.h>
#include <sst/jucegui/components/Label.h>
#include <sst/jucegui/components/VUMeter.h>
#include <fmt/core.h>

#include <version.h>

#include "sst/jucegui/components/ToggleButtonRadioGroup.h"
#include "sst/jucegui/data/Discrete.h"
#include "HasEditor.h"

namespace scxt::ui
{
struct SCXTEditor;

struct HeaderRegion : juce::Component, HasEditor, juce::FileDragAndDropTarget
{
    std::unique_ptr<sst::jucegui::components::ToggleButtonRadioGroup> selectedPage;
    std::unique_ptr<sst::jucegui::data::Discrete> selectedPageData;
    std::unique_ptr<sst::jucegui::components::VUMeter> vuMeter;
    std::unique_ptr<sst::jucegui::components::TextPushButton> undoButton, redoButton, tuningButton,
        zoomButton;
    std::unique_ptr<sst::jucegui::components::Label> cpuLevel, ramLevel;
    std::unique_ptr<sst::jucegui::components::Label> cpuLabel, ramLabel;

    std::unique_ptr<sst::jucegui::components::GlyphButton> chipButton, saveAsButton, scMenu;
    std::unique_ptr<sst::jucegui::components::MenuButton> multiMenuButton;

    HeaderRegion(SCXTEditor *);
    ~HeaderRegion();

    void resized() override;

    uint32_t voiceCount{0};
    void setVoiceCount(uint32_t vc)
    {
        if (vc != voiceCount)
        {
            voiceCount = vc;
            repaint();
        }
    }

    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;

    float memUsageInMegabytes{0.f};
    void setMemUsage(float m)
    {
        if (m != memUsageInMegabytes)
        {
            memUsageInMegabytes = m;
            ramLevel->setText(fmt::format("{:.0f} MB", m));
        }
    }

    float vuLevel[2]{0, 0};
    void setVULevel(float L, float R);

    float cpuLevValue{-100};
    void setCPULevel(float);

    void showSaveMenu();
    void doSaveMulti();
    void doLoadMulti();

    std::unique_ptr<juce::FileChooser> fileChooser;
};
} // namespace scxt::ui

#endif // SHORTCIRCUIT_HEADERREGION_H
