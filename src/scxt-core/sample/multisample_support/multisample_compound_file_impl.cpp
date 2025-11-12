//
// Created by Paul Walker on 11/12/25.
//

#include <miniz.h>
#include "sample/compound_file.h"
#include "browser/browser.h"
#include "infrastructure/md5support.h"

namespace scxt::sample::compound
{
std::vector<CompoundElement> getMultisampleCompoundList(const fs::path &p)
{
    SCLOG("Multisample Compound");

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