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

#include "sf2_import.h"

#include "gig.h"
#include "SF.h"

#include "engine/engine.h"
#include "messaging/messaging.h"
#include "infrastructure/md5support.h"

namespace scxt::sf2_support
{
bool importSF2(const fs::path &p, engine::Engine &e, int preset)
{
    auto &messageController = e.getMessageController();
    assert(messageController->threadingChecker.isSerialThread());

    SCLOG_IF(sampleLoadAndPurge, "Loading SF2 Preset: " << p.u8string() << " preset=" << preset);

    auto cng = messaging::MessageController::ClientActivityNotificationGuard("Loading SF2",
                                                                             *(messageController));

    try
    {
        auto riff = std::make_unique<RIFF::File>(p.u8string());
        auto sf = std::make_unique<sf2::File>(riff.get());
        auto md5 = infrastructure::createMD5SumFromFile(p);

        auto pt = e.getSelectionManager()->selectedPart;

        auto &part = e.getPatch()->getPart(pt);
        int firstGroup = -1;

        auto spc = 0;
        auto epc = sf->GetPresetCount();

        if (preset >= 0)
        {
            spc = preset;
            epc = preset + 1;
        }

        std::map<sf2::Instrument *, int> instToGroup;

        for (int pc = spc; pc < epc; ++pc)
        {
            auto *preset = sf->GetPreset(pc);
            auto pnm = std::string(preset->GetName());

            for (int i = 0; i < preset->GetRegionCount(); ++i)
            {
                auto *presetRegion = preset->GetRegion(i);
                sf2::Instrument *instr = presetRegion->pInstrument;

                if (instToGroup.find(instr) == instToGroup.end())
                {
                    auto gnum = part->addGroup() - 1;
                    auto &group = part->getGroup(gnum);
                    group->name = std::string(instr->GetName()) + " / " + pnm;
                    instToGroup[instr] = gnum;
                }
                auto grpnum = instToGroup[instr];
                auto &grp = part->getGroup(grpnum);

                if (instr->pGlobalRegion)
                {
                    // TODO: Global Region
                }
                for (int j = 0; j < instr->GetRegionCount(); ++j)
                {
                    auto region = instr->GetRegion(j);
                    auto sfsamp = region->GetSample();
                    if (sfsamp == nullptr)
                        continue;

                    messageController->updateClientActivityNotification("Loading sample " +
                                                                        std::to_string(j));
                    auto sid = e.getSampleManager()->loadSampleFromSF2(p, sf.get(), pc, i, j);
                    if (!sid.has_value())
                        continue;
                    e.getSampleManager()->getSample(*sid)->md5Sum = md5;

                    if (firstGroup < 0)
                        firstGroup = grpnum;
                    auto zn = std::make_unique<engine::Zone>(*sid);
                    zn->engine = &e;

                    if (region->overridingRootKey >= 0)
                    {
                        zn->mapping.rootKey = region->overridingRootKey;
                    }
                    else
                    {
                        zn->mapping.rootKey += sfsamp->OriginalPitch - 60;
                    }

                    auto noneOr = [](auto a, auto b, auto c) {
                        if (a != sf2::NONE)
                            return a;
                        if (b != sf2::NONE)
                            return b;
                        return c;
                    };

                    auto lk =
                        noneOr(region->loKey, presetRegion->loKey, (int)sfsamp->OriginalPitch);
                    auto hk =
                        noneOr(region->hiKey, presetRegion->hiKey, (int)sfsamp->OriginalPitch);
                    zn->mapping.keyboardRange = {lk, hk};

                    auto lv = noneOr(region->minVel, presetRegion->minVel, 0);
                    auto hv = noneOr(region->maxVel, presetRegion->maxVel, 127);
                    zn->mapping.velocityRange = {lv, hv};

                    // SF2 pitch correction is a *signed* char in cents.
                    auto pcv = static_cast<int>(static_cast<int8_t>(sfsamp->PitchCorrection));
                    zn->mapping.pitchOffset = 1.0 * pcv / 100.0;
                    if (!zn->attachToSample(*(e.getSampleManager())))
                    {
                        SCLOG("ERROR: Can't attach to sample");
                        return false;
                    }
                    auto &znSD = zn->variantData.variants[0];

                    auto pn = region->GetPan(presetRegion);
                    // pan in SF2 is -64 to 63 so hackety hack a bit
                    if (pn < 0)
                        zn->mapping.pan = pn / 64.f;
                    else
                        zn->mapping.pan = pn / 63.f;

                    using namespace std;
                    auto s = sfsamp;
                    auto reg = region;

                    zn->mapping.pitchOffset +=
                        reg->GetCoarseTune(presetRegion) + reg->GetFineTune(presetRegion) * 0.01;

                    auto s2a = scxt::modulation::secondsToNormalizedEnvTime;

                    auto sus2l = [](double sus) {
                        auto db = -sus / 10;
                        return pow(10.0, db * 0.05);
                    };

                    zn->egStorage[0].dly = s2a(reg->GetEG1PreAttackDelay(presetRegion));
                    zn->egStorage[0].a = s2a(reg->GetEG1Attack(presetRegion));
                    zn->egStorage[0].h = s2a(reg->GetEG1Hold(presetRegion));
                    zn->egStorage[0].d = s2a(reg->GetEG1Decay(presetRegion));
                    zn->egStorage[0].s = sus2l(reg->GetEG1Sustain(presetRegion));
                    zn->egStorage[0].r = s2a(reg->GetEG1Release(presetRegion));

                    auto GetValue = [](const auto &val) {
                        if (val == sf2::NONE)
                            return ToString("NONE");
                        return ToString(val);
                    };

                    zn->egStorage[1].dly = s2a(reg->GetEG2PreAttackDelay(presetRegion));
                    zn->egStorage[1].a = s2a(reg->GetEG2Attack(presetRegion));
                    zn->egStorage[1].h = s2a(reg->GetEG2Hold(presetRegion));
                    zn->egStorage[1].d = s2a(reg->GetEG2Decay(presetRegion));
                    zn->egStorage[1].s = sus2l(reg->GetEG2Sustain(presetRegion));
                    zn->egStorage[1].r = s2a(reg->GetEG2Release(presetRegion));

                    if (reg->HasLoop)
                    {
                        znSD.loopActive = true;
                        znSD.startLoop = reg->LoopStart;
                        znSD.endLoop = reg->LoopEnd;
                    }
                    else if (presetRegion->HasLoop)
                    {
                        znSD.loopActive = true;
                        znSD.startLoop = presetRegion->LoopStart;
                        znSD.endLoop = presetRegion->LoopEnd;
                    }

                    /*
                     * The SF2 spec says anything above 20khz is a flat response
                     * and that the filter cutoff is specified in cents (so really
                     * 100 * midi key in 12TET). That's a midi key between 135 and 136,
                     * so lets be safe a shave a touch
                     */
                    if (reg->initialFilterFc >= 0 && reg->initialFilterFc < 134 * 100)
                    {
                        // This only runs with audio thread stopped
                        e.getMessageController()->threadingChecker.bypassThreadChecks = true;
                        zn->setProcessorType(0, dsp::processor::ProcessorType::proct_CytomicSVF);
                        zn->processorStorage[0].floatParams[0] =
                            (reg->initialFilterFc / 100.0) - 69.0;
                        zn->processorStorage[0].floatParams[2] =
                            std::clamp((reg->initialFilterQ / 100.0), 0., 1.);
                        e.getMessageController()->threadingChecker.bypassThreadChecks = false;
                    }

                    int currentModRow{0};
                    // SCLOG("Unimplemented SF2 Features Follow: " )
                    if (reg->modEnvToPitch != 0)
                    {
                        zn->routingTable.routes[currentModRow].source =
                            voice::modulation::MatrixEndpoints::Sources::eg2A;
                        zn->routingTable.routes[currentModRow].target =
                            voice::modulation::MatrixEndpoints::MappingTarget::pitchOffsetA;
                        zn->routingTable.routes[currentModRow].depth =
                            1.0 * reg->modEnvToPitch / 19200;
                        zn->onRoutingChanged();
                        currentModRow++;
                    }
                    if (reg->modEnvToFilterFc != 0)
                    {
                        SCLOG("\tModulation Envelope Generator Cutoff="
                              << GetValue(reg->modEnvToFilterFc) << "cents");
                    }

                    if (reg->vibLfoToPitch > 0)
                    {
                        // Use LFO1 as the vibrato LFO
                        zn->modulatorStorage[0].modulatorShape =
                            modulation::ModulatorStorage::LFO_SINE;
                        zn->modulatorStorage[0].rate =
                            (reg->freqVibLfo + 3637.63165623) * 0.01 / 12.0;

                        if (reg->delayVibLfo > -11999)
                        {
                            SCLOG("Unimplemented: Curve Delay");
                            // zn->modulatorStorage[0].curveLfoStorage.useenv = true;
                            // zn->modulatorStorage[0].curveLfoStorage.delay = something;
                            // zn->modulatorStorage[0].curveLfoStorage.attack = 0.0;
                            // zn->modulatorStorage[0].curveLfoStorage.release = 1.0;
                        }

                        // TODO: Fix this with a constant
                        zn->routingTable.routes[currentModRow].source = {'znlf', 'outp', 0};
                        zn->routingTable.routes[currentModRow].target =
                            voice::modulation::MatrixEndpoints::MappingTarget::pitchOffsetA;
                        zn->routingTable.routes[currentModRow].depth =
                            1.0 * reg->vibLfoToPitch / 19200;
                        zn->onRoutingChanged();
                        currentModRow++;
                    }

                    if (reg->modulators.size() > 0)
                    {
                        SCLOG("Unsupported Modulators present in : " << instr->GetName()
                                                                     << " region " << i);
                        SCLOG("\tCount =" << reg->modulators.size() << "");

                        for (int mi = 0; mi < reg->modulators.size(); mi++)
                        {
                            auto m = reg->modulators[mi];
                            SCLOG("\tsrc=" << m.ModSrcOper.Type << " target=" << m.ModDestOper
                                           << " depth=" << m.ModAmount << " trans="
                                           << m.ModTransOper << " srcamt=" << m.ModAmtSrcOper.Type);
                            // PrintModulatorItem(&reg->modulators[i]);
                        }
                    }

                    if (reg->modLfoToFilterFc != 0 || reg->modLfoToPitch != 0)
                    {
                        SCLOG("Unsupported: Modulation LFO: Delay="
                              << ::sf2::ToSeconds(reg->delayModLfo)
                              << "s, Frequency=" << ::sf2::ToHz(reg->freqModLfo)
                              << "Hz, LFO to Volume=" << (reg->modLfoToVolume / 10) << "dB"
                              << ", LFO to Filter Cutoff=" << reg->modLfoToFilterFc
                              << ", LFO to Pitch=" << reg->modLfoToPitch);
                    }

                    if (reg->exclusiveClass)
                        SCLOG("Unsupported: Exclusive Class : " << reg->exclusiveClass << " in "
                                                                << instr->GetName() << " region "
                                                                << i);
                    ;

                    grp->addZone(zn);
                }
            }
        }
        e.getSelectionManager()->selectAction(
            {pt, firstGroup, firstGroup >= 0 ? 0 : -1, true, true, true});
    }
    catch (RIFF::Exception e)
    {
        messageController->reportErrorToClient("SF2 Load Error", e.Message);
        return false;
    }
    catch (const SCXTError &e)
    {
        messageController->reportErrorToClient("SF2 Load Error", e.what());
        return false;
    }
    catch (...)
    {
        return false;
    }
    return true;
}
} // namespace scxt::sf2_support