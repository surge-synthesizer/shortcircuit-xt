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

#include <cassert>
#include <chrono>

#include "sst/basic-blocks/modulators/Transport.h"

#include "scxt-plugin.h"
#include "sst/plugininfra/version_information.h"
#include "app/SCXTEditor.h"
#include "sst/basic-blocks/modulators/TransportClapAdapter.h"

namespace scxt::clap_first::scxt_plugin
{
const clap_plugin_descriptor *getDescription()
{
    static const char *features[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SAMPLER,
                                     CLAP_PLUGIN_FEATURE_SYNTHESIZER, "Free and Open Source",
                                     nullptr};
    static clap_plugin_descriptor desc = {
        CLAP_VERSION,
        "org.surge-synth-team.shortcircuit-xt",
        "Shortcircuit XT",
        "Surge Synth Team",
        "https://surge-synth-team.org",
        "",
        "",
        sst::plugininfra::VersionInformation::project_version_and_hash,
        "The Flagship Creative Sampler from the Surge Synth Team",
        &features[0]};
    return &desc;
}

const clap_plugin *makeSCXTPlugin(const clap_host *host)
{
    auto p = new scxt::clap_first::scxt_plugin::SCXTPlugin(host);
    return p->clapPlugin();
}

SCXTPlugin::SCXTPlugin(const clap_host *h) : plugHelper_t(getDescription(), h)
{
    engine = std::make_unique<scxt::engine::Engine>();
    engine->getMessageController()->passWrapperEventsToWrapperQueue = true;
    engine->runningEnvironment = std::string(h->name) + " " + std::string(h->version);
    engine->getMessageController()->requestHostCallback = [this, h](uint64_t flag) {
        if (h)
        {
            // FIXME - atomics better here
            this->nextMainThreadAction |= flag;
            h->request_callback(h);
        }
    };

    clapJuceShim = std::make_unique<sst::clap_juce_shim::ClapJuceShim>(this);
    clapJuceShim->setResizable(true);
}

SCXTPlugin::~SCXTPlugin() { engine.reset(nullptr); }

std::unique_ptr<juce::Component> SCXTPlugin::createEditor()
{
    auto ed = std::make_unique<scxt::ui::app::SCXTEditor>(
        *(engine->getMessageController()), *(engine->defaults), *(engine->getSampleManager()),
        *(engine->getBrowser()), engine->sharedUIMemoryState);
    ed->onZoomChanged = [this](auto f) {
        if (clapJuceShim->isEditorAttached())
        {
            auto s = f * clapJuceShim->getGuiScale();
            guiSetSize(scxt::ui::app::SCXTEditor::edWidth * s,
                       scxt::ui::app::SCXTEditor::edHeight * s);
            if (_host.canUseGui())
            {
                _host.guiRequestResize(scxt::ui::app::SCXTEditor::edWidth * s,
                                       scxt::ui::app::SCXTEditor::edHeight * s);
            }
        }
    };
    onShow = [e = ed.get()]() {
        e->setZoomFactor(e->zoomFactor);
        return true;
    };
    ed->setSize(scxt::ui::app::SCXTEditor::edWidth * ed->zoomFactor,
                scxt::ui::app::SCXTEditor::edHeight * ed->zoomFactor);

    ed->clapHost = _host.host();
    return ed;
}

bool SCXTPlugin::stateSave(const clap_ostream *ostream) noexcept
{
    engine->getSampleManager()->purgeUnreferencedSamples();
    engine->prepareToStream();
    try
    {
        auto sg = scxt::engine::Engine::StreamGuard(engine::Engine::FOR_DAW);
        auto xml = scxt::json::streamEngineState(*engine);

        auto c = xml.c_str();
        auto s = xml.length() + 1; // write the null terminator
        while (s > 0)
        {
            auto r = ostream->write(ostream, c, s);
            if (r < 0)
                return false;
            s -= r;
            c += r;
        }
    }
    catch (const std::runtime_error &err)
    {
        SCLOG_IF(warnings, "Streaming exception [" << err.what() << "]");
        return false;
    }
    return true;
}

bool SCXTPlugin::stateLoad(const clap_istream *istream) noexcept
{
    SCLOG_IF(plugin, "Loading state");
    static constexpr uint32_t initSize = 1 << 16, chunkSize = 1 << 8;
    std::vector<char> buffer;
    buffer.resize(initSize);

    int64_t rd{0};
    int64_t totalRd{0};
    auto bp = buffer.data();

    while ((rd = istream->read(istream, bp, chunkSize)) > 0)
    {
        bp += rd;
        totalRd += rd;
        if (totalRd >= buffer.size() - chunkSize - 1)
        {
            buffer.resize(buffer.size() * 2);
            bp = buffer.data() + totalRd;
        }
    }
    buffer[totalRd] = 0;
    if (totalRd == 0)
    {
        SCLOG_IF(warnings, "Received stream size 0. Invalid state");
        return false;
    }

    auto data = std::string(buffer.data());

    engine->getMessageController()->threadingChecker.bypassThreadChecks = true;
    SCLOG_IF(plugin, "State has size " << buffer.size());

    synchronousEngineUnstream(engine, data);
    SCLOG_IF(plugin, "Back from unstream");

    scxt::messaging::client::clientSendToSerialization(
        scxt::messaging::client::RequestHostCallback{(uint64_t)RESCAN_PARAM_IVT},
        *engine->getMessageController());

    engine->getMessageController()->threadingChecker.bypassThreadChecks = false;
    return true;
}

uint32_t SCXTPlugin::audioPortsCount(bool isInput) const noexcept
{
    if (isInput)
        return 0;
    else
        return numPluginOutputs;
}

bool SCXTPlugin::audioPortsInfo(uint32_t index, bool isInput,
                                clap_audio_port_info *info) const noexcept
{
    if (isInput)
        return false;

    if (index == 0)
    {
        info->id = 0;
        info->in_place_pair = CLAP_INVALID_ID;
        strncpy(info->name, "Main Out", sizeof(info->name));
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->channel_count = 2;
        info->port_type = CLAP_PORT_STEREO;
    }
    else if (index < numPluginOutputs)
    {
        info->id = 1000 + index - 1;
        info->in_place_pair = CLAP_INVALID_ID;
        snprintf(info->name, sizeof(info->name) - 1, "Out %d", index);
        info->flags = 0;
        info->channel_count = 2;
        info->port_type = CLAP_PORT_STEREO;
    }

    return true;
}

uint32_t SCXTPlugin::notePortsCount(bool isInput) const noexcept { return isInput ? 1 : 0; }
bool SCXTPlugin::notePortsInfo(uint32_t index, bool isInput,
                               clap_note_port_info *info) const noexcept
{
    if (isInput)
    {
        info->id = 1;
        info->supported_dialects =
            CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_MIDI_MPE | CLAP_NOTE_DIALECT_CLAP;
        info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
        strncpy(info->name, "Note Input", CLAP_NAME_SIZE - 1);
        return true;
    }
    else
    {
        return false;
    }
    return true;
}

clap_process_status SCXTPlugin::process(const clap_process *process) noexcept
{
#if BUILD_IS_DEBUG
    engine->getMessageController()->threadingChecker.registerAsAudioThread();
#endif
    float **out = process->audio_outputs[0].data32;
    auto chans = process->audio_outputs->channel_count;
    if (chans != 2)
    {
        return CLAP_PROCESS_SLEEP;
    }

    sst::basic_blocks::modulators::fromClapTransport(engine->transport, process->transport);
    engine->onTransportUpdated();

    auto &ptch = engine->getPatch();
    auto &main = ptch->busses.mainBus.output;

    auto ev = process->in_events;
    auto sz = ev->size(ev);

    auto ov = process->out_events;

    const clap_event_header_t *nextEvent{nullptr};
    uint32_t nextEventIndex{0};
    if (sz != 0)
    {
        nextEvent = ev->get(ev, nextEventIndex);
    }

    for (auto s = 0U; s < process->frames_count; ++s)
    {
        if (blockPos == 0)
        {
            // Only realy need to run events when we do the block process
            while (nextEvent && nextEvent->time <= s)
            {
                handleEvent(nextEvent);
                nextEventIndex++;
                if (nextEventIndex < sz)
                    nextEvent = ev->get(ev, nextEventIndex);
                else
                    nextEvent = nullptr;
            }

            engine->processAudio();
            engine->transport.timeInBeats += (double)scxt::blockSize * engine->transport.tempo *
                                             engine->getSampleRateInv() / 60.f;

            bool tryToDrain{true};
            while (tryToDrain &&
                   !engine->getMessageController()->engineToPluginWrapperQueue.empty())
            {
                auto msgopt = engine->getMessageController()->engineToPluginWrapperQueue.pop();
                if (!msgopt.has_value())
                {
                    tryToDrain = false;
                    break;
                }

                switch (msgopt->id)
                {
                case messaging::audio::s2a_param_beginendedit:
                {
                    auto begin = msgopt->payload.mix.u[0];
                    auto parm = msgopt->payload.mix.u[1];
                    auto val = msgopt->payload.mix.f[0];

                    auto evt = clap_event_param_gesture();
                    evt.header.size = sizeof(clap_event_param_gesture);
                    evt.header.type =
                        (begin ? CLAP_EVENT_PARAM_GESTURE_BEGIN : CLAP_EVENT_PARAM_GESTURE_END);
                    evt.header.time = s;
                    evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                    evt.header.flags = 0;
                    evt.param_id = parm;

                    ov->try_push(ov, &evt.header);
                }
                break;
                case messaging::audio::s2a_param_set_value:
                {
                    auto parm = msgopt->payload.mix.u[0];
                    auto val = msgopt->payload.mix.f[0];

                    auto evt = clap_event_param_value();
                    evt.header.size = sizeof(clap_event_param_value);
                    evt.header.type = (uint16_t)CLAP_EVENT_PARAM_VALUE;
                    evt.header.time = 0; // for now
                    evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                    evt.header.flags = 0;
                    evt.param_id = parm;
                    evt.value = val;

                    ov->try_push(ov, &evt.header);
                }
                break;
                case messaging::audio::s2a_param_refresh:
                {
                    nextMainThreadAction |= RESCAN_PARAM_IVT;
                    _host.requestCallback();
                }
                break;
                default:
                    SCLOG_IF(plugin, "Unexpected message " << msgopt->id);
                    break;
                }
            }
        }

        // TODO: this can be way more efficient and block wise and stuff
        out[0][s] = main[0][blockPos];
        out[1][s] = main[1][blockPos];
        for (int i = 0; i < scxt::numNonMainPluginOutputs; ++i)
        {
            float **pout = process->audio_outputs[i + 1].data32;

            if (pout)
            {
                pout[0][s] = 0.f;
                pout[1][s] = 0.f;
            }
            if (engine->getPatch()->usesOutputBus(i + 1))
            {
                if (pout)
                {
                    pout[0][s] = engine->getPatch()->busses.pluginNonMainOutputs[i][0][blockPos];
                    pout[1][s] = engine->getPatch()->busses.pluginNonMainOutputs[i][1][blockPos];
                }
            }
        }

        blockPos = (blockPos + 1) & (scxt::blockSize - 1);
    }

    // CLean up past-last-process events since we only sweep when processing in main loop to avoid
    // per sample if
    while (nextEvent)
    {
        handleEvent(nextEvent);
        nextEventIndex++;
        if (nextEventIndex < sz)
            nextEvent = ev->get(ev, nextEventIndex);
        else
            nextEvent = nullptr;
    }

    assert(!nextEvent);

    return CLAP_PROCESS_CONTINUE;
}

bool SCXTPlugin::handleEvent(const clap_event_header_t *nextEvent)
{
    /*
     * This body is a hack until we do the voice manager
     */
    if (nextEvent->space_id == CLAP_CORE_EVENT_SPACE_ID)
    {
        switch (nextEvent->type)
        {
        case CLAP_EVENT_MIDI:
        {
            auto mevt = reinterpret_cast<const clap_event_midi *>(nextEvent);
            engine->processMIDI1Event(mevt->port_index, mevt->data);
        }
        break;

        case CLAP_EVENT_NOTE_ON:
        {
            auto nevt = reinterpret_cast<const clap_event_note *>(nextEvent);
            engine->processNoteOnEvent(nevt->port_index, nevt->channel, nevt->key, nevt->note_id,
                                       nevt->velocity, 0.f);
        }
        break;

        case CLAP_EVENT_NOTE_OFF:
        {
            auto nevt = reinterpret_cast<const clap_event_note *>(nextEvent);
            engine->processNoteOffEvent(nevt->port_index, nevt->channel, nevt->key, nevt->note_id,
                                        nevt->velocity);
        }

        case CLAP_EVENT_PARAM_VALUE:
        {
            auto pevt = reinterpret_cast<const clap_event_param_value *>(nextEvent);
            handleParamValueEvent(pevt);
        }
        break;

        case CLAP_EVENT_NOTE_EXPRESSION:
        {
            auto nevt = reinterpret_cast<const clap_event_note_expression *>(nextEvent);
            engine->voiceManager.routeNoteExpression(nevt->port_index, nevt->channel, nevt->key,
                                                     nevt->note_id, nevt->expression_id,
                                                     nevt->value);
        }
        break;
        default:
        {
        }
        break;
        }
    }
    return true;
}

bool SCXTPlugin::activate(double sampleRate, uint32_t minFrameCount,
                          uint32_t maxFrameCount) noexcept
{
    engine->prepareToPlay(sampleRate);
    return true;
}

/*
 * Parameter support
 */
uint32_t SCXTPlugin::paramsCount() const noexcept { return scxt::numParts * scxt::macrosPerPart; }

using mac = engine::Macro;

bool SCXTPlugin::paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept
{
    static constexpr int firstMacroIndex{0};
    static constexpr int lastMacroIndex{scxt::numParts * scxt::macrosPerPart};
    if (paramIndex >= firstMacroIndex && paramIndex < lastMacroIndex)
    {
        int p0 = paramIndex - firstMacroIndex;
        int index = p0 % scxt::macrosPerPart;
        int part = p0 / scxt::macrosPerPart;

        info->id = mac::partIndexToMacroID(part, index);
        assert(mac::macroIDToPart(info->id) == part);
        assert(mac::macroIDToIndex(info->id) == index);
        info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        info->cookie = nullptr; // super easy to reverse engineer
        const auto &macro = engine->getPatch()->getPart(part)->macros[index];
        strncpy(info->name, macro.pluginParameterNameFor(part, index).c_str(), CLAP_NAME_SIZE);
        std::string moduleName = "Part " + std::to_string(part + 1) + " / Macros";
        strncpy(info->module, moduleName.c_str(), CLAP_NAME_SIZE);
        info->min_value = 0;
        info->max_value = 1;
        info->default_value = (macro.isBipolar ? 0.5 : 0);
        return true;
    }
    return false;
}

bool SCXTPlugin::paramsValue(clap_id paramId, double *value) noexcept
{
    if (mac::isMacroID(paramId))
    {
        *value = macroFor(paramId).getValue01();

        return true;
    }
    return false;
}

bool SCXTPlugin::paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept
{
    if (mac::isMacroID(paramId))
    {
        *value = macroFor(paramId).value01FromString(display);
        return true;
    }
    return false;
}
bool SCXTPlugin::paramsValueToText(clap_id paramId, double value, char *display,
                                   uint32_t size) noexcept
{
    if (mac::isMacroID(paramId))
    {
        auto fs = macroFor(paramId).value01ToString(value);

        strncpy(display, fs.c_str(), size);
        return true;
    }
    return false;
}
void SCXTPlugin::paramsFlush(const clap_input_events *ev, const clap_output_events *out) noexcept
{
    auto sz = ev->size(ev);
    for (int ei = 0; ei < sz; ++ei)
    {
        auto nextEvent = ev->get(ev, ei);
        if (nextEvent->space_id == CLAP_CORE_EVENT_SPACE_ID &&
            nextEvent->type == CLAP_EVENT_PARAM_VALUE)
        {
            auto pevt = reinterpret_cast<const clap_event_param_value *>(nextEvent);
            handleParamValueEvent(pevt);
        }
    }
}

void SCXTPlugin::handleParamValueEvent(const clap_event_param_value *pev)
{
    if (mac::isMacroID(pev->param_id))
    {
        engine->setMacro01ValueFromPlugin(mac::macroIDToPart(pev->param_id),
                                          mac::macroIDToIndex(pev->param_id), pev->value);
    }
}
void SCXTPlugin::onMainThread() noexcept
{
    // Fixme - better atomics
    uint64_t a = nextMainThreadAction;
    nextMainThreadAction = 0;
    if (a & RESCAN_PARAM_IVT)
    {
        if (_host.canUseParams())
        {
            _host.paramsRescan(CLAP_PARAM_RESCAN_INFO | CLAP_PARAM_RESCAN_VALUES |
                               CLAP_PARAM_RESCAN_TEXT);
        }
    }
}

bool SCXTPlugin::synchronousEngineUnstream(const std::unique_ptr<scxt::engine::Engine> &engine,
                                           const std::string &payload)
{
    auto &cont = engine->getMessageController();
    std::unique_lock<std::mutex> guard(cont->streamNotificationMutex);
    auto originalStreamCount{cont->streamNotificationCount};

    scxt::messaging::client::clientSendToSerialization(
        scxt::messaging::client::UnstreamEngineState{payload}, *engine->getMessageController());

    auto secondsBeforeTimeout{30.0};
    auto waitDuration = std::chrono::milliseconds(100);
    auto maxIterations = secondsBeforeTimeout / 100;

    int iterations = 0;
    while (cont->streamNotificationCount == originalStreamCount && iterations < maxIterations)
    {
        cont->streamNotificationConditionVariable.wait_for(guard, waitDuration);
        iterations++;
    }

    if (iterations >= maxIterations)
        SCLOG_IF(plugin, "State restore took longer than 30 seconds");
    return iterations < maxIterations;
}

} // namespace scxt::clap_first::scxt_plugin