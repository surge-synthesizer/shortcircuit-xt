/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#ifndef SCXT_SRC_MESSAGING_CLIENT_GROUP_OR_ZONE_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_GROUP_OR_ZONE_MESSAGES_H

namespace scxt::messaging::client
{
// structure here is (forZone, whichEG, active, data)
typedef std::tuple<bool, int, bool, datamodel::AdsrStorage> adsrViewResponsePayload_t;
SERIAL_TO_CLIENT(AdsrGroupOrZoneUpdate, s2c_update_group_or_zone_adsr_view,
                 adsrViewResponsePayload_t, onGroupOrZoneEnvelopeUpdated);

// strcture here is (forZone, whichEG, data)
typedef std::tuple<bool, int, datamodel::AdsrStorage> adsrSelectedGroupOrZoneC2SPayload_t;
inline void adsrSelectedGroupOrZoneUpdate(const adsrSelectedGroupOrZoneC2SPayload_t &payload,
                                          const engine::Engine &engine, MessageController &cont)
{
    // TODO Selected Zone State
    const auto &[forZone, e, adsr] = payload;
    if (forZone)
    {
        auto sz = engine.getSelectionManager()->currentlySelectedZones();
        if (!sz.empty())
        {
            cont.scheduleAudioThreadCallback([zs = sz, ew = e, adsrv = adsr](auto &eng) {
                for (const auto &[p, g, z] : zs)
                {
                    if (ew == 0)
                        eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->aegStorage = adsrv;
                    if (ew == 1)
                        eng.getPatch()->getPart(p)->getGroup(g)->getZone(z)->eg2Storage = adsrv;
                }
            });
        }
    }
    else
    {
        auto sg = engine.getSelectionManager()->currentlySelectedGroups();
        if (!sg.empty())
        {
            cont.scheduleAudioThreadCallback([gs = sg, ew = e, adsrv = adsr](auto &eng) {
                for (const auto &[p, g, z] : gs)
                {
                    eng.getPatch()->getPart(p)->getGroup(g)->gegStorage[ew] = adsrv;
                }
            });
        }
    }
}
CLIENT_TO_SERIAL(AdsrSelectedGroupOrZoneUpdateRequest, c2s_update_group_or_zone_adsr_view,
                 adsrSelectedGroupOrZoneC2SPayload_t,
                 adsrSelectedGroupOrZoneUpdate(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_GROUP_OR_ZONE_MESSAGES_H
