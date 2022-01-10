//
// Created by Paul Walker on 1/6/22.
//

#ifndef SHORTCIRCUIT_ZONEPAGE_H
#define SHORTCIRCUIT_ZONEPAGE_H

#include "PageBase.h"

namespace scxt
{

namespace components
{
struct ZoneKeyboardDisplay;
struct WaveDisplay;
struct ZoneEditor;

} // namespace components
namespace pages
{
struct ZonePage : PageBase
{
    ZonePage(SCXTEditor *ed, SCXTEditor::Pages p);
    ~ZonePage();

    void resized() override;
    virtual void connectProxies() override;

    std::unique_ptr<components::ZoneKeyboardDisplay> zoneKeyboardDisplay;
    std::unique_ptr<components::WaveDisplay> waveDisplay;
    std::unique_ptr<components::ZoneEditor> zoneEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZonePage);
};
} // namespace pages
} // namespace scxt

#endif // SHORTCIRCUIT_ZONEPAGE_H
