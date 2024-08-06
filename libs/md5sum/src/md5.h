#pragma once

#include <array>
#include <string>
#include <cstdint>

static_assert(sizeof(uintptr_t) == 8,
    "Project only works on 64-bits architecture.");

#ifdef _MSC_VER
static_assert('\x01\x02\x03\x04' == 0x04030201,
#else
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
#endif
    "Project only works on little-endian architecture.");

#if defined(__clang__) || defined(__GNUC__)
#define MD5_EXPORT __attribute__ ((visibility ("default")))
#elif defined(_MSC_VER)
#define MD5_EXPORT __declspec(dllexport)
#else
#define MD5_EXPORT
#endif

#include "impl/value.inl"
#include "impl/constexpr.inl"

namespace md5 {

class MD5 {
public:
    MD5() = default;

    /// Reset for next round of hashing.
    MD5& Reset();

    /// Update md5 hash with specified data.
    MD5& Update(const std::string_view &data);

    /// Update md5 hash with specified data.
    MD5_EXPORT MD5& Update(const void *data, uint64_t len);

    /// Stop streaming updates and calculate result.
    MD5& Final();

    /// Get the string result of md5.
    [[nodiscard]] MD5_EXPORT std::string Digest() const;

    /// Calculate the md5 hash value of the specified data.
    static std::string Hash(const std::string_view &data);

    /// Calculate the md5 hash value of the specified data.
    static std::string Hash(const void *data, uint64_t len);

    /// Calculate the md5 hash value of the specified data with constexpr.
    static constexpr std::array<char, 32> HashCE(const std::string_view &data);

    /// Calculate the md5 hash value of the specified data with constexpr.
    static constexpr std::array<char, 32> HashCE(const char *data, uint64_t len);

private:
    struct md5_ctx {
        uint32_t A = value::kA;
        uint32_t B = value::kB;
        uint32_t C = value::kC;
        uint32_t D = value::kD;
        uint64_t size = 0; // processed size in byte
    };

    md5_ctx ctx_;
    char buffer_[64] {};
    uint64_t buffer_size_ = 0; // size < 64

    /// Update md5 ctx with specified data, and return the pointer of unprocessed data (< 64 bytes).
    const void* UpdateImpl(const void *data, uint64_t len);

    /// Update and final the md5 hash with the specified data.
    MD5_EXPORT void FinalImpl(const void *data, uint64_t len);
};

} // namespace md5

#include "impl/inline.inl"
