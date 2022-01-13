//
// Created by Paul Walker on 1/10/22.
//

#ifndef SHORTCIRCUIT_PARTPROXY_H
#define SHORTCIRCUIT_PARTPROXY_H

#include "SCXTEditor.h"

namespace scxt
{
namespace proxies
{
struct PartDataProxy : public UIStateProxy
{
    PartDataProxy(SCXTEditor *ed) : editor(ed) {}

    bool processActionData(const actiondata &ad) override
    {
        auto g = InvalidateAndRepaintGuard(*this);
        auto &part = editor->parts[editor->selectedPart];

        if (collectStringEntries(ad, ip_part_aux_output, editor->partAuxOutputNames))
        {
            return true;
        }
        if (collectStringEntries(ad, ip_part_filter_type, editor->partFilterTypeNames))
            return true;
        if (collectStringEntries(ad, ip_part_mm_src, editor->partMMSrc))
            return true;
        if (collectStringEntries(ad, ip_part_mm_src2, editor->partMMSrc2))
            return true;
        if (collectStringEntries(ad, ip_part_mm_dst, editor->partMMDst))
            return true;
        if (collectStringEntries(ad, ip_part_mm_curve, editor->partMMCurve))
            return true;
        if (collectStringEntries(ad, ip_part_nc_src, editor->partNCSrc))
            return true;

        if (applyActionDataSubIdIf(ad, ip_part_filter1_fp, part.filters[0].p))
            return true;
        if (applyActionDataSubIdIf(ad, ip_part_filter1_ip, part.filters[0].ip))
            return true;
        if (applyActionDataSubIdIf(ad, ip_part_filter2_fp, part.filters[1].p))
            return true;
        if (applyActionDataSubIdIf(ad, ip_part_filter2_ip, part.filters[1].ip))
            return true;

        /*
         * This is a bit out of pattern
         */
        if (applyActionDataIf(ad, ip_part_filter_type, part.filters[ad.subid].type))
            return true;
        if (applyActionDataIf(ad, ip_part_filter_mix, part.filters[ad.subid].mix))
            return true;
        if (applyActionDataIf(ad, ip_part_filter_bypass, part.filters[ad.subid].bypass))
            return true;

        if (applyActionDataSubIdIf(ad, ip_part_userparam_value, part.userparameter))
            return true;
        if (applyActionDataSubIdIf(ad, ip_part_userparam_name, part.userparameter_name))
            return true;
        if (applyActionDataSubIdIf(ad, ip_part_userparam_polarity, part.userparameter_polarity))
            return true;

        /*
         * AUX busses
         */
        if (applyActionDataIf(ad, ip_part_aux_level, part.aux[ad.subid].level))
            return true;
        if (applyActionDataIf(ad, ip_part_aux_balance, part.aux[ad.subid].balance))
            return true;
        if (applyActionDataIf(ad, ip_part_aux_output, part.aux[ad.subid].output))
            return true;
        if (applyActionDataIf(ad, ip_part_aux_outmode, part.aux[ad.subid].outmode))
            return true;

        /*
         * MM object
         */
        if (applyActionDataIf(ad, ip_part_mm_src, part.mm[ad.subid].source))
            return true;
        if (applyActionDataIf(ad, ip_part_mm_src2, part.mm[ad.subid].source2))
            return true;
        if (applyActionDataIf(ad, ip_part_mm_dst, part.mm[ad.subid].destination))
            return true;
        if (applyActionDataIf(ad, ip_part_mm_amount, part.mm[ad.subid].strength))
            return true;
        if (applyActionDataIf(ad, ip_part_mm_active, part.mm[ad.subid].active))
            return true;
        if (applyActionDataIf(ad, ip_part_mm_curve, part.mm[ad.subid].curve))
            return true;

        /*
         * NCs
         */
        if (applyActionDataIf(ad, ip_part_nc_src, part.nc[ad.subid].source))
            return true;
        if (applyActionDataIf(ad, ip_part_nc_high, part.nc[ad.subid].high))
            return true;
        if (applyActionDataIf(ad, ip_part_nc_low, part.nc[ad.subid].low))
            return true;

        /*
         * One offs
         */
        if (applyActionDataIf(ad, ip_part_name, part.name))
            return true;
        if (applyActionDataIf(ad, ip_part_midichannel, part.midichannel))
            return true;
        if (applyActionDataIf(ad, ip_part_midichannel, part.midichannel))
            return true;
        if (applyActionDataIf(ad, ip_part_polylimit, part.polylimit))
            return true;
        if (applyActionDataIf(ad, ip_part_transpose, part.transpose))
            return true;
        if (applyActionDataIf(ad, ip_part_formant, part.formant))
            return true;
        if (applyActionDataIf(ad, ip_part_polymode, part.polymode))
            return true;
        if (applyActionDataIf(ad, ip_part_portamento_mode, part.portamento_mode))
            return true;
        if (applyActionDataIf(ad, ip_part_portamento, part.portamento))
            return true;
        if (applyActionDataIf(ad, ip_part_vs_layers, part.part_vs_layers))
            return true;
        if (applyActionDataIf(ad, ip_part_vs_distribution, part.part_vs_distribution))
            return true;
        if (applyActionDataIf(ad, ip_part_vs_xf_equality, part.part_vs_xf_equality))
            return true;
        if (applyActionDataIf(ad, ip_part_vs_xfade, part.part_vs_xfade))
            return true;

        if (ad.id == ip_part_aux_output)
        {
            if (std::get<VAction>(ad.actiontype) == vga_entry_replace_label_on_id)
            {
                std::cout << __FILE__ << ":" << __LINE__ << " " << ad << " " << ad.data.i[0]
                          << " - " << (char *)(&(ad.data.str[4])) << std::endl;
            }
            return g.deactivate();
        }

        if (ad.id >= ip_part_params_begin && ad.id <= ip_part_params_end)
        {
            std::cout << __FILE__ << ":" << __LINE__ << " " << ad << std::endl;
        }
        return g.deactivate();
    }

    SCXTEditor *editor{nullptr};
};
} // namespace proxies
} // namespace scxt

#endif // SHORTCIRCUIT_PARTPROXY_H
