//
// Created by Paul Walker on 2/11/22.
//

#ifndef SHORTCIRCUIT_STYLESHEET_H
#define SHORTCIRCUIT_STYLESHEET_H

#include "juce_gui_basics/juce_gui_basics.h"
#include <string>

namespace scxt
{
namespace style
{
struct Selector
{
    Selector(const char *c) : id(c) {}
    Selector(const std::string &n) : id(n) {}

    std::string id;
};

enum ColourFeature
{
    backgroundColor,
    outlineColor,
    textColor,
    headerTextColor,
    sliderHandleColor,
    sliderHandleOutlineColor,
    sliderHandleHoverColor,
    sliderHandleHoverOutlineColor,
    sliderTrayColor
};

struct DOMParticipant
{
    juce::Component *asJuceComponent() { return dynamic_cast<juce::Component *>(this); }

    explicit DOMParticipant(const Selector &s) : selector(s) {}
    virtual ~DOMParticipant() = default;

    virtual const Selector &getSelector() const { return selector; }
    virtual const bool getIsDOMContainer() const { return isContainerVal; }
    virtual const void setIsDOMContainer(bool b) { isContainerVal = b; }
    virtual void setupJuceAccessibility()
    {
        auto c = asJuceComponent();
        jassert(c);
        if (!c)
            return;
        c->setAccessible(true);
        if (isContainerVal)
            c->setFocusContainerType(juce::Component::FocusContainerType::focusContainer);
        c->setTitle(selector.id);
    }

    virtual void resetSelector(const Selector &s) { selector = s; }

  private:
    bool isContainerVal{true};
    Selector selector;
};

struct Sheet
{
    virtual ~Sheet() = default;
    virtual juce::Colour resolveColour(const DOMParticipant &d, const ColourFeature &f) = 0;
};

std::unique_ptr<Sheet> createDefaultSheet();

} // namespace style
} // namespace scxt
#endif // SHORTCIRCUIT_STYLESHEET_H
