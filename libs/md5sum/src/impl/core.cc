#include "md5.h"
#include <cstring>

using md5::MD5;
using md5::value::K;
using md5::value::S;
using md5::value::T;

#define R1 A, B, C, D
#define R2 D, A, B, C
#define R3 C, D, A, B
#define R4 B, C, D, A

#define F(x, y, z) (z ^ (x & (y ^ z)))
#define G(x, y, z) (y ^ (z & (x ^ y)))
#define H(x, y, z) (x ^ y ^ z)
#define I(x, y, z) (y ^ (x | ~z))

#define MD5_ROUND(i, f, a, b, c, d)           \
    do {                                      \
        a += f(b, c, d) + block[K(i)] + T(i); \
        a = a << S(i) | a >> (32 - S(i));     \
        a += b;                               \
    } while (0)

#ifdef _MSC_VER
#define EXPAND(...) __VA_ARGS__
#define ROUND(...) EXPAND(MD5_ROUND(__VA_ARGS__))
#else
#define ROUND MD5_ROUND
#endif

#define FF(i, ...) ROUND(i | 0x00, F, __VA_ARGS__)
#define GG(i, ...) ROUND(i | 0x10, G, __VA_ARGS__)
#define HH(i, ...) ROUND(i | 0x20, H, __VA_ARGS__)
#define II(i, ...) ROUND(i | 0x30, I, __VA_ARGS__)

#define MD5_UPDATE(OP)                                  \
    OP(0x0, R1); OP(0x1, R2); OP(0x2, R3); OP(0x3, R4); \
    OP(0x4, R1); OP(0x5, R2); OP(0x6, R3); OP(0x7, R4); \
    OP(0x8, R1); OP(0x9, R2); OP(0xa, R3); OP(0xb, R4); \
    OP(0xc, R1); OP(0xd, R2); OP(0xe, R3); OP(0xf, R4);

static constexpr unsigned char Padding[64] { 0x80, /* 0x00, ... */ };

const void* MD5::UpdateImpl(const void *data, uint64_t len) {
    auto *block = static_cast<const uint32_t *>(data);
    auto *limit = block + ((len &= ~0b111111ULL) >> 2);

    auto A = ctx_.A;
    auto B = ctx_.B;
    auto C = ctx_.C;
    auto D = ctx_.D;

    while (block < limit) {
        const auto A_ = A;
        const auto B_ = B;
        const auto C_ = C;
        const auto D_ = D;
        MD5_UPDATE(FF)
        MD5_UPDATE(GG)
        MD5_UPDATE(HH)
        MD5_UPDATE(II)
        A += A_;
        B += B_;
        C += C_;
        D += D_;
        block += 16; // move to next block
    }

    ctx_.A = A;
    ctx_.B = B;
    ctx_.C = C;
    ctx_.D = D;
    ctx_.size += len;
    return limit;
}

void MD5::FinalImpl(const void *data, uint64_t len) {
    if (len >= 120) { // len -> [64 + 56, INF)
        data = UpdateImpl(data, len);
        len &= 0b111111; // len -> [0, 64)
    }

    unsigned char buffer[128]; // 2 blocks
    std::memcpy(buffer, data, len);
    const uint64_t total = (ctx_.size + len) << 3; // total number in bit

    if (len < 56) { // len -> [0, 56)
        std::memcpy(buffer + len, Padding, 56 - len);
        std::memcpy(buffer + 56, &total, 8);
        UpdateImpl(buffer, 64); // update 1 block
    } else { // len -> [56, 64 + 56)
        std::memcpy(buffer + len, Padding, 120 - len);
        std::memcpy(buffer + 120, &total, 8);
        UpdateImpl(buffer, 128); // update 2 blocks
    }
}
