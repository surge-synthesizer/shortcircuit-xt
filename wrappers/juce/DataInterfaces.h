//
// Created by Paul Walker on 1/4/22.
//

#ifndef SHORTCIRCUIT_DATAINTERFACES_H
#define SHORTCIRCUIT_DATAINTERFACES_H

#include "sampler_wrapper_actiondata.h"
#include <charconv>

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
        InvalidateAndRepaintGuard(UIStateProxy &p) : proxy(p), go{true} {}
        ~InvalidateAndRepaintGuard()
        {
            if (go)
                proxy.markNeedsRepaintAndProxyUpdate();
        }
        bool deactivate()
        {
            go = false;
            return go;
        }
        UIStateProxy &proxy;
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

    enum Validity
    {
        VALID = 0,
        NEEDS_REPAINT = 1 << 1,
        NEEDS_PROXY_UPDATE = 1 << 2
    };
    uint32_t validity{VALID};

    void markNeedsRepaint() { validity = validity | NEEDS_REPAINT; }
    void markNeedsProxyUpdate() { validity = validity | NEEDS_PROXY_UPDATE; }
    void markNeedsRepaintAndProxyUpdate()
    {
        validity = validity | NEEDS_PROXY_UPDATE | NEEDS_REPAINT;
    }

    void sweepValidity()
    {
        if (validity == VALID)
            return;
        bool rp = validity & NEEDS_REPAINT;
        bool px = validity & NEEDS_PROXY_UPDATE;
        validity = VALID;
        for (auto c : clients)
        {
            if (rp)
            {
                c->repaint();
            }

            if (px)
            {
                if (auto q = dynamic_cast<Invalidatable *>(c))
                {
                    q->onProxyUpdate();
                }
            }
        }
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

template <typename T> struct ParameterProxy
{
    int id{-1};
    int subid{-1};
    T val;
    bool hidden{false};
    bool disabled{false};
    std::string label;

    T min, max, def, step;
    std::string units;

    void parseDatamode(const char *datamode)
    {
        // syntax is char,min,step,max,def,units
        std::string dm{datamode};
        std::vector<std::string> elements;
        while (true)
        {
            auto p = dm.find(',');
            if (p == std::string::npos)
            {
                elements.push_back(dm);
                break;
            }
            else
            {
                auto r = dm.substr(0, p);
                dm = dm.substr(p + 1);
                elements.push_back(r);
            }
        }
        jassert(elements.size() == 6);
        jassert(elements[0][0] == dataModeIndicator());
        min = strToT(elements[1]);
        step = strToT(elements[2]);
        max = strToT(elements[3]);
        def = strToT(elements[4]);
        units = elements[5];
    }

    char dataModeIndicator() const { return 'x'; }
    T strToT(const std::string &s)
    {
        jassertfalse;
        return T{};
    }
};

template <> inline char ParameterProxy<float>::dataModeIndicator() const { return 'f'; }
template <> inline float ParameterProxy<float>::strToT(const std::string &s)
{
    // Ideally we would use  auto [ptr, ec]{std::from_chars(s.data(), s.data() + s.size(), val,
    // std::chars_format::general)}; but the runtimes don't support it yet
    float res = 0.0f;
    std::istringstream istr(s);

    istr.imbue(std::locale::classic());
    istr >> res;
    return res;
}
template <> inline char ParameterProxy<int>::dataModeIndicator() const { return 'i'; }
template <> inline int ParameterProxy<int>::strToT(const std::string &s)
{
    return std::atoi(s.c_str());
}

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
        proxy.val = ad.data.f[0];
        return true;
        break;
    case vga_intval:
        proxy.val = ad.data.i[0];
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
    case vga_datamode:
        proxy.parseDatamode(ad.data.str);
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
