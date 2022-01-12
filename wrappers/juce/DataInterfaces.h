//
// Created by Paul Walker on 1/4/22.
//

#ifndef SHORTCIRCUIT_DATAINTERFACES_H
#define SHORTCIRCUIT_DATAINTERFACES_H

#include "sampler_wrapper_actiondata.h"
#include "sampler_state.h"
#include "configuration.h"
#include <charconv>
#include <fmt/core.h>
#include "util/unitconversion.h"
#include "synthesis/mathtables.h"

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
            if (at == vga_entry_add_ival_from_self)
            {
                v.push_back(ad.data.str);
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

    enum Units
    {
        DEF,
        DB,
        HZ,
        KHZ,
        PERCENT,
        SECONDS,
        OCTAVE
    } unitType{DEF};

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

        unitType = DEF;
        if (units == "dB")
            unitType = DB;
        if (units == "Hz")
            unitType = HZ;
        if (units == "%")
            unitType = PERCENT;
        if (units == "s")
            unitType = SECONDS;
        if (units == "kHz")
            unitType = KHZ;
        if (units == "oct")
            unitType = OCTAVE;
        jassert(unitType != DEF);
    }

    std::string valueToStringWithUnits()
    {
        switch (unitType)
        {
        case DB:
            return fmt::format("{:.2f} dB", val);
        case PERCENT:
            return fmt::format("{:.2f} %", val * 100);
        case OCTAVE:
            return fmt::format("{:.2f} oct", val);
        case SECONDS:
        {
            auto res = fmt::format("{:.3f} s", pow(2.0, val));
            return res;
        }
        case HZ:
        {
            auto res = fmt::format("{:.3f} Hz", 440 * pow(2.0, val));
            return res;
        }

        case DEF:
        case KHZ:
        default:
            return fmt::format("{:.3f} RAW", val);
        }
    }

    // Float vs Int a bit clumsy here
    float getValue01() const { return (val - min) / (max - min); }
    void sendValue01(float value01, ActionSender *s) { jassert(false); }
    void sendValue(T value, ActionSender *s) { jassertfalse; }

    std::string value_to_string() const {}

    char dataModeIndicator() const { return 'x'; }
    T strToT(const std::string &s)
    {
        jassertfalse;
        return T{};
    }

    template <typename Q> void setFrom(const Q &v) { jassertfalse; }
};

template <> template <> inline void ParameterProxy<float>::setFrom<float>(const float &v)
{
    val = v;
}
template <> template <> inline void ParameterProxy<int>::setFrom<int>(const int &v) { val = v; }

template <>
template <>
inline void ParameterProxy<std::string>::setFrom<std::string>(const std::string &v)
{
    val = v;
}

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

template <> inline void ParameterProxy<float>::sendValue01(float value01, ActionSender *s)
{
    auto v = value01 * (max - min) + min;
    val = v;
    actiondata ad;
    ad.id = id;
    ad.subid = subid;
    ad.actiontype = vga_floatval;
    ad.data.f[0] = v;
    s->sendActionToEngine(ad);
}

template <> inline char ParameterProxy<int>::dataModeIndicator() const { return 'i'; }
template <> inline int ParameterProxy<int>::strToT(const std::string &s)
{
    return std::atoi(s.c_str());
}
template <> inline float ParameterProxy<int>::getValue01() const
{
    jassert(false);
    return 0;
}
template <> inline void ParameterProxy<int>::sendValue(int value, ActionSender *s)
{
    actiondata ad;
    ad.id = id;
    ad.subid = subid;
    ad.actiontype = vga_intval;
    ad.data.i[0] = value;
    val = value;
    s->sendActionToEngine(ad);
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
        proxy.setFrom(ad.data.f[0]);
        return true;
        break;
    case vga_intval:
        proxy.setFrom(ad.data.i[0]);
        return true;
        break;
    case vga_text:
        proxy.setFrom(std::string(ad.data.str));
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

template <typename T>
inline bool applyActionDataIf(const actiondata &ad, const InteractionId &id,
                              ParameterProxy<T> &proxy)
{
    if (ad.id == id && applyActionData(ad, proxy))
        return true;
    return false;
}
template <typename T, size_t N>
inline bool applyActionDataSubIdIf(const actiondata &ad, const InteractionId &id,
                                   ParameterProxy<T> (&proxy)[N])
{
    if (ad.id == id && applyActionData(ad, proxy[ad.subid]))
        return true;
    return false;
}
struct FilterData
{
    ParameterProxy<float> p[n_filter_parameters];
    ParameterProxy<int> ip[n_filter_iparameters];
    ParameterProxy<float> mix;
    ParameterProxy<int> type;
    ParameterProxy<int> bypass;
};

struct AuxBusData
{
    ParameterProxy<float> level, balance;
    ParameterProxy<int> output, outmode;
};

struct MMEntryData
{
    ParameterProxy<int> source, source2;
    ParameterProxy<int> destination;
    ParameterProxy<float> strength;
    ParameterProxy<int> active, curve;
};

struct NCEntryData
{
    ParameterProxy<int> source, low, high;
};

struct PartData
{
    ParameterProxy<std::string> name;
    ParameterProxy<int> midichannel;
    ParameterProxy<int> polylimit;
    ParameterProxy<int> transpose;
    ParameterProxy<int> formant;
    ParameterProxy<int> polymode;
    ParameterProxy<float> portamento;
    ParameterProxy<int> portamento_mode;

    ParameterProxy<int> part_vs_layers;
    ParameterProxy<float> part_vs_distribution;
    ParameterProxy<int> part_vs_xf_equality;
    ParameterProxy<float> part_vs_xfade;

    FilterData filters[num_filters_per_part];

    AuxBusData aux[num_aux_busses];
    MMEntryData mm[mm_part_entries];
    NCEntryData nc[num_part_ncs];

    ParameterProxy<float> userparameter[num_midi_channels];
    ParameterProxy<std::string> userparameter_name[num_midi_channels];
    ParameterProxy<int> userparameter_polarity[num_midi_channels];
};
struct MultiData
{
    ParameterProxy<float> filter_pregain[num_fxunits], filter_postgain[num_fxunits];
    ParameterProxy<int> filter_output[num_fxunits];
    FilterData filters[num_fxunits];
};

struct ConfigData
{
    ParameterProxy<float> previewLevel;
    ParameterProxy<int> autoPreview, kbdMode, outputs;

    ParameterProxy<int> controllerId[n_custom_controllers], controllerMode[n_custom_controllers];
};
#endif // SHORTCIRCUIT_DATAINTERFACES_H
