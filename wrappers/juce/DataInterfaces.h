//
// Created by Paul Walker on 1/4/22.
//

#ifndef SHORTCIRCUIT_DATAINTERFACES_H
#define SHORTCIRCUIT_DATAINTERFACES_H

#include "sampler_wrapper_actiondata.h"

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
    virtual ~UIStateProxy() = default;
    virtual bool processActionData(const actiondata &d) = 0;
    std::unordered_set<juce::Component *> clients;
    void repaintClients()
    {
        for (auto c : clients)
        {
            c->repaint();
        }
    }
    void invalidateClients()
    {
        for (auto c : clients)
        {
            if (auto q = dynamic_cast<Invalidatable *>(c))
            {
                q->onProxyUpdate();
            }
        }
    }
    void invalidateAndRepaintClients()
    {
        invalidateClients();
        repaintClients();
    }
};

struct ActionSender
{
    virtual ~ActionSender() = default;
    virtual void sendActionToEngine(const actiondata &ad) = 0;
};
#endif // SHORTCIRCUIT_DATAINTERFACES_H
