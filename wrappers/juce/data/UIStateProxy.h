//
// Created by Paul Walker on 1/15/22.
//

#ifndef SHORTCIRCUIT_UISTATEPROXY_H
#define SHORTCIRCUIT_UISTATEPROXY_H

#include "sampler_wrapper_actiondata.h"
#include "data/BasicTypes.h"

namespace scxt
{
namespace data
{

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
};

} // namespace data
} // namespace scxt
#endif // SHORTCIRCUIT_UISTATEPROXY_H
