/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#include "sst/jucegui/data/Continuous.h"
#include "sst/jucegui/data/Discrete.h"
#include "datamodel/parameter.h"
#include <functional>
#include "sample/sample.h"

namespace scxt::ui::connectors
{
// TODO Factor this better obviously
template <typename Parent, typename Payload, typename ValueType = float>
struct PayloadDataAttachment : sst::jucegui::data::ContinunousModulatable
{
    ValueType &value;
    std::string label;
    std::function<void(const PayloadDataAttachment &at)> onGuiValueChanged;
    std::function<float(const Payload &)> extractFromPayload;

    PayloadDataAttachment(Parent *p, const datamodel::ControlDescription &cd, const std::string &l,
                          std::function<void(const PayloadDataAttachment &at)> oGVC,
                          std::function<float(const Payload &)> efp, ValueType &v)
        : description(cd), value(v), label(l), extractFromPayload(efp),
          onGuiValueChanged(std::move(oGVC))
    {
    }

    // TODO maybe. For now we have these descriptions as constexpr
    // in the code and apply them directly in many cases. We could
    // stream each and every one but don't. Maybe fix that one day.
    // Would def need to fix that to make web ui consistent if we write it
    datamodel::ControlDescription description;

    std::string getLabel() const override { return label; }
    float getValue() const override { return (float)value; }
    void setValueFromGUI(const float &f) override
    {
        value = (ValueType)f;
        onGuiValueChanged(*this);
    }

    std::function<std::string(float)> valueToString{nullptr};
    std::string getValueAsStringFor(float f) const override
    {
        if (valueToString)
            return valueToString(f);
        return Continuous::getValueAsStringFor(f);
    }
    std::function<std::optional<float>(const std::string &)> stringToValue{nullptr};
    void setValueAsString(const std::string &s) override
    {
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
    void setValueFromModel(const float &f) override { value = (ValueType)f; }
    void setValueFromPayload(const Payload &p) { setValueFromModel(extractFromPayload(p)); }

    float getMin() const override { return description.min; }
    float getMax() const override { return description.max; }
    float getDefaultValue() const override { return description.def; }

    bool isBipolar() const override { return description.min * description.max < 0; }

    float getModulationValuePM1() const override { return 0; }
    void setModulationValuePM1(const float &f) override {}
    bool isModulationBipolar() const override { return false; }

    void setAsMidiNote()
    {
        valueToString = [](float f) -> std::string {
            if (f < 0)
                return "";
            auto n = (int)std::round(f);
            auto o = n / 12 - 1;
            auto ni = n % 12;
            static std::array<std::string, 12> nn{"C",  "C#", "D",  "D#", "E",  "F",
                                                  "F#", "G",  "G#", "A",  "A#", "B"};

            auto res = nn[ni] + std::to_string(o);

            return res;
        };

        stringToValue = [](const std::string &s) -> float {
            auto c0 = std::toupper(s[0]);
            if (c0 >= 'A' && c0 <= 'G')
            {
                auto n0 = c0 - 'A';
                auto sharp = s[1] == '#';
                auto oct = std::atoi(s.c_str() + 1 + (sharp ? 1 : 0));

                std::vector<int> noteToPosition{-3, -1, 0, 2, 4, 5, 7};
                auto res = noteToPosition[n0] + sharp + (oct + 1) * 12;
                return res;
            }
            else
                return std::atoi(s.c_str());
        };
    }

    void setAsInteger()
    {
        valueToString = [](float f) -> std::string {
            auto n = (int)std::round(f);
            return std::to_string(n);
        };
    }
};

template <typename Parent, typename Payload, typename ValueType = int>
struct DiscretePayloadDataAttachment : sst::jucegui::data::Discrete
{
    ValueType &value;
    std::string label;
    std::function<void(const DiscretePayloadDataAttachment &at)> onGuiValueChanged;
    std::function<int(const Payload &)> extractFromPayload;

    DiscretePayloadDataAttachment(Parent *p, const datamodel::ControlDescription &cd,
                                  const std::string &l,
                                  std::function<void(const DiscretePayloadDataAttachment &at)> oGVC,
                                  std::function<int(const Payload &)> efp, ValueType &v)
        : description(cd), value(v), label(l), extractFromPayload(efp),
          onGuiValueChanged(std::move(oGVC))
    {
    }

    // TODO maybe. For now we have these descriptions as constexpr
    // in the code and apply them directly in many cases. We could
    // stream each and every one but don't. Maybe fix that one day.
    // Would def need to fix that to make web ui consistent if we write it
    datamodel::ControlDescription description;

    std::string getLabel() const override { return label; }
    int getValue() const override { return (int)value; }
    void setValueFromGUI(const int &f) override
    {
        value = (ValueType)f;
        onGuiValueChanged(*this);
    }
    void setValueFromModel(const int &f) override { value = (ValueType)f; }
    void setValueFromPayload(const Payload &p) { setValueFromModel(extractFromPayload(p)); }

    int getMin() const override { return (int)description.min; }
    int getMax() const override { return (int)description.max; }

    std::string getValueAsStringFor(int i) const override { return description.choices[i]; }
};

template <typename Parent, typename Payload>
struct BooleanPayloadDataAttachment : DiscretePayloadDataAttachment<Parent, Payload, bool>
{
    BooleanPayloadDataAttachment(
        Parent *p, const std::string &l,
        std::function<void(const DiscretePayloadDataAttachment<Parent, Payload, bool> &at)> oGVC,
        std::function<bool(const Payload &)> efp, bool &v)
        : DiscretePayloadDataAttachment<Parent, Payload, bool>(p, {}, l, oGVC, efp, v)
    {
    }

    int getMin() const override { return (int)0; }
    int getMax() const override { return (int)1; }

    std::string getValueAsStringFor(int i) const override { return i == 0 ? "Off" : "On"; }
};

struct SamplePointDataAttachment : sst::jucegui::data::ContinunousModulatable
{
    int64_t &value;
    std::string label;
    int64_t sampleCount{0};
    std::function<void(const SamplePointDataAttachment &)> onGuiChanged{nullptr};

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
    void setValueFromGUI(const float &f) override
    {
        value = (int64_t)f;
        if (onGuiChanged)
            onGuiChanged(*this);
    }
    std::string getLabel() const override { return label; }
    float getQuantizedStepSize() const override { return 1; }
    float getMin() const override { return -1; }
    float getMax() const override { return sampleCount; }
    float getDefaultValue() const override { return 0; }
    void setValueFromModel(const float &f) override { value = (int64_t)f; }

    float getModulationValuePM1() const override { return 0; }
    void setModulationValuePM1(const float &f) override {}
    bool isModulationBipolar() const override { return false; }
};

} // namespace scxt::ui::connectors
#endif // SHORTCIRCUIT_PAYLOADDATAATTACHMENT_H
