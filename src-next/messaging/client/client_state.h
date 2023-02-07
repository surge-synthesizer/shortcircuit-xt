//
// Created by Paul Walker on 2/7/23.
//

#ifndef SCXT_SRC_MESSAGING_CLIENT_CLIENT_STATE_H
#define SCXT_SRC_MESSAGING_CLIENT_CLIENT_STATE_H

namespace scxt::messaging::client
{
struct ClientState
{
    enum Pages
    {
        MULTI_PAGE,
        FX_PAGE
    } displayedPage{MULTI_PAGE};
    
    int selectedPart{0};
    int selectedGroup{0};
    int selectedZone{0};
};
} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_CLIENT_STATE_H
