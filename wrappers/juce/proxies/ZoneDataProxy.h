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

#ifndef SHORTCIRCUIT_ZONEDATAPROXY_H
#define SHORTCIRCUIT_ZONEDATAPROXY_H

#include "SCXTEditor.h"

namespace scxt
{
namespace proxies
{
class ZoneDataProxy : public scxt::data::UIStateProxy
{
  public:
    ZoneDataProxy(SCXTEditor *ed) : editor(ed){};

    virtual bool processActionData(const actiondata &ad)
    {
        auto guard = InvalidateAndRepaintGuard(*this);

        bool res = false;
        auto &cz = editor->currentZone;
        switch (ad.id)
        {
        case ip_playmode:
            res = collectStringEntries(ad, editor->zonePlaymode);
            if (!res)
                res = applyActionData(ad, cz.playmode);
            break;

        case ip_zone_aux_output:
            res = collectStringEntries(ad, editor->zoneAuxOutput);
            if (!res)
                res = applyToOneOrAll(
                    ad, cz.aux, [](auto &r) -> auto & { return r.output; });

            break;
        case ip_zone_aux_outmode:
            res = applyToOneOrAll(
                ad, cz.aux, [](auto &r) -> auto & { return r.outmode; });
            break;
        case ip_zone_aux_balance:
            res = applyToOneOrAll(
                ad, cz.aux, [](auto &r) -> auto & { return r.balance; });
            break;
        case ip_zone_aux_level:
            res = applyToOneOrAll(
                ad, cz.aux, [](auto &r) -> auto & { return r.level; });
            break;

        case ip_nc_src:
            res = collectStringEntries(ad, editor->zoneNCSrc);
            if (!res)
                res = applyToOneOrAll(
                    ad, cz.nc, [](auto &r) -> auto & { return r.source; });
            break;
        case ip_nc_high:
            res = applyToOneOrAll(
                ad, cz.nc, [](auto &r) -> auto & { return r.high; });
            break;
        case ip_nc_low:
            res = applyToOneOrAll(
                ad, cz.nc, [](auto &r) -> auto & { return r.low; });
            break;

        case ip_mm_src:
            res = collectStringEntries(ad, editor->zoneMMSrc);
            if (!res)
                res = applyToOneOrAll(
                    ad, cz.mm, [](auto &r) -> auto & { return r.source; });
            break;
        case ip_mm_src2:
            res = collectStringEntries(ad, editor->zoneMMSrc2);
            if (!res)
                res = applyToOneOrAll(
                    ad, cz.mm, [](auto &r) -> auto & { return r.source2; });
            break;
        case ip_mm_dst:
            res = collectStringEntries(ad, editor->zoneMMDst);
            if (!res)
                res = applyToOneOrAll(
                    ad, cz.mm, [](auto &r) -> auto & { return r.destination; });
            break;
        case ip_mm_curve:
            res = collectStringEntries(ad, editor->zoneMMCurve);
            if (!res)
                res = applyToOneOrAll(
                    ad, cz.mm, [](auto &r) -> auto & { return r.curve; });
            break;
        case ip_mm_active:
            res = applyToOneOrAll(
                ad, cz.mm, [](auto &r) -> auto & { return r.active; });
            break;
        case ip_mm_amount:
            res = applyToOneOrAll(
                ad, cz.mm, [](auto &r) -> auto & { return r.strength; });
            break;

        case ip_zone_name:
            res = applyActionData(ad, cz.name);
            break;
        case ip_low_key:
            res = applyActionData(ad, cz.key_low);
            break;
        case ip_high_key:
            res = applyActionData(ad, cz.key_high);
            break;
        case ip_root_key:
            res = applyActionData(ad, cz.key_root);
            break;
        case ip_low_key_f:
            res = applyActionData(ad, cz.key_low_fade);
            break;
        case ip_high_key_f:
            res = applyActionData(ad, cz.key_high_fade);
            break;
        case ip_low_vel:
            res = applyActionData(ad, cz.velocity_low);
            break;
        case ip_high_vel:
            res = applyActionData(ad, cz.velocity_high);
            break;
        case ip_low_vel_f:
            res = applyActionData(ad, cz.velocity_low_fade);
            break;
        case ip_high_vel_f:
            res = applyActionData(ad, cz.velocity_high_fade);
            break;
        case ip_coarse_tune:
            res = applyActionData(ad, cz.transpose);
            break;
        case ip_fine_tune:
            res = applyActionData(ad, cz.finetune);
            break;
        case ip_pitchcorrect:
            res = applyActionData(ad, cz.pitchcorrection);
            break;
        case ip_velsense:
            res = applyActionData(ad, cz.velsense);
            break;
        case ip_pbdepth:
            res = applyActionData(ad, cz.pitch_bend_depth);
            break;
        case ip_keytrack:
            res = applyActionData(ad, cz.keytrack);
            break;
        case ip_pfg:
            res = applyActionData(ad, cz.pre_filter_gain);
            break;
        case ip_channel:
            res = applyActionData(ad, cz.part);
            break;
        case ip_reverse:
            res = applyActionData(ad, cz.reverse);
            break;
        case ip_mute:
            res = applyActionData(ad, cz.mute);
            break;
        case ip_mute_group:
            res = applyActionData(ad, cz.mute_group);
            break;
        case ip_ignore_part_polymode:
            res = applyActionData(ad, cz.ignore_part_polymode);
            break;
        case ip_lag:
            res = applyToOneOrAll(
                ad, cz.lag_generator, [](auto &r) -> auto & { return r; });
            break;

        case ip_filter_type:
            res = collectStringEntries(ad, editor->zoneFilterType);
            if (!res)
                res = applyToOneOrAll(
                    ad, cz.filter, [](auto &r) -> auto & { return r.type; });
            break;
        case ip_filter1_fp:
            res = applyToOneOrAll(
                ad, cz.filter[0].p, [](auto &r) -> auto & { return r; });
            break;
        case ip_filter1_ip:
            res = applyToOneOrAll(
                ad, cz.filter[0].ip, [](auto &r) -> auto & { return r; });
            break;
        case ip_filter2_fp:
            res = applyToOneOrAll(
                ad, cz.filter[1].p, [](auto &r) -> auto & { return r; });
            break;
        case ip_filter2_ip:
            res = applyToOneOrAll(
                ad, cz.filter[1].ip, [](auto &r) -> auto & { return r; });
            break;
        case ip_filter_bypass:
            res = applyToOneOrAll(
                ad, cz.filter, [](auto &r) -> auto & { return r.bypass; });
            break;
        case ip_filter_mix:
            res = applyToOneOrAll(
                ad, cz.filter, [](auto &r) -> auto & { return r.mix; });
            break;

        case ip_EG_a:
            res = applyToOneOrAll(
                ad, cz.env, [](auto &r) -> auto & { return r.a; });
            break;
        case ip_EG_h:
            res = applyToOneOrAll(
                ad, cz.env, [](auto &r) -> auto & { return r.h; });
            break;
        case ip_EG_d:
            res = applyToOneOrAll(
                ad, cz.env, [](auto &r) -> auto & { return r.d; });
            break;
        case ip_EG_s:
            res = applyToOneOrAll(
                ad, cz.env, [](auto &r) -> auto & { return r.s; });
            break;
        case ip_EG_r:
            res = applyToOneOrAll(
                ad, cz.env, [](auto &r) -> auto & { return r.r; });
            break;
        case ip_EG_s0:
            res = applyToOneOrAll(
                ad, cz.env, [](auto &r) -> auto & { return r.s0; });
            break;
        case ip_EG_s1:
            res = applyToOneOrAll(
                ad, cz.env, [](auto &r) -> auto & { return r.s1; });
            break;
        case ip_EG_s2:
            res = applyToOneOrAll(
                ad, cz.env, [](auto &r) -> auto & { return r.s2; });
            break;

        case ip_lfo_load:
            res = collectStringEntries(ad, editor->zoneLFOPresets);
            if (!res)
                res = applyToOneOrAll(
                    ad, cz.lfo, [](auto &r) -> auto & { return r.presetLoad; });
            break;
        case ip_lforate:
            res = applyToOneOrAll(
                ad, cz.lfo, [](auto &r) -> auto & { return r.rate; });
            break;
        case ip_lfoshape:
            res = applyToOneOrAll(
                ad, cz.lfo, [](auto &r) -> auto & { return r.smooth; });
            break;
        case ip_lforepeat:
            res = applyToOneOrAll(
                ad, cz.lfo, [](auto &r) -> auto & { return r.repeat; });
            break;
        case ip_lfocycle:
            res = applyToOneOrAll(
                ad, cz.lfo, [](auto &r) -> auto & { return r.cyclemode; });
            break;
        case ip_lfosync:
            res = applyToOneOrAll(
                ad, cz.lfo, [](auto &r) -> auto & { return r.temposync; });
            break;
        case ip_lfotrigger:
            res = applyToOneOrAll(
                ad, cz.lfo, [](auto &r) -> auto & { return r.triggermode; });
            break;
        case ip_lfoshuffle:
            res = applyToOneOrAll(
                ad, cz.lfo, [](auto &r) -> auto & { return r.shuffle; });
            break;
        case ip_lfoonce:
            res = applyToOneOrAll(
                ad, cz.lfo, [](auto &r) -> auto & { return r.onlyonce; });
            break;

        default:
            break;
        }

        if (res)
            return res;

        if (ad.id == ip_lfosteps)
        {
            auto at = std::get<VAction>(ad.actiontype);
            switch (at)
            {
            case vga_steplfo_data_single:
            {
                auto lfo = ad.subid;
                jassert(ad.subid >= 0 && ad.subid < 3);
                auto step = ad.data.i[0];
                jassert(step >= 0 && step < 32);
                auto val = ad.data.f[1];
                cz.lfo[lfo].data[step].val = val;
                cz.lfo[lfo].data[step].id = ad.id;
                cz.lfo[lfo].data[step].subid = ad.subid;
                res = true;
            }
            break;

            case vga_steplfo_repeat:
                cz.lfo[ad.subid].repeat.val = ad.data.i[0];
                res = true;
                break;
            case vga_steplfo_shape:
                cz.lfo[ad.subid].smooth.val = ad.data.f[0];
                res = true;
                break;
            case vga_label:
                // that's ok;
                res = true;
                break;
            case vga_disable_state:
            case vga_floatval:
                // std::cout << FILE_LINE_OS << ad << " ignored" << std::endl;
                res = true;
                break;
            default:
                std::cout << FILE_LINE_OS << ad << std::endl;
                break;
            }
            if (res)
                return res;
        }

        if (ad.id >= ip_zone_params_begin && ad.id <= ip_zone_params_end)
        {
            std::cout << FILE_LINE_OS << ad << std::endl;
            return guard.deactivate();
        }
        return guard.deactivate();
    }

    SCXTEditor *editor{nullptr};
};
} // namespace proxies
} // namespace scxt
#endif // SHORTCIRCUIT_ZONEDATAPROXY_H
