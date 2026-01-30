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

#ifndef SCXT_SRC_SCXT_PLUGIN_CLAP_SCXT_PLUGIN_H
#define SCXT_SRC_SCXT_PLUGIN_CLAP_SCXT_PLUGIN_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <map>
#include <optional>

#include <atomic>
#include <array>
#include <unordered_map>
#include <memory>
#include <random>
#include <tuple>
#include <cstdint>

#include "sst/clap_juce_shim/clap_juce_shim.h"

#include "engine/engine.h"
#include "voice/voice.h"

#include "clap-helpers-config.h"
#include "utils.h"

static_assert((int)scxt::voice::Voice::ExpressionIDs::VOLUME == (int)CLAP_NOTE_EXPRESSION_VOLUME);
static_assert((int)scxt::voice::Voice::ExpressionIDs::PAN == (int)CLAP_NOTE_EXPRESSION_PAN);
static_assert((int)scxt::voice::Voice::ExpressionIDs::TUNING == (int)CLAP_NOTE_EXPRESSION_TUNING);
static_assert((int)scxt::voice::Voice::ExpressionIDs::VIBRATO == (int)CLAP_NOTE_EXPRESSION_VIBRATO);
static_assert((int)scxt::voice::Voice::ExpressionIDs::EXPRESSION ==
              (int)CLAP_NOTE_EXPRESSION_EXPRESSION);
static_assert((int)scxt::voice::Voice::ExpressionIDs::BRIGHTNESS ==
              (int)CLAP_NOTE_EXPRESSION_BRIGHTNESS);
static_assert((int)scxt::voice::Voice::ExpressionIDs::PRESSURE ==
              (int)CLAP_NOTE_EXPRESSION_PRESSURE);

namespace scxt::clap_first::scxt_plugin
{

struct SCXTPlugin : public plugHelper_t, sst::clap_juce_shim::EditorProvider
{
    SCXTPlugin(const clap_host *h);
    ~SCXTPlugin();

    std::unique_ptr<scxt::engine::Engine> engine;
    size_t blockPos{0};

  protected:
    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override;

    /*
     * What can we do on main thread
     */
    enum MainThreadActions : uint64_t
    {
        RESCAN_PARAM_IVT = 1 << 0,
    };
    std::atomic<uint64_t> nextMainThreadAction;
    void onMainThread() noexcept override;

    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override;
    bool audioPortsInfo(uint32_t index, bool isInput,
                        clap_audio_port_info *info) const noexcept override;

    bool implementsNotePorts() const noexcept override { return true; }
    uint32_t notePortsCount(bool isInput) const noexcept override;
    bool notePortsInfo(uint32_t index, bool isInput,
                       clap_note_port_info *info) const noexcept override;

    clap_process_status process(const clap_process *process) noexcept override;
    bool handleEvent(const clap_event_header_t *);

    bool implementsState() const noexcept override { return true; }
    bool stateSave(const clap_ostream *stream) noexcept override;
    bool stateLoad(const clap_istream *stream) noexcept override;

    bool implementsParams() const noexcept override { return true; }
    uint32_t paramsCount() const noexcept override;
    bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept override;
    bool paramsValue(clap_id paramId, double *value) noexcept override;
    bool paramsValueToText(clap_id paramId, double value, char *display,
                           uint32_t size) noexcept override;
    bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept override;
    void paramsFlush(const clap_input_events *in, const clap_output_events *out) noexcept override;

    const scxt::engine::Macro &macroFor(uint32_t paramId) const
    {
        auto part = engine::Macro::macroIDToPart(paramId);
        auto idx = engine::Macro::macroIDToIndex(paramId);
        assert(part >= 0 && part < scxt::numParts);
        assert(idx >= 0 && idx < scxt::macrosPerPart);
        return engine->getPatch()->getPart(part)->macros[idx];
    }

    void handleParamValueEvent(const clap_event_param_value *);

  public:
    bool implementsGui() const noexcept override { return clapJuceShim != nullptr; }
    std::unique_ptr<sst::clap_juce_shim::ClapJuceShim> clapJuceShim;
    ADD_SHIM_IMPLEMENTATION(clapJuceShim)
    ADD_SHIM_LINUX_TIMER(clapJuceShim)
    std::unique_ptr<juce::Component> createEditor() override;

    bool registerOrUnregisterTimer(clap_id &id, int ms, bool reg) override
    {
        if (!_host.canUseTimerSupport())
            return false;
        if (reg)
        {
            _host.timerSupportRegister(ms, &id);
        }
        else
        {
            _host.timerSupportUnregister(id);
        }
        return true;
    }

    bool registerOrUnregisterPosixFd(int fd, clap_posix_fd_flags_t flags, bool reg) override
    {
        if (!_host.canUsePosixFdSupport())
            return false;
        if (reg)
        {
            _host.posixFdSupportRegister(fd, flags);
        }
        else
        {
            _host.posixFdSupportUnregister(fd);
        }
        return true;
    }

    // a few top level non-clap factored functions
    static bool synchronousEngineUnstream(const std::unique_ptr<scxt::engine::Engine> &e,
                                          const std::string &payload);
};

} // namespace scxt::clap_first::scxt_plugin

#endif
