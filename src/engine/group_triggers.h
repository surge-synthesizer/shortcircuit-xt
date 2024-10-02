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

#ifndef SCXT_SRC_ENGINE_GROUP_TRIGGERS_H
#define SCXT_SRC_ENGINE_GROUP_TRIGGERS_H

#include <memory>

#include "datamodel/metadata.h"

namespace scxt::engine
{

struct Group;

/**
 * Maintain the state of keys ccs etc... for a particular instrument
 */
struct GroupTriggerInstrumentState
{
};

/**
 * Calculate if a group is triggered
 */
struct GroupTrigger
{
    GroupTriggerInstrumentState &state;
    GroupTrigger(GroupTriggerInstrumentState &onState) : state(onState) {}
    virtual ~GroupTrigger() = default;
    virtual bool value(const std::unique_ptr<Group> &) const = 0;

    datamodel::pmd argMetadata(int argNo) const;
    std::string displayName;
};

std::unique_ptr<GroupTrigger> makeMacroGroupTrigger(GroupTriggerInstrumentState &);
std::unique_ptr<GroupTrigger> makeMIDI1CCGroupTrigger(GroupTriggerInstrumentState &);

} // namespace scxt::engine
#endif // GROUP_TRIGGERS_H
