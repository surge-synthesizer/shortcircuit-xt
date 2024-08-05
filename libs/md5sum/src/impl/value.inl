#pragma once

#include "sine.inl"

namespace md5::value {

/// Hexadecimal character mapping table.
constexpr char HexTable[] = {
    '0','1','2','3','4','5','6','7',
    '8','9','a','b','c','d','e','f',
};

/// MD5 fixed constants in little endian.
constexpr uint32_t kA = 0x67452301;
constexpr uint32_t kB = 0xefcdab89;
constexpr uint32_t kC = 0x98badcfe;
constexpr uint32_t kD = 0x10325476;

/// MD5 data block index, input between 0 and 63.
constexpr int K(const int i) {
    constexpr int step[4] = {1, 5, 3, 7};
    constexpr int begin[4] = {0, 1, 5, 0};
    return (begin[i >> 4] + step[i >> 4] * i) & 0b1111;
}
static_assert(K(0) != K(63));

/// MD5 circular shift times, input between 0 and 63.
constexpr int S(const int i) {
    constexpr int shift[4][4] = {
        {7, 12, 17, 22},
        {5, 9, 14, 20},
        {4, 11, 16, 23},
        {6, 10, 15, 21},
    };
    return shift[i >> 4][i & 0b11];
}
static_assert(S(0) != S(63));

/// In order to be compatible with C++17, the `consteval` keyword cannot be used
/// here. The MD5 T-table constants will be macro-expanded and calculated.
#define MD5_TT                                                                      \
    MD5_T(00) MD5_T(01) MD5_T(02) MD5_T(03) MD5_T(04) MD5_T(05) MD5_T(06) MD5_T(07) \
    MD5_T(08) MD5_T(09) MD5_T(0a) MD5_T(0b) MD5_T(0c) MD5_T(0d) MD5_T(0e) MD5_T(0f) \
    MD5_T(10) MD5_T(11) MD5_T(12) MD5_T(13) MD5_T(14) MD5_T(15) MD5_T(16) MD5_T(17) \
    MD5_T(18) MD5_T(19) MD5_T(1a) MD5_T(1b) MD5_T(1c) MD5_T(1d) MD5_T(1e) MD5_T(1f) \
    MD5_T(20) MD5_T(21) MD5_T(22) MD5_T(23) MD5_T(24) MD5_T(25) MD5_T(26) MD5_T(27) \
    MD5_T(28) MD5_T(29) MD5_T(2a) MD5_T(2b) MD5_T(2c) MD5_T(2d) MD5_T(2e) MD5_T(2f) \
    MD5_T(30) MD5_T(31) MD5_T(32) MD5_T(33) MD5_T(34) MD5_T(35) MD5_T(36) MD5_T(37) \
    MD5_T(38) MD5_T(39) MD5_T(3a) MD5_T(3b) MD5_T(3c) MD5_T(3d) MD5_T(3e) MD5_T(3f)

#define MD5_T(x) constexpr auto kT_##x = static_cast<uint32_t>(math::abs(math::sin(0x##x + 1)) * 0x100000000);
MD5_TT
#undef MD5_T

#define MD5_T(x) kT_##x,
constexpr std::array kT = {MD5_TT};
#undef MD5_T

#undef MD5_TT

/// MD5 T-table constant, input between 0 and 63.
constexpr uint32_t T(const int i) {
    return kT[i];
}
static_assert(T(0) != T(63));

} // namespace md5::value
