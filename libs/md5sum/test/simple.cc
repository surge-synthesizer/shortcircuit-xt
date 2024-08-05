#include "md5.h"
#include "helper.h"
#include "gtest/gtest.h"

using md5::MD5;

TEST(md5sum, empty) {
    constexpr auto expect = "d41d8cd98f00b204e9800998ecf8427e";

    EXPECT_EQ(MD5::Hash(""), expect);
    EXPECT_EQ(MD5::HashCE(""), expect);
    EXPECT_EQ(MD5().Final().Digest(), expect);

    MD5 md5;
    EXPECT_EQ(md5.Reset().Final().Digest(), expect);
    EXPECT_EQ(md5.Reset().Final().Digest(), expect);
    EXPECT_EQ(md5.Reset().Update("").Final().Digest(), expect);
}

TEST(md5sum, simple) {
    constexpr auto expect = "5227827849ea5e9d942ff40dbbfaffd6";

    EXPECT_EQ(MD5::Hash("dnomd343"), expect);
    EXPECT_EQ(MD5::HashCE("dnomd343"), expect);

    MD5 md5;
    EXPECT_EQ(md5.Reset().Update("").Update("dnomd343").Final().Digest(), expect);
    EXPECT_EQ(md5.Reset().Update("dnomd").Update("343").Final().Digest(), expect);
    EXPECT_EQ(md5.Reset().Update("dnomd343").Final().Digest(), expect);
}
