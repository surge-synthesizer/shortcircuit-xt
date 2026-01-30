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

#include <iostream>
#include <memory>
#include "sst/plugininfra/version_information.h"

#include "utils.h"

#include "sample/exs_support/exs_import.h"

#include "RIFF.h"

void dumpList(RIFF::List *list, const std::string &pfx = "+-- ")
{
    RIFF::Chunk *ck = list->GetFirstSubChunk();
    while (ck != NULL)
    {
        RIFF::Chunk *ckParent = ck;
        std::ostringstream oss;
        while (ckParent->GetParent() != NULL)
        {
            oss << "  "; // e.g. 'LIST(INFO)->'
            ckParent = ckParent->GetParent();
        }
        oss << ck->GetChunkIDString();
        switch (ck->GetChunkID())
        {
        case CHUNK_ID_LIST:
        case CHUNK_ID_RIFF:
        {
            RIFF::List *l = (RIFF::List *)ck;
            oss << "(" << l->GetListTypeString() << ")->";
            oss << " (" << l->GetSize() << " Bytes)";
            SCLOG_IF(cliTools, oss.str());
            dumpList(l);
            break;
        }
        default:
            oss << ";";
            oss << " (" << ck->GetSize() << " Bytes)";
            SCLOG_IF(cliTools, oss.str());
            break;
        }
        ck = list->GetNextSubChunk();
    }
};

int main(int argc, char **argv)
{
    SCLOG_IF(cliTools, "RIFF-DUMP");
    SCLOG_IF(cliTools,
             "    Version   = " << sst::plugininfra::VersionInformation::git_implied_display_version
                                << " / "
                                << sst::plugininfra::VersionInformation::project_version_and_hash);
    if (argc < 2)
    {
        SCLOG_IF(cliTools, "Usage: " << argv[0] << " <exs-file>");
        return 1;
    }
    SCLOG_IF(cliTools, "Opening " << argv[1]);

    try
    {
        auto rf = std::make_unique<RIFF::File>(argv[1]);

        SCLOG_IF(cliTools, "RIFF : " << rf->GetChunkIDString() << " " << rf->GetListTypeString());
        dumpList(rf.get());
    }
    catch (const RIFF::Exception &e)
    {
        SCLOG_IF(cliTools, "Exception: " << e.Message);
    }
    return 0;
}