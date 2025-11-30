/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_BROWSER_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_BROWSER_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/selection_traits.h"
#include "selection/selection_manager.h"
#include "engine/engine.h"
#include "client_macros.h"
#include "filesystem/import.h"
#include "browser/browser.h"
#include "voice/preview_voice.h"

namespace scxt::messaging::client
{

using addBrowserLocation_t = std::tuple<std::string, bool>;
inline void doAddBrowserDeviceLocation(const addBrowserLocation_t &pl, const engine::Engine &engine,
                                       MessageController &cont)
{
    auto p = fs::path(fs::u8path(std::get<0>(pl)));
    auto idx = std::get<1>(pl);
    engine.getBrowser()->addRootPathForDeviceView(p, idx, cont);
}
CLIENT_TO_SERIAL(AddBrowserDeviceLocation, c2s_add_browser_device_location, addBrowserLocation_t,
                 doAddBrowserDeviceLocation(payload, engine, cont));

inline void doReindexBrowserLocation(const std::string &p, const engine::Engine &engine)
{
    auto path = fs::path(fs::u8path(p));
    engine.getBrowser()->reindexLocation(path);
}
CLIENT_TO_SERIAL(ReindexBrowserLocation, c2s_reindex_browser_location, std::string,
                 doReindexBrowserLocation(payload, engine))

inline void doRequestBrowserUpdate(MessageController &cont)
{
    serializationSendToClient(s2c_refresh_browser, true, cont);
}
CLIENT_TO_SERIAL(RequestBrowserUpdate, c2s_request_browser_update, bool,
                 doRequestBrowserUpdate(cont));

inline void doRemoveBrowserDeviceLocation(const fs::path &p, const engine::Engine &engine,
                                          MessageController &cont)
{
    engine.getBrowser()->removeRootPathForDeviceView(p, cont);
}
CLIENT_TO_SERIAL(RemoveBrowserDeviceLocation, c2s_remove_browser_device_location, std::string,
                 doRemoveBrowserDeviceLocation(fs::path(fs::u8path(payload)), engine, cont));

using browserQueueRefreshPayload_t = std::pair<int32_t, int32_t>;
inline void doBrowserQueueRefresh(browserQueueRefreshPayload_t payload,
                                  const engine::Engine &engine)
{
    if (payload.first >= 0 || payload.second >= 0)
    {
        serializationSendToClient(s2c_send_browser_queuelength, payload,
                                  *engine.getMessageController());
    }
}
CLIENT_TO_SERIAL(BrowserQueueRefresh, c2s_browser_queue_refresh, browserQueueRefreshPayload_t,
                 doBrowserQueueRefresh(payload, engine));

SERIAL_TO_CLIENT(SendBrowserQueueLength, s2c_send_browser_queuelength, browserQueueRefreshPayload_t,
                 onBrowserQueueLengthRefresh)

using previewBrowserSamplePayload_t = std::tuple<bool, float, sample::Sample::SampleFileAddress>;
inline void doPreviewBrowserSample(const previewBrowserSamplePayload_t &p,
                                   const engine::Engine &engine, MessageController &cont)
{
    auto [startstop, amplitude, sampleAddress] = p;

    if (startstop)
    {
        auto sid = engine.getSampleManager()->loadSampleByFileAddress(sampleAddress, {});

        if (sid.has_value())
        {
            auto smp = engine.getSampleManager()->getSample(*sid);
            cont.scheduleAudioThreadCallback(
                [smp, amplitude](auto &eng) {
                    eng.previewVoice->attachAndStartUnlessPlaying(smp, amplitude);
                },
                [](const auto &e) { e.getSampleManager()->purgeUnreferencedSamples(); });
        }
        else
        {
            cont.reportErrorToClient("Unable to launch preview",
                                     "Sample file preview load failed for " +
                                         sampleAddress.path.u8string());
        }
    }
    else
    {
        cont.scheduleAudioThreadCallback(
            [](auto &eng) { eng.previewVoice->detatchAndStop(); },
            [](const auto &e) { e.getSampleManager()->purgeUnreferencedSamples(); });
    }
    engine.getSampleManager()->purgeUnreferencedSamples();
}
CLIENT_TO_SERIAL(PreviewBrowserSample, c2s_preview_browser_sample, previewBrowserSamplePayload_t,
                 doPreviewBrowserSample(payload, engine, cont));

SERIAL_TO_CLIENT(RefreshBrowser, s2c_refresh_browser, bool, onBrowserRefresh)

} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_BROWSER_MESSAGES_H
