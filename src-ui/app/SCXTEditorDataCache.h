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

#ifndef SCXT_SRC_UI_APP_SCXTEDITORDATACACHE_H
#define SCXT_SRC_UI_APP_SCXTEDITORDATACACHE_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

#include "engine/bus.h"
#include "engine/part.h"
#include "engine/group.h"
#include "engine/zone.h"
#include "engine/patch.h"
#include "sample/compound_file.h"

#include "sst/jucegui/data/Continuous.h"
#include "sst/jucegui/data/Discrete.h"

namespace scxt::ui::app
{
/**
 * The iriginal design of the SC UI (which is still partly that way) has local copies
 * of the data cache right next to the particular panel. This works well until you want
 * to share data across more than one panel, and then it gets clumsy, or if your heirarchy
 * gets deep.
 *
 * So this class exists to have a place for the SCXTEditorResonder thingy to push data
 * copies and for hte UI to bind to, since the UI will reliably be components which
 * haveeditor.
 */
struct SCXTEditorDataCache
{
    // Displayed group tab output info
    scxt::engine::Group::GroupOutputInfo groupOutputInfo;
    // Displayed zone tab output info
    scxt::engine::Zone::ZoneOutputInfo zoneOutputInfo;

    void addSubscription(void *el, sst::jucegui::data::Continuous *);
    void addSubscription(void *el, sst::jucegui::data::Discrete *);
    void fireSubscription(void *el, sst::jucegui::data::Continuous *);
    void fireSubscription(void *el, sst::jucegui::data::Discrete *);

    void addNotificationCallback(void *el, std::function<void()>);
    template <typename P> void fireAllNotificationsFor(const P &p)
    {
        auto st = &p;
        auto end = &p + sizeof(P);
        fireAllNotificationsBetween((void *)st, (void *)end);
    }

  private:
    void fireAllNotificationsBetween(void *st, void *end);
    std::unordered_map<void *, std::unordered_set<sst::jucegui::data::Continuous *>> csubs;
    std::unordered_map<void *, std::unordered_set<sst::jucegui::data::Discrete *>> dsubs;
    std::unordered_map<void *, std::vector<std::function<void()>>> fsubs;

    struct CListener : sst::jucegui::data::Continuous::DataListener
    {
        SCXTEditorDataCache &cache;
        CListener(SCXTEditorDataCache &dc) : cache(dc){};
        virtual ~CListener() = default;
        void dataChanged() override{};
        void sourceVanished(sst::jucegui::data::Continuous *c) override;
    } clistener{*this};

    struct DListener : sst::jucegui::data::Discrete::DataListener
    {
        SCXTEditorDataCache &cache;
        DListener(SCXTEditorDataCache &dc) : cache(dc){};
        virtual ~DListener() = default;
        void dataChanged() override{};
        void sourceVanished(sst::jucegui::data::Discrete *c) override;
    } dlistener{*this};
};

} // namespace scxt::ui::app

#endif // SCXTEDITORDATACACHE_H
