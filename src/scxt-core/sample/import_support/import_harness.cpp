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

#include "import_harness.h"
#include "selection/selection_manager.h"
#include "configuration.h"
#include "utils.h"

#include <cassert>
#include <sstream>

namespace scxt::import_support
{
ImporterContext::ImporterContext(engine::Engine &eng, const std::string &activityTitle)
    : engineRef(eng), activityGuard(activityTitle, *eng.getMessageController())
{
    assert(eng.getMessageController()->threadingChecker.isSerialThread());
    ptIdx =
        std::clamp(eng.getSelectionManager()->selectedPart, (int16_t)0, (int16_t)(numParts - 1));
    partPtr = eng.getPatch()->getPart(ptIdx).get();
}

void ImporterContext::raise(const std::string &title, const std::string &msg)
{
    RAISE_ERROR_ENGINE(engineRef, title, msg);
}

bool ImporterContext::fail(const std::string &title, const std::string &msg)
{
    raise(title, msg);
    return false;
}

void ImporterContext::software_error(const std::string &where, const std::string &what)
{
    SCLOG_IF(warnings, "Software error in " << where << ": " << what);
    raise("Software Error", where + ": " + what);
}

void ImporterContext::unsupported(const std::string &category, const std::string &detail)
{
    unsupportedItems.emplace_back(category, detail);
    SCLOG_IF(sampleCompoundParsers, "Unsupported " << category << ": " << detail);
}

int ImporterContext::addGroup(const std::string &name)
{
    int idx = partPtr->addGroup() - 1;
    if (!name.empty())
        partPtr->getGroup(idx)->name = name;
    addedGroups.push_back(idx);
    return idx;
}

void ImporterContext::addZoneToGroup(int groupIdx, std::unique_ptr<engine::Zone> zone)
{
    auto &group = partPtr->getGroup(groupIdx);
    int zoneIdx = (int)group->getZones().size();
    group->addZone(zone);
    addedZoneAddresses.emplace_back(groupIdx, zoneIdx);
}

std::optional<SampleID>
ImporterContext::loadSampleFromDisk(std::initializer_list<fs::path> candidates,
                                    std::initializer_list<const char *> extensionFallbacks)
{
    auto &sm = *engineRef.getSampleManager();
    for (const auto &candidate : candidates)
    {
        if (fs::exists(candidate))
        {
            if (auto sid = sm.loadSampleByPath(candidate))
                return sid;
        }
        for (auto ext : extensionFallbacks)
        {
            auto withExt = fs::path{candidate.u8string() + ext};
            if (fs::exists(withExt))
            {
                if (auto sid = sm.loadSampleByPath(withExt))
                    return sid;
            }
        }
    }
    return std::nullopt;
}

bool ImporterContext::finish()
{
    if (!unsupportedItems.empty())
    {
        std::ostringstream oss;
        const char *pfx = "";
        for (auto &[cat, det] : unsupportedItems)
        {
            oss << pfx << cat << ": " << det;
            pfx = "; ";
        }
        SCLOG_IF(sampleCompoundParsers, "Unsupported features in this import: " << oss.str());
    }

    if (addedZoneAddresses.empty())
    {
        raise("Import produced no zones", "The importer ran without adding any zones to the part.");
        return false;
    }

    auto [firstGroup, firstZone] = addedZoneAddresses.front();
    engineRef.getSelectionManager()->applySelectActions(
        selection::SelectionManager::SelectActionContents(ptIdx, firstGroup, firstZone, true, true,
                                                          true));
    return true;
}
} // namespace scxt::import_support
