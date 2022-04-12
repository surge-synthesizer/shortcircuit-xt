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

#ifndef SHORTCIRCUIT_CONFIGDATAPROXY_H
#define SHORTCIRCUIT_CONFIGDATAPROXY_H

#include "SCXTEditor.h"

namespace scxt
{
namespace proxies
{
struct ConfigDataProxy : public scxt::data::UIStateProxy
{
    ConfigDataProxy(SCXTEditor *ed) : editor(ed) {}

    bool processActionData(const actiondata &ad)
    {
        auto g = InvalidateAndRepaintGuard(*this);
        auto &cd = editor->config;

        if (applyActionDataIf(ad, ip_config_previewvolume, cd.previewLevel))
            return true;
        if (applyActionDataIf(ad, ip_config_autopreview, cd.autoPreview))
            return true;

        if (ad.id == ip_config_controller_id)
            if (data::applyToOneOrAll(ad, cd.controllerId))
                return true;
        if (ad.id == ip_config_controller_mode)
            if (data::applyToOneOrAll(ad, cd.controllerMode))
                return true;

        return g.deactivate();
    }

    SCXTEditor *editor;
};
} // namespace proxies
} // namespace scxt
#endif // SHORTCIRCUIT_CONFIGDATAPROXY_H
