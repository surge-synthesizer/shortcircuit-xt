//
// Created by Paul Walker on 4/4/25.
//

#ifndef ZONERIGHTMOUSEMENU_H
#define ZONERIGHTMOUSEMENU_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "selection/selection_manager.h"
#include "messaging/client/client_messages.h"

namespace scxt::ui::app::shared
{
template <typename SendingComp, typename RenamingComp> // COMP is a HasEditor and a few other things
void populateZoneRightMouseMenuForZone(SendingComp *that, RenamingComp *rnThat, juce::PopupMenu &p,
                                       const selection::SelectionManager::ZoneAddress &forZone,
                                       const std::string &zoneSectionName)
{
    namespace cmsg = scxt::messaging::client;

    p.addSectionHeader(zoneSectionName);
    p.addItem("Rename", [w = juce::Component::SafePointer(rnThat), forZone]() {
        if (!w)
            return;
        w->doZoneRename(forZone);
    });
    p.addItem("Copy", [w = juce::Component::SafePointer(that), forZone]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::CopyZone(forZone));
    });
    p.addItem("Paste", [w = juce::Component::SafePointer(that), forZone]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::PasteZone(forZone));
    });
    p.addItem("Duplicate", [w = juce::Component::SafePointer(that), forZone]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::DuplicateZone(forZone));
    });
    p.addItem("Delete", [w = juce::Component::SafePointer(that), forZone]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::DeleteZone(forZone));
    });
}

template <typename SendingComp>
void populateZoneRightMouseMenuForSelectedZones(SendingComp *that, juce::PopupMenu &p, int part,
                                                const std::string &name = "")
{
    namespace cmsg = scxt::messaging::client;

    if (name.empty())
        p.addSectionHeader("Selected Zones");
    else
        p.addSectionHeader(name);

    p.addItem("Delete All", [w = juce::Component::SafePointer(that)]() {
        if (!w)
            return;

        w->sendToSerialization(cmsg::DeleteAllSelectedZones(true));
    });

    p.addSeparator();
    p.addSectionHeader("Current Part");
    p.addItem("Delete All Zones and Groups", [w = juce::Component::SafePointer(that), part]() {
        if (!w)
            return;

        w->sendToSerialization(cmsg::ClearPart(part));
    });

    if (that->editor->hasMissingSamples)
    {
        p.addSeparator();
        p.addItem("Resolve Missing Samples", [w = juce::Component::SafePointer(that)]() {
            if (!w)
                return;
            w->editor->showMissingResolutionScreen();
        });
    }
}
} // namespace scxt::ui::app::shared

#endif // ZONERIGHTMOUSEMENU_H
