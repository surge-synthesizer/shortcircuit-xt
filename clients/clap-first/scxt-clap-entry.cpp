/*
 * Conduit - a project highlighting CLAP-first development
 *           and exercising the surge synth team libraries.
 *
 * Copyright 2023 Paul Walker and authors in github
 *
 * This file you are viewing now is released under the
 * MIT license as described in LICENSE.md
 *
 * The assembled program which results from compiling this
 * project has GPL3 dependencies, so if you distribute
 * a binary, the combined work would be a GPL3 product.
 *
 * Roughly, that means you are welcome to copy the code and
 * ideas in the src/ directory, but perhaps not code from elsewhere
 * if you are closed source or non-GPL3. And if you do copy this code
 * you will need to replace some of the dependencies. Please see
 * the discussion in README.md for further information on what this may
 * mean for you.
 */

/*
 * This file provides the `clap_plugin_entry` entry point required in the DLL for all
 * clap plugins. It also provides the basic functions for the resulting factory class
 * which generates the plugin. In a single plugin case, this really is just plumbing
 * through to expose polysynth::ConduitPolysynthConfig::getDescription() and create a polysynth
 * plugin instance using the helper classes.
 *
 * For more information on this mechanism, see include/clap/entry.h
 */

#include <iostream>
#include <cmath>
#include <cstring>

#include <clap/clap.h>
#include <clap/plugin.h>
#include <clap/factory/plugin-factory.h>

#include "clapwrapper/vst3.h"
#include "clapwrapper/auv2.h"
#include "utils.h"

#include "scxt-plugin/scxt-plugin.h"

namespace scxt::clap_first
{

uint32_t clap_get_plugin_count(const clap_plugin_factory *f) { return 1; }
const clap_plugin_descriptor *clap_get_plugin_descriptor(const clap_plugin_factory *f, uint32_t w)
{
    if (w == 0)
        return scxt::clap_first::scxt_plugin::getDescription();

    return nullptr;
}

static const clap_plugin *clap_create_plugin(const clap_plugin_factory *f, const clap_host *host,
                                             const char *plugin_id)
{

    if (strcmp(plugin_id, scxt::clap_first::scxt_plugin::getDescription()->id) == 0)
    {
        auto p = new scxt::clap_first::scxt_plugin::SCXTPlugin(host);
        return p->clapPlugin();
    }
    return nullptr;
}

static bool clap_get_auv2_info(const clap_plugin_factory_as_auv2 *factory, uint32_t index,
                               clap_plugin_info_as_auv2_t *info)
{
    auto desc = clap_get_plugin_descriptor(nullptr, index); // we don't use the factory above

    auto plugin_id = desc->id;

    info->au_type[0] = 0; // use the features to determine the type

    if (strcmp(plugin_id, scxt::clap_first::scxt_plugin::getDescription()->id) == 0)
    {
        strncpy(info->au_subt, "scCF", 5);
        return true;
    }

    return false;
}

const struct clap_plugin_factory scxt_clap_factory = {
    clap_get_plugin_count,
    clap_get_plugin_descriptor,
    clap_create_plugin,
};

const struct clap_plugin_factory_as_auv2 scxt_auv2_factory = {"SSTx",             // manu
                                                              "Surge Synth Team", // manu name
                                                              clap_get_auv2_info};

static const void *get_factory(const char *factory_id)
{
    if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID))
        return &scxt_clap_factory;

    if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_INFO_AUV2))
        return &scxt_auv2_factory;

    return nullptr;
}

// clap_init and clap_deinit are required to be fast, but we have nothing we need to do here
bool clap_init(const char *p) { return true; }
void clap_deinit() {}

} // namespace scxt::clap_first

extern "C"
{

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes" // other peoples errors are outside my scope
#endif

    // clang-format off
    const CLAP_EXPORT struct clap_plugin_entry clap_entry = {
        CLAP_VERSION,
        scxt::clap_first::clap_init,
        scxt::clap_first::clap_deinit,
        scxt::clap_first::get_factory
    };
    // clang-format on
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}
