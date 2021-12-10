
/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include "proxies/ZoneStateProxy.h"
#include "interaction_parameters.h"
#include "sampler_wrapper_actiondata.h"

bool ZoneStateProxy::processActionData(const actiondata &ad)
{
    bool res = false;
    switch (ad.id)
    {
    case ip_kgv_or_list:
    {
        if (!std::holds_alternative<VAction>(ad.actiontype))
            break;
        switch (std::get<VAction>(ad.actiontype))
        {
        case vga_zonelist_clear:
        {
            for (int i = 0; i < max_zones; ++i)
                activezones[i] = false;
            res = true;
            break;
        }
        case vga_zonelist_populate:
        {
            ad_zonedata *zd = (ad_zonedata *)&ad;

            sample_zone *sz = &zonecopies[zd->zid];
            activezones[zd->zid] = true;
            sz->key_low = zd->keylo;
            sz->key_high = zd->keyhi;
            strncpy(sz->name, zd->name, 32);
            res = true;
            break;
        }
        case vga_zonelist_done:
        {
            repaintClients();
            res = true;
            break;
        }
        case vga_note:
        {
            auto note = ad.data.i[0];
            if (note >= 0 && note < 128)
            {
                playingMidiNotes[note] = ad.data.i[1];
                repaintClients();
            }
            res = true;
        }
        default:
            break;
        }
    }
    break;
    default:
        break;
    }

    return res;
}