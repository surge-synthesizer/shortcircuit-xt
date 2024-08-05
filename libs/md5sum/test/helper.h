#pragma once

#include <string>

inline std::string build_test_data(const uint32_t size) {
    std::string data(size, 0x00);
    for (uint32_t i = 0; i < size; ++i) {
        data[i] = static_cast<char>(i);
    }
    return data;
}

namespace testing::internal {

inline bool operator==(const std::array<char, 32> &s1, const std::string_view &s2) {
    return std::string {s1.data(), 32} == s2;
}

} // namespace testing::internal
