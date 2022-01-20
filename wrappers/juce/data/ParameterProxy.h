//
// Created by Paul Walker on 1/15/22.
//

#ifndef SHORTCIRCUIT_PARAMETERPROXY_H
#define SHORTCIRCUIT_PARAMETERPROXY_H

#include <fmt/core.h>

#include "configuration.h"
#include "sampler_wrapper_actiondata.h"
#include "data/BasicTypes.h"

namespace scxt
{
namespace data
{

template <typename T> struct SendVGA
{
};

template <> struct SendVGA<float>
{
    static constexpr VAction action = vga_floatval;
};
template <> struct SendVGA<int>
{
    static constexpr VAction action = vga_intval;
};
template <> struct SendVGA<std::string>
{
    static constexpr VAction action = vga_text;
};

template <typename T, VAction A = SendVGA<T>::action> struct ParameterProxy
{
    ParameterProxy() {}
    int id{-1};
    int subid{-1};
    T val;
    bool hidden{false};
    bool disabled{false};
    std::string label;

    T min{0}, max{1}, def{0}, step;
    std::string units;
    bool paramRangesSet{false};
    bool complainedAboutParamRangesSet{false};

    enum Units
    {
        DEF,
        DB,
        HERTZ,
        KHERTZ,
        PERCENT,
        SECONDS,
        SEMITONES,
        CENTS,
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
        paramRangesSet = true;

        unitType = DEF;
        if (units == "dB")
            unitType = DB;
        if (units == "Hz")
            unitType = HERTZ;
        if (units == "%")
            unitType = PERCENT;
        if (units == "s")
            unitType = SECONDS;
        if (units == "kHz")
            unitType = KHERTZ;
        if (units == "oct")
            unitType = OCTAVE;
        if (units == "st")
            unitType = SEMITONES;
        if (units == "cents")
            unitType = CENTS;

        if (unitType == DEF)
        {
            // std::cout << "UNIT [" << units << "]" << " dm=[" << datamode << "]" << std::endl;
            // jassert(unitType != DEF);
        }
    }

    std::string valueToStringWithUnits() const
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
        case HERTZ:
        {
            auto res = fmt::format("{:.3f} Hz", 440 * pow(2.0, val));
            return res;
        }

        case DEF:
        case KHERTZ:
        default:
            return fmt::format("{:.3f} RAW", val);
        }
    }

    // Float vs Int a bit clumsy here
    float getValue01() const { return (val - min) / (max - min); }
    void sendValue01(float value01, ActionSender *s) { jassert(false); }
    void sendValue(const T &value, ActionSender *s) { jassertfalse; }

    std::string value_to_string() const { return std::to_string(val); }

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

template <> inline std::string ParameterProxy<float>::value_to_string() const
{
    return fmt::format("{:.2f}", val);
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

template <>
inline void ParameterProxy<float, vga_floatval>::sendValue01(float value01, ActionSender *s)
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
template <>
inline void ParameterProxy<float, vga_floatval>::sendValue(const float &value, ActionSender *s)
{
    val = value;
    actiondata ad;
    ad.id = id;
    ad.subid = subid;
    ad.actiontype = vga_floatval;
    ad.data.f[0] = val;
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
template <> inline void ParameterProxy<int>::sendValue(const int &value, ActionSender *s)
{
    actiondata ad;
    ad.id = id;
    ad.subid = subid;
    ad.actiontype = vga_intval;
    ad.data.i[0] = value;
    val = value;
    s->sendActionToEngine(ad);
}

template <>
inline void ParameterProxy<std::string, vga_text>::sendValue(const std::string &value,
                                                             ActionSender *s)
{
    val = value;
    actiondata ad;
    ad.id = id;
    ad.subid = subid;
    ad.actiontype = vga_text;
    strncpy(ad.data.str, val.c_str(), 54);
    s->sendActionToEngine(ad);
}
} // namespace data
} // namespace scxt

#endif // SHORTCIRCUIT_PARAMETERPROXY_H
