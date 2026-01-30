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

#include <miniz.h>
#include "sample/compound_file.h"
#include "browser/browser.h"
#include "infrastructure/md5support.h"

namespace scxt::sample::compound
{
std::vector<CompoundElement> getMultisampleCompoundList(const fs::path &p)
{
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    auto status = mz_zip_reader_init_file(&zip_archive, p.u8string().c_str(), 0);

    if (!status)
        return {};

    auto md5 = std::string("no-md5-for-multisample"); // infrastructure::createMD5SumFromFile(p);

    std::vector<CompoundElement> res;
    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        mz_zip_reader_file_stat(&zip_archive, i, &file_stat);

        if (browser::Browser::isLoadableSingleSample(fs::path(file_stat.m_filename)))
        {
            CompoundElement item;
            item.type = CompoundElement::Type::SAMPLE;
            item.name = file_stat.m_filename;
            item.sampleAddress = {Sample::MULTISAMPLE_FILE, p, md5, -1, -1, i};
            res.push_back(item);
        }
    }
    return res;
}
} // namespace scxt::sample::compound