/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_UI_CONNECTORS_PAYLOADDATAATTACHMENT_H
#define SCXT_SRC_UI_CONNECTORS_PAYLOADDATAATTACHMENT_H

#include <functional>
#include <utility>
#include <memory>
#include <string>
#include <type_traits>
#include "sst/jucegui/data/Continuous.h"
#include "sst/jucegui/data/Discrete.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "datamodel/metadata.h"
#include "sample/sample.h"
#include "app/HasEditor.h"

namespace scxt::ui::connectors
{

template <typename M, typename P, typename V, typename... Args>
inline void updateSingleValue(const P &p, V &value, app::HasEditor *e, Args... args)
{
    static_assert(std::is_standard_layout_v<P>);

    if constexpr (M::hasBoundPayload)
    {
        static_assert(std::is_same_v<typename M::bound_t, P>,
                      "This means you used a message for a member of a wrong payload type");
    }

    ptrdiff_t pdiff = (uint8_t *)&value - (uint8_t *)&p;
    assert(pdiff >= 0);
    assert(pdiff <= sizeof(p) - sizeof(value));

    e->sendToSerialization(M({args..., pdiff, value}));
}

template <typename M, typename A, typename ABase, typename... Args>
inline std::function<void(const ABase &)> makeUpdater(A &att, const typename A::payload_t &p,
                                                      app::HasEditor *e, Args... args)
{
    static_assert(std::is_standard_layout_v<typename A::payload_t>);

    if constexpr (M::hasBoundPayload)
    {
        static_assert(std::is_same_v<typename M::bound_t, typename A::payload_t>,
                      "This means you used a message for a member of a wrong payload type");
    }

    ptrdiff_t pdiff = (uint8_t *)&att.value - (uint8_t *)&p;
    assert(pdiff >= 0);
    assert(pdiff <= sizeof(p) - sizeof(att.value));

    auto jc = dynamic_cast<juce::Component *>(e);
    assert(jc);

    auto showTT = std::is_same_v<
        typename std::remove_cv<typename std::remove_reference<decltype(att.value)>::type>::type,
        float>;

    return [w = juce::Component::SafePointer(jc), e, showTT, pdiff, args...](const ABase &a) {
        if (w)
        {
            e->sendToSerialization(M({args..., pdiff, a.value}));
            if (showTT)
            {
                e->updateValueTooltip(a);
            }
            w->repaint();
        }
    };
}

template <typename M, typename A, typename ABase = A, typename... Args>
inline void configureUpdater(A &att, const typename A::payload_t &p, app::HasEditor *e,
                             Args... args)
{
    att.onGuiValueChanged = makeUpdater<M, A, ABase>(att, p, e, std::forward<Args>(args)...);
}

template <typename A, typename F> inline void addGuiStepBeforeSend(A &att, F preStep)
{
    auto orig = att.onGuiValueChanged;
    att.onGuiValueChanged = [orig, preStep](const auto &a) {
        preStep(a);
        orig(a);
    };
}
template <typename A, typename F> inline void addGuiStep(A &att, F andThen)
{
    auto orig = att.onGuiValueChanged;
    att.onGuiValueChanged = [orig, andThen](const auto &a) {
        orig(a);
        andThen(a);
    };
}

template <typename Payload, typename ValueType = float>
struct PayloadDataAttachment : sst::jucegui::data::Continuous
{
    typedef Payload payload_t;
    typedef ValueType value_t;

    ValueType &value;
    ValueType prevValue;
    std::string label;
    std::function<void(const PayloadDataAttachment &at)> onGuiValueChanged{nullptr};
    std::function<bool(const PayloadDataAttachment &at)> isTemposynced{nullptr};

    PayloadDataAttachment(const datamodel::pmd &cd,
                          std::function<void(const PayloadDataAttachment &at)> oGVC, ValueType &v)
        : description(cd), value(v), prevValue(v), label(cd.name),
          onGuiValueChanged(std::move(oGVC))
    {
    }

    PayloadDataAttachment(const datamodel::pmd &cd, ValueType &v)
        : description(cd), value(v), prevValue(v), label(cd.name), onGuiValueChanged(nullptr)
    {
    }

    PayloadDataAttachment(const PayloadDataAttachment &other) = delete;
    PayloadDataAttachment &operator=(const PayloadDataAttachment &other) = delete;
    PayloadDataAttachment &operator=(PayloadDataAttachment &&other) = delete;

    // TODO maybe. For now we have these descriptions as constexpr
    // in the code and apply them directly in many cases. We could
    // stream each and every one but don't. Maybe fix that one day.
    // Would def need to fix that to make web ui consistent if we write it
    datamodel::pmd description;

    std::string getLabel() const override { return label; }
    float getValue() const override { return (float)value; }

    void setValueFromGUIQuantized(const float &f) override
    {
        if (isTemposynced && isTemposynced(*this))
        {
            setValueFromGUI(f);
        }
        if (description.supportsQuantization())
        {
            setValueFromGUI(description.quantize(f));
        }
        else
        {
            setValueFromGUI(f);
        }
    }
    void setValueFromGUI(const float &ff) override
    {
        float f = ff;
        if (isTemposynced && isTemposynced(*this))
        {
            f = description.snapToTemposync(f);
        }
        if (f != prevValue)
        {
            prevValue = value;
            value = (ValueType)f;
            if (onGuiValueChanged)
            {
                onGuiValueChanged(*this);
            }
        }
    }

    std::function<std::string(float)> valueToString{nullptr};
    std::string getValueAsStringFor(float f) const override
    {
        if (description.supportsStringConversion)
        {
            auto res = description.valueToString(f, getFeatureState());
            if (res.has_value())
                return *res;
        }
        if (valueToString)
            return valueToString(f);
        return Continuous::getValueAsStringFor(f);
    }

    std::string getValueAsStringWithoutUnitsFor(float f) const override
    {
        if (description.supportsStringConversion)
        {
            auto res = description.valueToString(f, getFeatureState().withNoUnits(true));
            if (res.has_value())
                return *res;
        }
        if (valueToString)
            return valueToString(f);
        return Continuous::getValueAsStringWithoutUnitsFor(f);
    }

    std::function<std::optional<float>(const std::string &)> stringToValue{nullptr};
    void setValueAsString(const std::string &s) override
    {
        if (description.supportsStringConversion)
        {
            if (isTemposynced && isTemposynced(*this))
            {
                auto f = description.valueFromTemposyncNotation(s);
                if (f.has_value())
                {
                    setValueFromGUI(*f);
                    return;
                }
            }
            std::string em;
            auto res = description.valueFromString(s, em);
            if (res.has_value())
            {
                setValueFromGUI(*res);
                return;
            }
        }
        if (stringToValue)
        {
            auto f = stringToValue(s);
            if (f.has_value())
            {
                setValueFromGUI(*f);
                return;
            }
        }
        Continuous::setValueAsString(s);
    }
    void setValueFromModel(const float &f) override
    {
        prevValue = value;
        value = (ValueType)f;

        for (auto *l : guilisteners)
            l->dataChanged();
    }

    float getMin() const override { return description.minVal; }
    float getMax() const override { return description.maxVal; }
    float getDefaultValue() const override { return description.defaultVal; }

    bool isBipolar() const override { return description.isBipolar(); }

    void andThenOnGui(std::function<void(const PayloadDataAttachment &)> f)
    {
        auto orig = onGuiValueChanged;
        onGuiValueChanged = [f, orig](auto &at) {
            if (orig)
                orig(at);
            if (f)
                f(at);
        };
    }

    void setTemposyncFunction(std::function<bool(const PayloadDataAttachment &at)> f)
    {
        assert(description.canTemposync);
        isTemposynced = f;
    }

    sst::basic_blocks::params::ParamMetaData::FeatureState getFeatureState() const
    {
        auto res = sst::basic_blocks::params::ParamMetaData::FeatureState();

        if (isTemposynced)
            res = res.withTemposync(isTemposynced(*this));

        return res;
    }
};

template <typename Payload, typename ValueType = int32_t>
struct DiscretePayloadDataAttachment : sst::jucegui::data::Discrete
{
    typedef Payload payload_t;
    typedef ValueType value_t;
    typedef DiscretePayloadDataAttachment<Payload, ValueType> onGui_t;

    ValueType &value;
    ValueType prevValue;
    std::string label;
    std::function<void(const onGui_t &at)> onGuiValueChanged;

    DiscretePayloadDataAttachment(const datamodel::pmd &cd,
                                  std::function<void(const onGui_t &at)> oGVC, ValueType &v)
        : description(cd), value(v), prevValue(v), label(cd.name),
          onGuiValueChanged(std::move(oGVC))
    {
    }

    DiscretePayloadDataAttachment(const datamodel::pmd &cd, ValueType &v)
        : description(cd), value(v), prevValue(v), label(cd.name), onGuiValueChanged(nullptr)
    {
    }

    // TODO maybe. For now we have these descriptions as constexpr
    // in the code and apply them directly in many cases. We could
    // stream each and every one but don't. Maybe fix that one day.
    // Would def need to fix that to make web ui consistent if we write it
    datamodel::pmd description;

    std::string getLabel() const override { return label; }
    int getValue() const override { return (int)value; }
    void setValueFromGUI(const int &f) override
    {
        prevValue = value;
        value = (ValueType)f;
        onGuiValueChanged(*this);
    }
    void setValueFromModel(const int &f) override
    {
        prevValue = value;
        value = (ValueType)f;

        for (auto *l : guilisteners)
            l->dataChanged();
    }

    int getMin() const override { return (int)description.minVal; }
    int getMax() const override { return (int)description.maxVal; }

    std::string getValueAsStringFor(int i) const override
    {
        auto r = description.valueToString((float)i);
        if (r.has_value())
            return *r;
        return "";
    }

    void andThenOnGui(std::function<void(const DiscretePayloadDataAttachment &)> f)
    {
        auto orig = onGuiValueChanged;
        onGuiValueChanged = [f, orig](auto &at) {
            if (orig)
                orig(at);
            if (f)
                f(at);
        };
    }
};

template <typename Payload>
struct BooleanPayloadDataAttachment : DiscretePayloadDataAttachment<Payload, bool>
{
    using payload_t = Payload;
    using value_t = bool;
    using parent_t = DiscretePayloadDataAttachment<Payload, bool>;

    BooleanPayloadDataAttachment(
        const std::string &l,
        std::function<void(const DiscretePayloadDataAttachment<Payload, bool> &at)> oGVC, bool &v)
        : parent_t(datamodel::pmd().withType(datamodel::pmd::BOOL).withName(l), oGVC, v)
    {
    }

    BooleanPayloadDataAttachment(const std::string &p, bool &v)
        : DiscretePayloadDataAttachment<Payload, bool>(datamodel::pmd().asBool().withName(p), v)
    {
    }
    BooleanPayloadDataAttachment(const datamodel::pmd &p, bool &v)
        : DiscretePayloadDataAttachment<Payload, bool>(p, v)
    {
    }

    int getMin() const override { return (int)0; }
    int getMax() const override { return (int)1; }
    int getValue() const override
    {
        if (isInverted)
            return !parent_t::getValue();
        else
            return parent_t::getValue();
    }
    void setValueFromGUI(const int &f) override
    {
        if (isInverted)
            parent_t::setValueFromGUI(!f);
        else
            parent_t::setValueFromGUI(f);
    }

    // Don't override setValueFromModel since invert is a gui-side only concept. I think.

    std::string getValueAsStringFor(int i) const override
    {
        if (isInverted)
            return i == 0 ? "On" : "Off";
        else
            return i == 0 ? "Off" : "On";
    }

    bool isInverted{false};
};

struct DirectBooleanPayloadDataAttachment : sst::jucegui::data::Discrete
{
    bool &value;
    std::function<void(const bool)> callback;
    DirectBooleanPayloadDataAttachment(std::function<void(const bool val)> oGVC, bool &v)
        : callback(oGVC), value(v)
    {
    }

    std::string getLabel() const override { return "Bool"; }
    int getValue() const override { return value ? 1 : 0; }
    void setValueFromGUI(const int &f) override
    {
        value = f;
        callback(f);
    }
    void setValueFromModel(const int &f) override
    {
        value = f;
        for (auto *l : guilisteners)
            l->dataChanged();
    }

    int getMin() const override { return (int)0; }
    int getMax() const override { return (int)1; }

    std::string getValueAsStringFor(int i) const override { return i == 0 ? "Off" : "On"; }
};

struct SamplePointDataAttachment : sst::jucegui::data::Continuous
{
    int64_t &value;
    std::string label;
    int64_t sampleCount{0};
    std::function<void(const SamplePointDataAttachment &)> onGuiChanged{nullptr};
    std::function<int64_t(int64_t)> precheckGuiAdjust{nullptr};

    SamplePointDataAttachment(int64_t &v,
                              std::function<void(const SamplePointDataAttachment &)> ogc)
        : value(v), onGuiChanged(ogc)
    {
    }

    float getValue() const override { return value; }
    std::string getValueAsStringFor(float f) const override
    {
        if (f < 0)
            return "";
        return fmt::format("{}", (int64_t)f);
    }
    void setValueFromGUI(const float &ff) override
    {
        auto f = ff;
        if (precheckGuiAdjust)
        {
            f = precheckGuiAdjust(ff);
        }
        value = (int64_t)f;
        if (onGuiChanged)
            onGuiChanged(*this);
    }
    std::string getLabel() const override { return label; }
    float getQuantizedStepSize() const override { return 1; }
    float getMin() const override { return -1; }
    float getMax() const override { return sampleCount; }
    float getDefaultValue() const override { return 0; }
    void setValueFromModel(const float &f) override
    {
        value = (int64_t)f;
        for (auto *l : guilisteners)
            l->dataChanged();
    }
};

template <typename A, typename Msg, typename ABase = A> struct SingleValueFactory
{
    template <typename W, typename... Args>
    static std::pair<std::unique_ptr<A>, std::unique_ptr<W>>
    attachR(const datamodel::pmd &md, const typename A::payload_t &p, typename A::value_t &val,
            app::HasEditor *e, Args... args)
    {
        auto att = std::make_unique<A>(md, val);
        configureUpdater<Msg, A, ABase>(*att, p, e, std::forward<Args>(args)...);
        auto wid = std::make_unique<W>();
        wid->setSource(att.get());

        auto showTT = std::is_same_v<
            typename std::remove_cv<typename std::remove_reference<decltype(val)>::type>::type,
            float>;
        constexpr bool isText =
            std::is_same_v<W, sst::jucegui::components::DraggableTextEditableValue>;

        if constexpr (isText)
        {
            wid->onPopupMenu = [sw = juce::Component::SafePointer(wid.get())](auto &m) {
                if (sw)
                    sw->activateEditor();
            };
        }
        else if (showTT)
        {
            e->setupWidgetForValueTooltip(wid.get(), att.get());
        }
        else
        {
            e->setupIntAttachedWidgetForValueMenu(wid.get(), att.get());
        }
        e->addSubscription(val, att);

        return {std::move(att), std::move(wid)};
    }

    template <typename... Args>
    static std::unique_ptr<A> attachOnly(const typename A::payload_t &p, typename A::value_t &val,
                                         app::HasEditor *e, Args... args)
    {
        auto md = scxt::datamodel::describeValue(p, val);

        auto att = std::make_unique<A>(md, val);
        configureUpdater<Msg, A, ABase>(*att, p, e, std::forward<Args>(args)...);

        e->addSubscription(val, att);
        return std::move(att);
    }

    template <typename W, typename... Args>
    static void attach(const typename A::payload_t &p, typename A::value_t &val, app::HasEditor *e,
                       std::unique_ptr<A> &aRes, std::unique_ptr<W> &wRes, Args... args)
    {
        auto md = scxt::datamodel::describeValue(p, val);

        auto [a, w] = attachR<W>(md, p, val, e, std::forward<Args>(args)...);
        aRes = std::move(a);
        wRes = std::move(w);
    }

    template <typename W, typename... Args>
    static void attach(const datamodel::pmd &md, const typename A::payload_t &p,
                       typename A::value_t &val, app::HasEditor *e, std::unique_ptr<A> &aRes,
                       std::unique_ptr<W> &wRes, Args... args)
    {
        auto [a, w] = attachR<W>(md, p, val, e, std::forward<Args>(args)...);
        aRes = std::move(a);
        wRes = std::move(w);
    }

    template <typename W, typename... Args>
    static void attachAndAdd(const datamodel::pmd &md, const typename A::payload_t &p,
                             typename A::value_t &val, app::HasEditor *e, std::unique_ptr<A> &aRes,
                             std::unique_ptr<W> &wRes, Args... args)
    {
        auto [a, w] = attachR<W>(md, p, val, e, std::forward<Args>(args)...);
        aRes = std::move(a);
        wRes = std::move(w);
        auto jc = dynamic_cast<juce::Component *>(e);
        assert(jc);
        if (jc)
        {
            jc->addAndMakeVisible(*wRes);
        }
    }

    template <typename W, typename... Args>
    static void attachAndAdd(const typename A::payload_t &p, typename A::value_t &val,
                             app::HasEditor *e, std::unique_ptr<A> &aRes, std::unique_ptr<W> &wRes,
                             Args... args)
    {
        auto md = scxt::datamodel::describeValue(p, val);
        attachAndAdd(md, p, val, e, aRes, wRes, std::forward<Args>(args)...);
    }

    template <typename W, typename... Args>
    static void attachLabelAndAdd(const datamodel::pmd &md, const typename A::payload_t &p,
                                  typename A::value_t &val, app::HasEditor *e,
                                  std::unique_ptr<A> &aRes, std::unique_ptr<W> &wRes,
                                  const std::string &lab, Args... args)
    {
        assert(!wRes);
        wRes = std::make_unique<W>();

        attachAndAdd(md, p, val, e, aRes, wRes->item, std::forward<Args>(args)...);

        using labUPType = decltype(W::label);
        using labType = typename labUPType::element_type;
        wRes->label = std::make_unique<labType>();
        wRes->label->setText(lab);
        auto jc = dynamic_cast<juce::Component *>(e);
        assert(jc);
        if (jc)
        {
            jc->addAndMakeVisible(*(wRes->label));
        }
    }

    template <typename W, typename... Args>
    static void attachLabelAndAdd(const typename A::payload_t &p, typename A::value_t &val,
                                  app::HasEditor *e, std::unique_ptr<A> &aRes,
                                  std::unique_ptr<W> &wRes, const std::string &lab, Args... args)
    {
        auto md = scxt::datamodel::describeValue(p, val);
        attachLabelAndAdd(md, p, val, e, aRes, wRes, lab, std::forward<Args>(args)...);
    }
};

struct DummyContinuous : sst::jucegui::data::Continuous
{
    float f{0.5};
    bool bip{false};
    std::string lab{"Unimpl"};
    virtual float getValue() const override { return f; }
    virtual void setValueFromGUI(const float &fv) override
    {
        if (!everEdited && onFirstEdit)
            onFirstEdit();
        f = fv;
        everEdited = true;
    }
    virtual void setValueFromModel(const float &fv) override { f = fv; };
    virtual float getDefaultValue() const override { return 0.5f; };
    virtual bool isBipolar() const override { return bip; }
    float getMin() const override { return isBipolar() ? -1 : 0; }

    std::string getValueAsStringFor(float f) const override { return fmt::format("{:.2f}", f); }

    virtual std::string getLabel() const override
    {
        if (lab != "Unimpl")
            return lab + " (Unimplemented)";
        return lab;
    };

    std::function<void()> onFirstEdit{nullptr};
    bool everEdited{false};
};

template <typename T>
std::unique_ptr<T> makeConnectedToDummy(uint32_t index, const std::string &lab, float initVal,
                                        bool isBip, std::function<void()> onFirstEdit)
{
    static std::unordered_map<uint32_t, std::unique_ptr<DummyContinuous>> dummyMap;

    auto res = std::make_unique<T>();

    DummyContinuous *thisWillLeak{nullptr};
    auto dmp = dummyMap.find(index);
    if (dmp != dummyMap.end())
    {
        thisWillLeak = dmp->second.get();
    }
    else
    {
        auto tmp = std::make_unique<DummyContinuous>();
        thisWillLeak = tmp.get();

        thisWillLeak->f = initVal;
        thisWillLeak->bip = isBip;
        thisWillLeak->lab = lab;
        thisWillLeak->onFirstEdit = onFirstEdit;

        dummyMap[index] = std::move(tmp);
        std::string s;

        auto t = index;
        for (int i = 0; i < 4; ++i)
        {
            char c = (char)((t & 0xFF000000) >> 24);
            s += c;
            t = t << 8;
        }

        // SCLOG("Incomplete UI bound to dummy source '" << s << "'");
    }

    res->setSource(thisWillLeak);
    return res;
}

template <typename A, typename Msg>
using BooleanSingleValueFactory =
    SingleValueFactory<A, Msg, DiscretePayloadDataAttachment<typename A::payload_t, bool>>;

} // namespace scxt::ui::connectors
#endif // SHORTCIRCUIT_PAYLOADDATAATTACHMENT_H
