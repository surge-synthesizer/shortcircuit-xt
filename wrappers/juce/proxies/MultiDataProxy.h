//
// Created by Paul Walker on 1/7/22.
//

#ifndef SHORTCIRCUIT_MULTIDATAPROXY_H
#define SHORTCIRCUIT_MULTIDATAPROXY_H

#include "SCXTEditor.h"

namespace scxt
{
namespace proxies
{
struct MultiDataProxy : public scxt::data::UIStateProxy
{
    MultiDataProxy(SCXTEditor *ed) : editor(ed) {}

    bool processActionData(const actiondata &ad)
    {
        auto guard = InvalidateAndRepaintGuard(*this);
        if (collectStringEntriesIf(ad, ip_multi_filter_type, editor->multiFilterTypeNames))
            return true;

        if (collectStringEntriesIf(ad, ip_multi_filter_output, editor->multiFilterOutputNames))
            return true;

        if (ad.id >= ip_multi_filter_fp1 && ad.id <= ip_multi_filter_fp9)
        {
            if (data::applyToOneOrAll(
                    ad, ip_multi_filter_fp1, editor->multi.filters,
                    [](auto &r) -> auto & { return r.p; }))
                return true;
        }

        if (ad.id >= ip_multi_filter_ip1 && ad.id <= ip_multi_filter_ip2)
        {
            if (applyToOneOrAll(
                    ad, ip_multi_filter_ip1, editor->multi.filters,
                    [](auto &r) -> auto & { return r.ip; }))
                return true;
        }

        switch (ad.id)
        {
        case ip_multi_filter_pregain:
            if (applyToOneOrAll(ad, editor->multi.filter_pregain))
                return true;
            break;
        case ip_multi_filter_postgain:
            if (applyToOneOrAll(ad, editor->multi.filter_postgain))
                return true;
            break;
        case ip_multi_filter_output:
            if (applyToOneOrAll(ad, editor->multi.filter_output))
                return true;
            break;
        case ip_multi_filter_type:
            if (applyToOneOrAll(
                    ad, editor->multi.filters, [](auto &r) -> auto & { return r.type; }))
                return true;
            break;
        case ip_multi_filter_bypass:
            if (applyToOneOrAll(
                    ad, editor->multi.filters, [](auto &r) -> auto & { return r.bypass; }))
                return true;
            break;

        case ip_multi_filter_object:
            // jassertfalse;
            return true;
        }
        return guard.deactivate();
    }

    SCXTEditor *editor{nullptr};
};
} // namespace proxies
} // namespace scxt

#endif // SHORTCIRCUIT_MULTIDATAPROXY_H
