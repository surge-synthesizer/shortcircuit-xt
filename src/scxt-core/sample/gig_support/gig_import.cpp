/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "gig_import.h"

#include "gig.h"

#include "engine/engine.h"
#include "messaging/messaging.h"
#include "infrastructure/md5support.h"
#include "configuration.h"

namespace scxt::gig_support
{
bool importGIG(const fs::path &p, engine::Engine &e, int preset)
{
    auto &messageController = e.getMessageController();
    assert(messageController->threadingChecker.isSerialThread());

    SCLOG_IF(sampleCompoundParsers, "Loading " << p.u8string() << " ps=" << preset);

    auto cng = messaging::MessageController::ClientActivityNotificationGuard(
        "Loading GIG '" + p.filename().u8string() + "'", *(messageController));

    try
    {
        auto riff = std::make_unique<RIFF::File>(p.u8string());
        auto gig = std::make_unique<gig::File>(riff.get());
        auto md5 = infrastructure::createMD5SumFromFile(p);

        auto pt = e.getSelectionManager()->selectedPart;

        auto &part = e.getPatch()->getPart(pt);

        auto spc = 0;
        auto epc = gig->Instruments;

        if (preset >= 0)
        {
            spc = preset;
            epc = preset + 1;
        }

        std::map<gig::Instrument *, int> instToGroup;

        std::map<gig::Sample *, int> sampleToIndex;
        auto smp = gig->GetFirstSample();
        int si = 0;
        while (smp)
        {
            sampleToIndex[smp] = si;
            smp = gig->GetNextSample();
            si++;
        }

        int firstGroup{-1};
        for (int pc = spc; pc < epc; ++pc)
        {
            auto *preset = gig->GetInstrument(pc);
            std::string pnm = "Instrument " + std::to_string(pc + 1);
            if (preset->pInfo)
                pnm = preset->pInfo->Name;

            // For now put it all in one group
            auto grNum = part->addGroup() - 1;
            auto &group = part->getGroup(grNum);
            group->name = pnm;
            if (firstGroup < 0)
                firstGroup = grNum;

            auto region = preset->GetFirstRegion();
            int j = 0;
            while (region)
            {
                if (region->pInfo && !region->pInfo->Name.empty())
                {
                    SCLOG_IF(sampleCompoundParsers, region->pInfo->Name);
                }
                else
                {
                    SCLOG_IF(sampleCompoundParsers, "Unnamed region");
                }
                SCLOG_IF(sampleCompoundParsers, "Dimension Count " << region->Dimensions);

                SCLOG_IF(sampleCompoundParsers, region->KeyGroup);
                SCLOG_IF(sampleCompoundParsers,
                         region->KeyRange.high << " " << region->KeyRange.low);
                auto dim = region->pDimensionRegions[0];

                auto sfsamp = region->GetSample();
                if (sfsamp == nullptr)
                    continue;
                if (sampleToIndex.find(sfsamp) == sampleToIndex.end())
                    continue;
                auto sidx = sampleToIndex[sfsamp];

                // messageController->updateClientActivityNotification("Loading " +
                // p.filename().u8string()+ " sample " +
                //                                                                     std::to_string(j));
                auto sid = e.getSampleManager()->loadSampleFromGIG(p, md5, gig.get(), -1, -1, sidx);
                if (!sid.has_value())
                    continue;
                e.getSampleManager()->getSample(*sid)->md5Sum = md5;

                auto zn = std::make_unique<engine::Zone>(*sid);
                zn->engine = &e;

                zn->mapping.keyboardRange = {region->KeyRange.low, region->KeyRange.high};
                zn->mapping.velocityRange = {region->VelocityRange.low, region->VelocityRange.high};

                zn->mapping.rootKey = dim->UnityNote;

                if (!zn->attachToSample(*(e.getSampleManager())))
                {
                    messageController->reportErrorToClient("GIG Load Error",
                                                           "Zone Unable to Attach to Sample");
                    return false;
                }

                /*
                 * At this point, look at gig_dump.cpp and go ahead and start porting in features
                 * like we did for sf2.
                 */

                group->addZone(zn);
                region = preset->GetNextRegion();
                ++j;
            }
        }

        e.getSelectionManager()->applySelectActions(
            {pt, firstGroup, firstGroup >= 0 ? 0 : -1, true, true, true});
    }
    catch (RIFF::Exception e)
    {
        messageController->reportErrorToClient("GIG Load Error", e.Message);
        return false;
    }
    catch (const SCXTError &e)
    {
        messageController->reportErrorToClient("GIG Load Error", e.what());
        return false;
    }
    catch (...)
    {
        return false;
    }
    return true;
}
} // namespace scxt::gig_support