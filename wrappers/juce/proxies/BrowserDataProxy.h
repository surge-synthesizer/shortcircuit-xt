//
// Created by Paul Walker on 1/4/22.
//

#ifndef SHORTCIRCUIT_BROWSERDATAPROXY_H
#define SHORTCIRCUIT_BROWSERDATAPROXY_H

#include "SCXTEditor.h"

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
