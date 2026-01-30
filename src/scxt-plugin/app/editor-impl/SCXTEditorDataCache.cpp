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

#include <cassert>
#include "app/SCXTEditorDataCache.h"
#include "utils.h"

namespace scxt::ui::app
{

void SCXTEditorDataCache::addSubscription(void *el, size_t soel, sst::jucegui::data::Continuous *c)
{
    if (soel == sizeof(float))
    {
        csubs[el].insert(c);
        c->addGUIDataListener(&clistener);
    }
}
void SCXTEditorDataCache::addSubscription(void *el, size_t soel, sst::jucegui::data::Discrete *d)
{
    dsubs[el].insert(d);
    if (dsizes.find(el) != dsizes.end())
    {
        if (dsizes[el] != soel)
        {
            SCLOG_IF(warnings, "Configuration in add subscription varied size " << soel);
        }
    }
    dsizes[el] = soel;
    d->addGUIDataListener(&dlistener);
}

void SCXTEditorDataCache::fireSubscription(void *el, sst::jucegui::data::Continuous *c)
{
    auto cp = csubs.find(el);
    if (cp != csubs.end())
    {
        for (auto &ca : cp->second)
        {
            if (ca != c)
            {
                ca->setValueFromModel(c->getValue());
            }
        }
    }
}

void SCXTEditorDataCache::fireSubscription(void *el, sst::jucegui::data::Discrete *c)
{
    auto cp = dsubs.find(el);
    if (cp != dsubs.end())
    {
        for (auto &ca : cp->second)
        {
            if (ca != c)
            {
                ca->setValueFromModel(c->getValue());
            }
        }
    }
}

void SCXTEditorDataCache::CListener::sourceVanished(sst::jucegui::data::Continuous *c)
{
    for (auto &cs : cache.csubs)
    {
        auto p = cs.second.find(c);
        if (p != cs.second.end())
            cs.second.erase(p);
    }
}

void SCXTEditorDataCache::DListener::sourceVanished(sst::jucegui::data::Discrete *c)
{
    for (auto &cs : cache.dsubs)
    {
        auto p = cs.second.find(c);
        if (p != cs.second.end())
            cs.second.erase(p);
    }
}

void SCXTEditorDataCache::addNotificationCallback(void *el, std::function<void()> f)
{
    fsubs[el].push_back(f);
}

void SCXTEditorDataCache::fireAllNotificationsBetween(void *st, void *end)
{
    for (auto &[da, ds] : fsubs)
    {
        if (da >= st && da <= end)
        {
            for (auto &d : ds)
            {
                d();
            }
        }
    }

    for (auto &[da, ds] : dsubs)
    {
        if (da >= st && da <= end)
        {
            auto dsp = dsizes.find(da);
            size_t size{4};
            if (dsp == dsizes.end())
            {
                SCLOG_IF(warnings, "Cant locate data size for " << da);
            }
            else
            {
                size = dsp->second;
            }

            for (auto &d : ds)
            {
                int v{0};
                switch (size)
                {
                case 1:
                    v = *((char *)da);
                    break;
                case 2:
                    v = *((int16_t *)da);
                    break;
                case 8:
                    v = *((int64_t *)da);
                    break;
                case 4:
                default:
                    v = *((int32_t *)da);
                    break;
                }

                d->setValueFromModel(v);
            }
        }
    }

    for (auto &[da, ds] : csubs)
    {
        if (da >= st && da <= end)
        {
            for (auto &d : ds)
            {
                d->setValueFromModel(*((float *)da));
            }
        }
    }
}

} // namespace scxt::ui::app