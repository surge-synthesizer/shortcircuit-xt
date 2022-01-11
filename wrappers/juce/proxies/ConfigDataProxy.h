//
// Created by Paul Walker on 1/11/22.
//

#ifndef SHORTCIRCUIT_CONFIGDATAPROXY_H
#define SHORTCIRCUIT_CONFIGDATAPROXY_H

#include "SCXTEditor.h"

namespace scxt
{
namespace proxies
{
struct ConfigDataProxy : public UIStateProxy
{
    ConfigDataProxy(SCXTEditor *ed) : editor(ed) {}

    bool processActionData(const actiondata &ad)
    {
        auto g = InvalidateAndRepaintGuard(*this);
        auto &cd = editor->config;

        if (applyActionDataIf(ad, ip_config_previewvolume, cd.previewLevel))
            return true;
        if (applyActionDataIf(ad, ip_config_kbdmode, cd.kbdMode))
            return true;
        if (applyActionDataIf(ad, ip_config_outputs, cd.outputs))
            return true;
        if (applyActionDataIf(ad, ip_config_autopreview, cd.autoPreview))
            return true;

        if (applyActionDataIf(ad, ip_config_controller_id, cd.controllerId[ad.subid]))
            return true;
        if (applyActionDataIf(ad, ip_config_controller_mode, cd.controllerMode[ad.subid]))
            return true;

        return g.deactivate();
    }

    SCXTEditor *editor;
};
} // namespace proxies
} // namespace scxt
#endif // SHORTCIRCUIT_CONFIGDATAPROXY_H
