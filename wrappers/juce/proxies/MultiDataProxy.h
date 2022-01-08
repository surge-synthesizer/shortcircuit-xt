//
// Created by Paul Walker on 1/7/22.
//

#ifndef SHORTCIRCUIT_MULTIDATAPROXY_H
#define SHORTCIRCUIT_MULTIDATAPROXY_H

#include "SC3Editor.h"

struct MultiDataProxy : public UIStateProxy
{
    MultiDataProxy(SC3Editor *ed) : editor(ed) {}

    bool processActionData(const actiondata &ad)
    {
        auto guard = InvalidateAndRepaintGuard(*this);
        if (collectStringEntries(ad, ip_multi_filter_type, editor->filterTypeNames))
        {
            return true;
        }
        if (collectStringEntries(ad, ip_multi_filter_output, editor->filterOutputNames))
        {
            return true;
        }

        if (ad.id >= ip_multi_filter_fp1 && ad.id <= ip_multi_filter_fp9)
        {
            if (applyActionData(ad, ip_multi_filter_fp1, editor->multi.filters[ad.subid].p))
                return true;
        }

        if (ad.id >= ip_multi_filter_ip1 && ad.id <= ip_multi_filter_ip2)
        {
            if (applyActionData(ad, ip_multi_filter_ip1, editor->multi.filters[ad.subid].ip))
                return true;
        }

        switch (ad.id)
        {
        case ip_multi_filter_pregain:
            if (applyActionData(ad, editor->multi.filter_pregain[ad.subid]))
                return true;
            break;
        case ip_multi_filter_postgain:
            if (applyActionData(ad, editor->multi.filter_postgain[ad.subid]))
                return true;
            break;
        case ip_multi_filter_output:
            if (applyActionData(ad, editor->multi.filter_output[ad.subid]))
                return true;
            break;
        case ip_multi_filter_type:
            if (applyActionData(ad, editor->multi.filters[ad.subid].type))
                return true;
            break;
        case ip_multi_filter_bypass:
            if (applyActionData(ad, editor->multi.filters[ad.subid].bypass))
                return true;
            break;

        case ip_multi_filter_object:
            // jassertfalse;
            return true;
        }
        return guard.deactivate();
    }

    SC3Editor *editor{nullptr};
};
#endif // SHORTCIRCUIT_MULTIDATAPROXY_H
