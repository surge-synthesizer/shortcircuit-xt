//
// Created by Paul Walker on 1/6/22.
//

#ifndef SHORTCIRCUIT_VUMETERPROXY_H
#define SHORTCIRCUIT_VUMETERPROXY_H

#include "SC3Editor.h"
#include "wrapper_msg_to_string.h"

struct VUMeterProxy : public UIStateProxy
{
    VUMeterProxy(SC3Editor *ed) : editor(ed) {}

    bool processActionData(const actiondata &ad)
    {
        switch (ad.id)
        {
        case ip_vumeter:
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
        return false;
    }

    SC3Editor *editor{nullptr};
};

#endif // SHORTCIRCUIT_VUMETERPROXY_H
