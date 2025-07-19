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

#include "app/SCXTEditorDataCache.h"
#include "utils.h"

namespace scxt::ui::app
{

void SCXTEditorDataCache::addSubscription(void *el, sst::jucegui::data::Continuous *c)
{
    csubs[el].insert(c);
    c->addGUIDataListener(&clistener);
}
void SCXTEditorDataCache::addSubscription(void *el, sst::jucegui::data::Discrete *d)
{
    dsubs[el].insert(d);
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
            for (auto &d : ds)
            {
                int v = *((int *)da);
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