#include "md5.h"
#include "helper.h"
#include "gtest/gtest.h"

using md5::MD5;

TEST(md5sum, stream) {
    const auto test_data = build_test_data(256 * 256);

    MD5 md5;
    for (uint64_t size = 1; size <= 256; ++size) {
        auto expect = MD5::Hash(test_data.data(), size * 256);

        for (int times = 0; times < 256; ++times) {
            const auto offset = test_data.data() + times * size;
            md5.Update(offset, size); // update multiple times
        }
        EXPECT_EQ(md5.Final().Digest(), expect);
        md5.Reset(); // reset for next round

        for (int times = 0; times < 256; ++times) {
            const auto offset = test_data.data() + times * size;
            md5.Update(std::string_view {offset, size}); // update multiple times
        }
        EXPECT_EQ(md5.Final().Digest(), expect);
        md5.Reset(); // reset for next round
    }
}
