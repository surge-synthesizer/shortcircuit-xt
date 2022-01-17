//
// Created by Paul Walker on 1/13/22.
//

#ifndef SHORTCIRCUIT_ZONELISTDATAPROXY_H
#define SHORTCIRCUIT_ZONELISTDATAPROXY_H

#include "SCXTEditor.h"

namespace scxt
{
namespace proxies
{
struct ZoneListDataProxy : public scxt::data::UIStateProxy
{
    SCXTEditor *editor{nullptr};
    ZoneListDataProxy(SCXTEditor *ed) : editor(ed){};
    virtual bool processActionData(const actiondata &ad)
    {
        auto g = InvalidateAndRepaintGuard(*this);
        if (ad.id != ip_kgv_or_list)
        {
            return g.deactivate();
        }

        if (!std::holds_alternative<VAction>(ad.actiontype))
            return g.deactivate();

        switch (std::get<VAction>(ad.actiontype))
        {
        case vga_zonelist_clear:
        {
            for (int i = 0; i < max_zones; ++i)
                editor->activeZones[i] = false;
            break;
        }
        case vga_zonelist_populate:
        {
            ad_zonedata *zd = (ad_zonedata *)&ad;

            sample_zone *sz = &editor->zonesCopy[zd->zid];
            editor->activeZones[zd->zid] = true;

            sz->part = zd->part;
            sz->layer = zd->layer;
            sz->key_low = zd->keylo;
            sz->key_low_fade = zd->keylofade;
            sz->key_high = zd->keyhi;
            sz->key_high_fade = zd->keyhifade;
            sz->key_root = zd->keyroot;
            sz->velocity_low = zd->vello;
            sz->velocity_low_fade = zd->vellofade;
            sz->velocity_high = zd->velhi;
            sz->velocity_high_fade = zd->velhifade;
            sz->mute = zd->mute;

            strncpy(sz->name, zd->name, 32);
            break;
        }
        case vga_zonelist_done:
        {
            markNeedsRepaintAndProxyUpdate();
            break;
        }
        case vga_note:
        {
            auto note = ad.data.i[0];
            if (note >= 0 && note < 128)
            {
                editor->playingMidiNotes[note] = ad.data.i[1];
                markNeedsRepaint();
            }
        }
        default:
            break;
        }
        return true;
    }
};
} // namespace proxies
} // namespace scxt

#endif // SHORTCIRCUIT_ZONELISTDATAPROXY_H
