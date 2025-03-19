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

#ifndef SCXT_SRC_UI_APP_MISSING_RESOLUTION_MISSINGRESOLUTIONSCREEN_H
#define SCXT_SRC_UI_APP_MISSING_RESOLUTION_MISSINGRESOLUTIONSCREEN_H

#include "app/HasEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "engine/missing_resolution.h"
#include "sst/jucegui/components/NamedPanel.h"

namespace scxt::ui::app::missing_resolution
{

struct MissingResolutionScreen : juce::Component, HasEditor
{
    std::unique_ptr<sst::jucegui::components::NamedPanel> contentsArea;
    MissingResolutionScreen(SCXTEditor *e);
    ~MissingResolutionScreen();

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colour(0x90, 0x90, 0x90).withAlpha(0.3f));
    }

    void resized() override;
    void mouseUp(const juce::MouseEvent &) override { setVisible(false); }

    void setWorkItemList(const std::vector<engine::MissingResolutionWorkItem> &l);

    void resolveItem(int idx);
    void applyResolution(int idx, const fs::path &toThis);
    void applyDirectoryResolution(std::vector<engine::MissingResolutionWorkItem> indexes,
                                  const fs::path &newParent);

    std::unique_ptr<juce::FileChooser> fileChooser;

    std::vector<engine::MissingResolutionWorkItem> workItems;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MissingResolutionScreen);
};
} // namespace scxt::ui::app::missing_resolution
#endif // MISSINGRESOLUTIONSCREEN_H
