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
                editor->n_databaseSampleList = ad.data.i[2];
                editor->databaseSampleList = (database_samplelist *)ad.data.ptr[0];
                invalidateAndRepaintClients();

                std::cout << "UPDATED DATABASE SAMPLES " << editor->n_databaseSampleList
                          << std::endl;
                auto d = editor->databaseSampleList;
                for (int i = 0; i < editor->n_databaseSampleList; ++i)
                {
                    std::cout << d[i].name << " " << d[i].size << " " << d[i].refcount << " "
                              << d[i].id << std::endl;
                }
                return true;
            }
        }
        return false;
    }

    SC3Editor *editor{nullptr};
};

#endif // SHORTCIRCUIT_BROWSERDATAPROXY_H
