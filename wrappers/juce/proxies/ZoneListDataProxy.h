/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

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

        bool handled{false};
        switch (std::get<VAction>(ad.actiontype))
        {
        case vga_zonelist_clear:
        {
            for (int i = 0; i < MAX_ZONES; ++i)
                editor->activeZones[i] = false;
            handled = true;
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
            handled = true;
            break;
        }
        case vga_zonelist_done:
        {
            markNeedsRepaintAndProxyUpdate();
            handled = true;
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
            handled = true;
        }
        break;
        case vga_zonelist_mode:
        {
            editor->zoneListMode = ad.data.i[0];
            handled = true;
        }
        break;
        case vga_zone_playtrigger:
        {
            auto z = ad.data.i[0];
            auto s = ad.data.i[1];
            handled = true;
        }
        break;
        default:
            break;
        }
        return handled;
    }
};
} // namespace proxies
} // namespace scxt

#endif // SHORTCIRCUIT_ZONELISTDATAPROXY_H
