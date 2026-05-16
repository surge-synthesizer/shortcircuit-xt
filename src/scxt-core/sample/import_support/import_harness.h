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

#ifndef SCXT_SRC_SCXT_CORE_SAMPLE_IMPORT_SUPPORT_IMPORT_HARNESS_H
#define SCXT_SRC_SCXT_CORE_SAMPLE_IMPORT_SUPPORT_IMPORT_HARNESS_H

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "sample/sample_manager.h"

namespace scxt::import_support
{
// Per-import scaffolding. Constructed at the top of each format importer; owns
// the activity notification, the part-index clamp, group/zone bookkeeping, and
// the consolidated diagnostics that finish() emits.
class ImporterContext
{
  public:
    ImporterContext(engine::Engine &engine, const std::string &activityTitle);
    ~ImporterContext() = default;

    ImporterContext(const ImporterContext &) = delete;
    ImporterContext &operator=(const ImporterContext &) = delete;

    engine::Engine &getEngine() { return engineRef; }
    engine::Part &getPart() { return *partPtr; }
    int getPartIndex() const { return ptIdx; }

    // User-visible error, raised immediately via the engine's message controller.
    void raise(const std::string &title, const std::string &msg);
    // raise() + returns false — for the early-out idiom `return ctx.fail(...)`.
    bool fail(const std::string &title, const std::string &msg);

    // Programmer-level invariant violations (e.g. helper required-field missing).
    // Reported now via raise() under a "Software Error" title prefix; the marker
    // lets future UI filter these out of end-user-visible diagnostics.
    void software_error(const std::string &where, const std::string &what);

    // Soft, collected — consolidated and emitted in finish().
    void unsupported(const std::string &category, const std::string &detail);

    // Adds a new group to the part and tracks it. Returns the group index.
    int addGroup(const std::string &name = "");
    // Moves the zone into the named group and records its address for the
    // end-of-import selection push.
    void addZoneToGroup(int groupIdx, std::unique_ptr<engine::Zone> zone);

    // Loads a sample from disk for file-based formats (SFZ/EXS/AKP/etc.). Tries
    // each candidate path in order; for the first that exists, calls
    // SampleManager::loadSampleByPath. extensionFallbacks (if non-empty) are
    // appended to each candidate that doesn't exist as-given (e.g. AKP's
    // `.WAV/.wav/.AIF/.aif` search). Returns nullopt if nothing resolved.
    std::optional<SampleID>
    loadSampleFromDisk(std::initializer_list<fs::path> candidates,
                       std::initializer_list<const char *> extensionFallbacks = {});

    // Finalize: emits the unsupported-features summary, pushes a selection
    // action for everything added, and returns true iff ≥1 zone was added.
    // If nothing was added, raises a user-visible error and returns false.
    bool finish();

  private:
    engine::Engine &engineRef;
    int ptIdx{0};
    engine::Part *partPtr{nullptr};
    messaging::MessageController::ClientActivityNotificationGuard activityGuard;

    std::vector<int> addedGroups;
    std::vector<std::pair<int, int>> addedZoneAddresses; // (groupIdx, zoneIdx)
    std::vector<std::pair<std::string, std::string>> unsupportedItems;
};

// Looks up the SCXT group index for a native key (e.g. sf2::Instrument*, AKP group
// number). On first sight of a key, calls ctx.addGroup(nameFn()) and stores the
// result in the caller-supplied map. Returns the group index either way.
template <typename Map, typename Key, typename NameFn>
int getOrCreateGroup(ImporterContext &ctx, Map &map, const Key &key, NameFn &&nameFn)
{
    if (auto it = map.find(key); it != map.end())
        return it->second;
    int gid = ctx.addGroup(nameFn());
    map.emplace(key, gid);
    return gid;
}
} // namespace scxt::import_support

#endif
