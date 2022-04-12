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

#ifndef SHORTCIRCUIT_VUMETERPOLYPHONYPROXY_H
#define SHORTCIRCUIT_VUMETERPOLYPHONYPROXY_H

#include "SCXTEditor.h"
#include "wrapper_msg_to_string.h"

namespace scxt
{
namespace proxies
{
struct VUMeterPolyphonyProxy : public scxt::data::UIStateProxy
{
    VUMeterPolyphonyProxy(SCXTEditor *ed) : editor(ed) {}

    bool processActionData(const actiondata &ad)
    {
        switch (ad.id)
        {
        case ip_vumeter:
        {
            auto at = std::get<VAction>(ad.actiontype);
            if (at != vga_vudata)
                break;
            int nOuts = ad.data.i[0];
            for (int i = 0; i < nOuts; ++i)
            {
                auto d1 = ad.data.i[i * 2 + 1];
                auto d2 = ad.data.i[i * 2 + 2];

                auto cl1 = d2 & 0x1;
                auto cl2 = (d2 & 0x2) >> 1;

                auto lv1 = d1 & 0xFF;
                auto lv2 = (d1 & 0xFF00) >> 8;

                editor->vuData[i].ch1 = lv1;
                editor->vuData[i].ch2 = lv2;
                editor->vuData[i].clip1 = cl1;
                editor->vuData[i].clip2 = cl2;
            }
            markNeedsRepaintAndProxyUpdate();
            return true;
            break;
        }
        case ip_polyphony:
        {
            auto at = std::get<VAction>(ad.actiontype);
            if (at != vga_intval)
                break;
            editor->polyphony = ad.data.i[0];
            markNeedsRepaintAndProxyUpdate();
            return true;
            break;
        }
        }
        return false;
    }
    SCXTEditor *editor{nullptr};
};
} // namespace proxies
} // namespace scxt

#endif // SHORTCIRCUIT_VUMETERPOLYPHONYPROXY_H
