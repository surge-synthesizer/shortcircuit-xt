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

#ifndef SCXT_SRC_UI_COMPONENTS_MULTI_MAPPINGPANE_H
#define SCXT_SRC_UI_COMPONENTS_MULTI_MAPPINGPANE_H

#include "sst/jucegui/components/NamedPanel.h"
#include "components/HasEditor.h"
#include "engine/zone.h"
#include "engine/part.h"
#include "selection/selection_manager.h"

namespace scxt::ui::multi
{
// Each of these are in the cpp file
struct MappingDisplay;
struct SampleDisplay;
struct MacroDisplay;

struct MappingPane : sst::jucegui::components::NamedPanel, HasEditor
{
    MappingPane(SCXTEditor *e);
    ~MappingPane();

    void resized() override;

    void setMappingData(const engine::Zone::ZoneMappingData &);
    void setSampleData(const engine::Zone::AssociatedSampleArray &);
    void setGroupZoneMappingSummary(const engine::Part::zoneMappingSummary_t &);
    void editorSelectionChanged();
    void setActive(bool b);

    std::unique_ptr<MappingDisplay> mappingDisplay;
    std::unique_ptr<SampleDisplay> sampleDisplay;
    std::unique_ptr<MacroDisplay> macroDisplay;

    engine::Zone::ZoneMappingData mappingView;
    engine::Zone::AssociatedSampleArray sampleView;

    void updateSamplePlaybackPosition(int64_t samplePos);

    void invertScroll(bool invert);
    bool invertScroll() const;
};
} // namespace scxt::ui::multi
#endif // SHORTCIRCUIT_MAPPINGPANE_H
