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
struct PartDataProxy : public scxt::data::UIStateProxy
{
    PartDataProxy(SCXTEditor *ed) : editor(ed) {}

    bool processActionData(const actiondata &ad) override
    {
        auto g = InvalidateAndRepaintGuard(*this);
        auto &part = editor->currentPart;

        bool res = false;
        switch (ad.id)
        {
        case ip_part_aux_output:
            res = data::collectStringEntries(ad, editor->partAuxOutputNames);
            if (!res)
                res = data::applyToOneOrAll(
                    ad, part.aux, [](auto &r) -> auto & { return r.output; });
            break;
        case ip_part_aux_level:
            res = data::applyToOneOrAll(
                ad, part.aux, [](auto &r) -> auto & { return r.level; });
            break;
        case ip_part_aux_balance:
            res = data::applyToOneOrAll(
                ad, part.aux, [](auto &r) -> auto & { return r.balance; });
            break;
        case ip_part_aux_outmode:
            res = data::applyToOneOrAll(
                ad, part.aux, [](auto &r) -> auto & { return r.outmode; });
            break;

        case ip_part_filter1_fp:
            res = data::applyToOneOrAll(ad, part.filters[0].p);
            break;
        case ip_part_filter1_ip:
            res = data::applyToOneOrAll(ad, part.filters[0].ip);
            break;
        case ip_part_filter2_fp:
            res = data::applyToOneOrAll(ad, part.filters[1].p);
            break;
        case ip_part_filter2_ip:
            res = data::applyToOneOrAll(ad, part.filters[1].ip);
            break;
        case ip_part_filter_type:
            res = data::collectStringEntries(ad, editor->partFilterTypeNames);
            if (!res)
                res = data::applyToOneOrAll(
                    ad, part.filters, [](auto &r) -> auto & { return r.type; });
            break;
        case ip_part_filter_mix:
            res = data::applyToOneOrAll(
                ad, part.filters, [](auto &r) -> auto & { return r.mix; });
            break;
        case ip_part_filter_bypass:
            res = data::applyToOneOrAll(
                ad, part.filters, [](auto &r) -> auto & { return r.bypass; });
            break;

        case ip_part_userparam_value:
            res = data::applyToOneOrAll(ad, part.userparameter);
            break;
        case ip_part_userparam_name:
            res = data::applyToOneOrAll(ad, part.userparameter_name);
            break;
        case ip_part_userparam_polarity:
            res = data::applyToOneOrAll(ad, part.userparameter_polarity);
            break;

        case ip_part_name:
            res = data::applyActionData(ad, part.name);
            break;
        case ip_part_midichannel:
            res = data::applyActionData(ad, part.midichannel);
            break;
        case ip_part_polylimit:
            res = data::applyActionData(ad, part.polylimit);
            break;
        case ip_part_transpose:
            res = data::applyActionData(ad, part.transpose);
            break;
        case ip_part_formant:
            res = data::applyActionData(ad, part.formant);
            break;
        case ip_part_polymode:
            res = data::applyActionData(ad, part.polymode);
            break;
        case ip_part_portamento_mode:
            res = data::applyActionData(ad, part.portamento_mode);
            break;
        case ip_part_portamento:
            res = data::applyActionData(ad, part.portamento);
            break;
        case ip_part_vs_layers:
            res = data::applyActionData(ad, part.part_vs_layers);
            break;
        case ip_part_vs_distribution:
            res = data::applyActionData(ad, part.part_vs_distribution);
            break;
        case ip_part_vs_xf_equality:
            res = data::applyActionData(ad, part.part_vs_xf_equality);
            break;
        case ip_part_vs_xfade:
            res = data::applyActionData(ad, part.part_vs_xfade);
            break;

        case ip_part_nc_src:
            res = data::collectStringEntries(ad, editor->partNCSrc);
            if (!res)
                res = data::applyToOneOrAll(
                    ad, part.nc, [](auto &r) -> auto & { return r.source; });
            break;
        case ip_part_nc_high:
            res = data::applyToOneOrAll(
                ad, part.nc, [](auto &r) -> auto & { return r.high; });
            break;
        case ip_part_nc_low:
            res = data::applyToOneOrAll(
                ad, part.nc, [](auto &r) -> auto & { return r.low; });
            break;

        case ip_part_mm_src:
            res = data::collectStringEntries(ad, editor->partMMSrc);
            if (!res)
                res = data::applyToOneOrAll(
                    ad, part.mm, [](auto &r) -> auto & { return r.source; });
            break;
        case ip_part_mm_src2:
            res = data::collectStringEntries(ad, editor->partMMSrc2);
            if (!res)
                res = data::applyToOneOrAll(
                    ad, part.mm, [](auto &r) -> auto & { return r.source2; });
            break;
        case ip_part_mm_dst:
            res = data::collectStringEntries(ad, editor->partMMDst);
            if (!res)
                res = data::applyToOneOrAll(
                    ad, part.mm, [](auto &r) -> auto & { return r.destination; });
            break;
        case ip_part_mm_curve:
            res = data::collectStringEntries(ad, editor->partMMCurve);
            if (!res)
                res = data::applyToOneOrAll(
                    ad, part.mm, [](auto &r) -> auto & { return r.curve; });
            break;
        case ip_part_mm_active:
            res = data::applyToOneOrAll(
                ad, part.mm, [](auto &r) -> auto & { return r.active; });
            break;
        case ip_part_mm_amount:
            res = data::applyToOneOrAll(
                ad, part.mm, [](auto &r) -> auto & { return r.strength; });
            break;
        default:
            break;
        }

        if (res)
            return true;

        if (ad.id >= ip_part_params_begin && ad.id <= ip_part_params_end)
        {
            std::cout << FILE_LINE_OS << ad << std::endl;
        }
        return g.deactivate();
    }

    SCXTEditor *editor{nullptr};
};
} // namespace proxies
} // namespace scxt

#endif // SHORTCIRCUIT_PARTPROXY_H
