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

#ifndef SCXT_SRC_SCXT_CORE_SAMPLE_IMPORT_SUPPORT_IMPORT_GENERATOR_H
#define SCXT_SRC_SCXT_CORE_SAMPLE_IMPORT_SUPPORT_IMPORT_GENERATOR_H

#include <string>
#include "utils.h"

namespace scxt::engine
{
struct Zone;
}

namespace scxt::import_support
{
class ImporterContext;

// SFZ supports magic sample names of the form `sample=*sine`, `*square`,
// `*saw`, `*triangle` / `*tri`, `*noise`, `*silence`. Returns true if `name`
// matches one of those tokens. Caller should still call
// `installVirtualSample` to actually wire it up.
bool isVirtualSampleName(const std::string &name);

// Configures a zone as a generator zone matching the SFZ `*<name>` token.
// Attaches the engine's synthetic silent sample (so the voice has something
// to play) and, for non-silence tokens, installs a generator processor in
// slot 0 (EllipticBlepWaveforms for sine/saw/square/triangle, TiltNoise
// for noise). The zone's mapping defaults (rootKey/keyboard range) are left
// for the caller. Returns true if `name` was recognized.
bool installVirtualSample(engine::Zone &zone, ImporterContext &ctx, const std::string &name);
} // namespace scxt::import_support

#endif
