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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_SHARED_HEADERREGION_H
#define SCXT_SRC_SCXT_PLUGIN_APP_SHARED_HEADERREGION_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/components/ToggleButton.h>
#include <sst/jucegui/components/TextPushButton.h>
#include <sst/jucegui/components/MenuButton.h>
#include <sst/jucegui/components/GlyphButton.h>
#include <sst/jucegui/components/Label.h>
#include <sst/jucegui/components/VUMeter.h>
#include <fmt/core.h>

#include "sst/jucegui/components/ToggleButtonRadioGroup.h"
#include "sst/jucegui/data/Discrete.h"
#include "patch_io/patch_io.h"
#include "app/HasEditor.h"
#include "utils.h"

namespace scxt::ui::app::shared
{

struct ActivityDisplay;

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
    std::unique_ptr<ActivityDisplay> activityDisplay;

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

    float memUsageInBytes{-1.f};
    void setMemUsage(float m)
    {
        if (m != memUsageInBytes)
        {
            memUsageInBytes = m;
            auto mb = memUsageInBytes / 1024.f / 1024.f;
            if (mb < 10)
            {
                ramLevel->setText(fmt::format("{:.2f} MB", mb));
            }
            else if (mb < 100)
            {
                ramLevel->setText(fmt::format("{:.1f} MB", mb));
            }
            else
            {
                ramLevel->setText(fmt::format("{:.0f} MB", mb));
            }
        }
    }

    float vuLevel[2]{0, 0};
    void setVULevel(float L, float R);

    float cpuLevValue{-100};
    void setCPULevel(float);

    void showSaveMenu();
    void doSaveMulti(patch_io::SaveStyles style);
    void doLoadMulti();
    void doSaveSelectedPart(patch_io::SaveStyles style);
    void doLoadIntoSelectedPart();

    void showMultiSelectionMenu();

    void addResetMenuItems(juce::PopupMenu &menu);

    void onActivityNotification(int idx, const std::string &msg);

    std::unique_ptr<juce::FileChooser> fileChooser;
};
} // namespace scxt::ui::app::shared

#endif // SHORTCIRCUIT_HEADERREGION_H
