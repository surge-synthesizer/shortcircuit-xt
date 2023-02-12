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

#ifndef SHORTCIRCUIT_SELECTIONSTATEPROXY_H
#define SHORTCIRCUIT_SELECTIONSTATEPROXY_H

#include "SCXTPluginEditor.h"
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
