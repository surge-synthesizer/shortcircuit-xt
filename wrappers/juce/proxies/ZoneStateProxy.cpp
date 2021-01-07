
/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

#include "proxies/ZoneStateProxy.h"
#include "interaction_parameters.h"
#include "sampler_wrapper_actiondata.h"

void ZoneStateProxy::processActionData(actiondata &ad)
{
    switch (ad.id)
    {
    case ip_kgv_or_list:
    {
        switch (ad.actiontype)
        {
        case vga_zonelist_clear:
        {
            for (int i = 0; i < max_zones; ++i)
                activezones[i] = false;
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
            break;
        }
        case vga_zonelist_done:
        {
            repaintClients();
            break;
        }
        default:
            break;
        }
    }
    break;
    default:
        break;
    }
}