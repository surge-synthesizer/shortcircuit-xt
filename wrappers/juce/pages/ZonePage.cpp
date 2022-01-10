//
// Created by Paul Walker on 1/6/22.
//

#include "ZonePage.h"
#include "components/ZoneKeyboardDisplay.h"
#include "components/ZoneEditor.h"
#include "components/WaveDisplay.h"

// This is a bummer
#include "proxies/ZoneStateProxy.h"

namespace scxt
{
namespace pages
{
ZonePage::ZonePage(SCXTEditor *ed, SCXTEditor::Pages p) : PageBase(ed, p)
{
    waveDisplay = std::make_unique<components::WaveDisplay>(editor, editor);
    zoneKeyboardDisplay = std::make_unique<components::ZoneKeyboardDisplay>(editor, editor);
    zoneEditor = std::make_unique<components::ZoneEditor>(editor);
    addAndMakeVisible(*waveDisplay);
    addAndMakeVisible(*zoneKeyboardDisplay);
    addAndMakeVisible(*zoneEditor);
}
ZonePage::~ZonePage() = default;

void ZonePage::resized()
{
    auto r = getLocalBounds();
    zoneKeyboardDisplay->setBounds(r.withHeight(180));
    r = r.withTrimmedTop(180);
    waveDisplay->setBounds(r.withWidth(400));
    zoneEditor->setBounds(r.withTrimmedLeft(400));
}

void ZonePage::connectProxies()
{
    editor->zoneStateProxy->clients.insert(zoneEditor.get());
    editor->zoneStateProxy->clients.insert(zoneKeyboardDisplay.get());
    // FIX THIS
    editor->uiStateProxies.insert(waveDisplay.get());
}

} // namespace pages
} // namespace scxt
