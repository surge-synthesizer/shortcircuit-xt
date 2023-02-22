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

#include "SCXTEditor.h"
#include "MultiScreen.h"
#include "multi/AdsrPane.h"
#include "multi/PartGroupSidebar.h"
#include "HeaderRegion.h"
#include "components/multi/MappingPane.h"

namespace scxt::ui
{
void SCXTEditor::onVoiceCount(const uint32_t &v)
{
    if (headerRegion)
        headerRegion->setVoiceCount(v);
}

void SCXTEditor::onEnvelopeUpdated(const int &which, const bool &active,
                                   const datamodel::AdsrStorage &v)
{
    if (active)
    {
        // TODO - do I want a multiScreen->onEnvelopeUpdated or just
        multiScreen->eg[which]->adsrChangedFromModel(v);
    }
    else
    {
        multiScreen->eg[which]->adsrDeactivated();
    }
}

void SCXTEditor::onMappingUpdated(const bool &active, const engine::Zone::ZoneMappingData &m)
{
    if (active)
    {
        multiScreen->sample->setMappingData(m);
    }
    else
    {
        // TODO
        // multiScreen->sample->deactivate();
    }
}

void SCXTEditor::onStructureUpdated(const engine::Engine::pgzStructure_t &s)
{
    if (multiScreen && multiScreen->parts)
        multiScreen->parts->setPartGroupZoneStructure(s);
}
} // namespace scxt::ui