//
// Created by Paul Walker on 1/5/22.
//

#ifndef SHORTCIRCUIT_SELECTIONSTATEPROXY_H
#define SHORTCIRCUIT_SELECTIONSTATEPROXY_H

#include "SCXTEditor.h"
#include "wrapper_msg_to_string.h"

namespace scxt
{
namespace proxies
{

struct SelectionStateProxy : public scxt::data::UIStateProxy
{
    SelectionStateProxy(SCXTEditor *ed) : editor(ed) {}

    bool processActionData(const actiondata &ad)
    {
        auto g = InvalidateAndRepaintGuard(*this);
        switch (ad.id)
        {
        case ip_partselect:
            editor->selectedPart = ad.data.i[0];
            return true;
            break;

        case ip_layerselect:
            editor->selectedLayer = ad.data.i[0];
            return true;
            break;

        case ip_wavedisplay:
            // FIXME - re-handle the way wavedisplay gets sent up
            return true;
            break;

        case ip_sample_name:
            editor->currentSampleName = ad.data.str;
            return true;
            break;

        case ip_sample_metadata:
            editor->currentSampleMetadata = ad.data.str;
            return true;
            break;


        case ip_browser_previewbutton:
        case ip_sample_prevnext:
        case ip_patch_prevnext:
            auto inter = ip_data[ad.id];
            // std::cout << "SELECT " << debug_wrapper_ip_to_string(ad.id) << " " << inter << " " <<
            // ad
            //           << std::endl;
            return g.deactivate();
            break;
        }
        return g.deactivate();
    }

    SCXTEditor *editor{nullptr};
};
} // namespace proxies
} // namespace scxt
#endif // SHORTCIRCUIT_SELECTIONSTATEPROXY_H
