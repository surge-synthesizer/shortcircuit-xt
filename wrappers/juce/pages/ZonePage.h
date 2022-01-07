//
// Created by Paul Walker on 1/6/22.
//

#ifndef SHORTCIRCUIT_ZONEPAGE_H
#define SHORTCIRCUIT_ZONEPAGE_H

#include "PageBase.h"

namespace SC3
{

namespace Components
{
struct ZoneKeyboardDisplay;
struct WaveDisplay;
struct ZoneEditor;

} // namespace Components
namespace Pages
{
struct ZonePage : PageBase
{
    ZonePage(SC3Editor *ed, SC3Editor::Pages p);
    ~ZonePage();

    void resized() override;
    virtual void connectProxies() override;

    std::unique_ptr<Components::ZoneKeyboardDisplay> zoneKeyboardDisplay;
    std::unique_ptr<Components::WaveDisplay> waveDisplay;
    std::unique_ptr<Components::ZoneEditor> zoneEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZonePage);
};
} // namespace Pages
} // namespace SC3

#endif // SHORTCIRCUIT_ZONEPAGE_H
