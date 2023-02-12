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

#ifndef SHORTCIRCUIT_BROWSERDATAPROXY_H
#define SHORTCIRCUIT_BROWSERDATAPROXY_H

#include "SCXTPluginEditor.h"

namespace scxt
{
namespace proxies
{
class BrowserDataProxy : public scxt::data::UIStateProxy
{
  public:
    BrowserDataProxy(SCXTEditor *ed) : editor(ed){};

    virtual bool processActionData(const actiondata &ad)
    {
        return false;
        auto ig = InvalidateAndRepaintGuard(*this);
        if (std::holds_alternative<VAction>(ad.actiontype))
        {
            auto at = std::get<VAction>(ad.actiontype);
            if (at == vga_database_samplelist)
            {
                editor->samplesCopyActiveCount = ad.data.i[2];
                auto dsl = (database_samplelist *)ad.data.ptr[0];

                for (auto &d : editor->samplesCopy)
                    d.id = -1;

                if (!dsl)
                    return true;

                for (int i = 0; i < editor->samplesCopyActiveCount; ++i)
                {
                    editor->samplesCopy[i] = dsl[i];
                }

                *((int *)dsl) = 'done';

                for (const auto &d : editor->samplesCopy)
                {
                    if (d.id >= 0)
                        std::cout << d.name << " " << d.size << " " << d.refcount << " " << d.id
                                  << std::endl;
                }
                return true;
            }
        }
        return ig.deactivate();
    }

    SCXTEditor *editor{nullptr};
};
} // namespace proxies
} // namespace scxt

#endif // SHORTCIRCUIT_BROWSERDATAPROXY_H
