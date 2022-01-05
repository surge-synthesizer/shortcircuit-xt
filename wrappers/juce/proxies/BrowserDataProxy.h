//
// Created by Paul Walker on 1/4/22.
//

#ifndef SHORTCIRCUIT_BROWSERDATAPROXY_H
#define SHORTCIRCUIT_BROWSERDATAPROXY_H

#include "SC3Editor.h"

class BrowserDataProxy : public UIStateProxy
{
  public:
    BrowserDataProxy(SC3Editor *ed) : editor(ed){};

    virtual bool processActionData(const actiondata &ad)
    {
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

                invalidateAndRepaintClients();

                for (const auto &d : editor->samplesCopy)
                {
                    if (d.id >= 0)
                        std::cout << d.name << " " << d.size << " " << d.refcount << " " << d.id
                                  << std::endl;
                }
                return true;
            }
        }
        return false;
    }

    SC3Editor *editor{nullptr};
};

#endif // SHORTCIRCUIT_BROWSERDATAPROXY_H
