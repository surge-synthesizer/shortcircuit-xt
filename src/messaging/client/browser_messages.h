//
// Created by Paul Walker on 7/14/23.
//

#ifndef SHORTCIRCUITXT_BROWSER_MESSAGES_H
#define SHORTCIRCUITXT_BROWSER_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/selection_traits.h"
#include "selection/selection_manager.h"
#include "engine/engine.h"
#include "client_macros.h"
#include "filesystem/import.h"
#include "browser/browser.h"

namespace scxt::messaging::client
{

inline void addDeviceLocation(const fs::path &p, const engine::Engine &engine,
                              MessageController &cont)
{
    engine.getBrowser()->addRootPathForDeviceView(p);
    serializationSendToClient(s2c_refresh_browser, true, cont);
}
CLIENT_TO_SERIAL(AddBrowserDeviceLocation, c2s_browser_add_device_location, std::string,
                 addDeviceLocation(fs::path{payload}, engine, cont));

SERIAL_TO_CLIENT(RefreshBrowser, s2c_refresh_browser, bool, onBrowserRefresh)

} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_BROWSER_MESSAGES_H
