//
// Created by Paul Walker on 1/4/22.
//

#ifndef SHORTCIRCUIT_DATAINTERFACES_H
#define SHORTCIRCUIT_DATAINTERFACES_H

#include "sampler_wrapper_actiondata.h"

template <typename T> struct ParameterProxy
{
    int id{-1};
    int subid{-1};
    T data;
    bool hidden{false};
    bool disabled{false};
    std::string label;
};

/*
 * The UIStateProxy is a class which handles messages and keeps an appropriate state.
 * Components can render it for bulk action. For instance there woudl be a ZoneMapProxy
 * and so on. Each UIStateProxy registerred with the editor gets all the messages.
 */
class UIStateProxy
{
  public:
    struct Invalidatable
    {
        virtual ~Invalidatable() = default;
        virtual void onProxyUpdate() = 0;
    };

    struct InvalidateAndRepaintGuard
    {
        InvalidateAndRepaintGuard(const UIStateProxy &p) : proxy(p), go{true} {}
        ~InvalidateAndRepaintGuard()
        {
            if (go)
                proxy.invalidateAndRepaintClients();
        }
        bool deactivate()
        {
            go = false;
            return go;
        }
        const UIStateProxy &proxy;
        bool go;
    };

    virtual ~UIStateProxy() = default;
    virtual bool processActionData(const actiondata &d) = 0;
    std::unordered_set<juce::Component *> clients;
    void repaintClients() const
    {
        for (auto c : clients)
        {
            c->repaint();
        }
    }
    void invalidateClients() const
    {
        for (auto c : clients)
        {
            if (auto q = dynamic_cast<Invalidatable *>(c))
            {
                q->onProxyUpdate();
            }
        }
    }
    void invalidateAndRepaintClients() const
    {
        invalidateClients();
        repaintClients();
    }

    bool collectStringEntries(const actiondata &ad, InteractionId id, std::vector<std::string> &v)
    {
        if (ad.id != id)
            return false;
        if (std::holds_alternative<VAction>(ad.actiontype))
        {
            auto at = std::get<VAction>(ad.actiontype);
            if (at == vga_entry_clearall)
            {
                v.clear();
                return true;
            }
            if (at == vga_entry_add_ival_from_self_with_id)
            {
                auto entry = ad.data.i[0];
                auto val = (char *)(&ad.data.str[4]);
                if (entry >= v.size())
                    v.resize(entry + 1);
                v[entry] = val;
                return true;
            }
        }
        return false;
    }
};

struct ActionSender
{
    virtual ~ActionSender() = default;
    virtual void sendActionToEngine(const actiondata &ad) = 0;
};

template <typename T> inline bool applyActionData(const actiondata &ad, ParameterProxy<T> &proxy)
{
    if (!std::holds_alternative<VAction>(ad.actiontype))
        return false;

    proxy.id = ad.id;
    proxy.subid = ad.subid;

    auto at = std::get<VAction>(ad.actiontype);
    switch (at)
    {
    case vga_floatval:
        proxy.data = ad.data.f[0];
        return true;
        break;
    case vga_intval:
        proxy.data = ad.data.i[0];
        return true;
        break;
    case vga_disable_state:
        proxy.disabled = ad.data.i[0];
        return true;
        break;
    case vga_hide:
        proxy.hidden = ad.data.i[0];
        return true;
        break;
    case vga_label:
        proxy.label = (char *)(&ad.data.str[0]); // fixme - check for overruns
        return true;
        break;
    default:
        break;
    }
    return false;
}

template <typename T, size_t N>
inline bool applyActionData(const actiondata &ad, int base, ParameterProxy<T> (&proxy)[N])
{
    auto idx = ad.id - base;

    auto res = applyActionData(ad, proxy[idx]);

    return res;
}

struct FilterData
{
    ParameterProxy<float> p[n_filter_parameters];
    ParameterProxy<int> ip[n_filter_iparameters];
    ParameterProxy<float> mix;
    ParameterProxy<int> type;
    ParameterProxy<int> bypass;
};
struct MultiData
{
    ParameterProxy<float> filter_pregain[num_fxunits], filter_postgain[num_fxunits];
    ParameterProxy<int> filter_output[num_fxunits];
    FilterData filters[num_fxunits];
};
#endif // SHORTCIRCUIT_DATAINTERFACES_H
