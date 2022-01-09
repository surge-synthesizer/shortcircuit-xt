//
// Created by Paul Walker on 1/5/22.
//

#ifndef SHORTCIRCUIT_SELECTIONSTATEPROXY_H
#define SHORTCIRCUIT_SELECTIONSTATEPROXY_H

#include "SC3Editor.h"
#include "wrapper_msg_to_string.h"

struct SelectionStateProxy : public UIStateProxy
{
    SelectionStateProxy(SC3Editor *ed) : editor(ed) {}

    bool processActionData(const actiondata &ad)
    {
        switch (ad.id)
        {
        case ip_partselect:
            editor->selectedPart = ad.data.i[0];
            markNeedsRepaintAndProxyUpdate();
            return true;
            break;
        case ip_layerselect:
        case ip_kgv_or_list:
        case ip_wavedisplay:
        case ip_browser_previewbutton:
        case ip_sample_prevnext:
        case ip_patch_prevnext:
            auto inter = ip_data[ad.id];
            // std::cout << "SELECT " << debug_wrapper_ip_to_string(ad.id) << " " << inter << " " <<
            // ad
            //           << std::endl;
            return true;
            break;
        }
        return false;
    }

    SC3Editor *editor{nullptr};
};

#endif // SHORTCIRCUIT_SELECTIONSTATEPROXY_H
