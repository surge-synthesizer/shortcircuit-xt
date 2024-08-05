#pragma once

namespace md5::ce {

struct md5_ctx {
    uint32_t A = value::kA;
    uint32_t B = value::kB;
    uint32_t C = value::kC;
    uint32_t D = value::kD;
};

struct md5_data {
    const char *ptr;
    uint64_t len, padded_len;

    constexpr md5_data(const char *data, const uint64_t len)
        : ptr(data), len(len), padded_len((len + 64 + 8) & ~0b111111ULL) {}
};

using Block = std::array<uint32_t, 16>; // single md5 block with 64 bytes

/// Get the data and padding byte of the specified index.
constexpr uint8_t GetByte(const md5_data &data, const uint64_t index) {
    if (index < data.len) // message data
        return data.ptr[index];
    if (index == data.len) // padding flag
        return 0x80;
    if (index < data.padded_len - 8) // padding content
        return 0x00;
    const auto offset = (index + 8 - data.padded_len) * 8;
    return static_cast<uint8_t>(0xff & (data.len * 8) >> offset);
}

/// Get the MD5 block content at the specified index.
constexpr Block GetBlock(const md5_data &data, const uint64_t index) {
    Block block {};
    for (int i = 0; i < 16; ++i) {
        const auto offset = index + i * 4;
        block[i] |= GetByte(data, offset + 3);
        block[i] <<= 8;
        block[i] |= GetByte(data, offset + 2);
        block[i] <<= 8;
        block[i] |= GetByte(data, offset + 1);
        block[i] <<= 8;
        block[i] |= GetByte(data, offset + 0);
    }
    return block;
}

/// Apply MD5 round process with 64 times calculate.
constexpr md5_ctx Round(const Block &block, md5_ctx ctx) {
    constexpr auto calc = [](const md5_ctx &c, const int i) {
        if (i < 0x10)
            return c.D ^ (c.B & (c.C ^ c.D));
        if (i < 0x20)
            return c.C ^ (c.D & (c.B ^ c.C));
        if (i < 0x30)
            return c.B ^ c.C ^ c.D;
        return c.C ^ (c.B | ~c.D);
    };

    for (int i = 0; i < 64; ++i) {
        const auto a = ctx.A + calc(ctx, i) + block[value::K(i)] + value::T(i);
        ctx.A = ctx.D;
        ctx.D = ctx.C;
        ctx.C = ctx.B;
        ctx.B += a << value::S(i) | a >> (32 - value::S(i));
    }
    return ctx;
}

/// Convert origin MD5 integers to hexadecimal character array.
constexpr std::array<char, 32> DigestCE(const std::array<uint32_t, 4> &ctx) {
    std::array<char, 32> result {};
    for (uint32_t i = 0, val = 0; i < 32; val >>= 8) {
        if (!(i & 0b111))
            val = ctx[i >> 3];
        result[i++] = value::HexTable[(val >> 4) & 0b1111];
        result[i++] = value::HexTable[val & 0b1111];
    }
    return result;
}

/// MD5 hash implement based on constexpr.
constexpr std::array<char, 32> Hash(const char *data, const uint64_t len) {
    md5_ctx ctx;
    const md5_data md5(data, len);
    for (uint32_t index = 0; index < md5.padded_len; index += 64) {
        const auto [A, B, C, D] = Round(GetBlock(md5, index), ctx);
        ctx.A += A;
        ctx.B += B;
        ctx.C += C;
        ctx.D += D;
    }
    return DigestCE({ctx.A, ctx.B, ctx.C, ctx.D});
}

static_assert(Hash("", 0)[0] == 'd');

} // namespace md5::ce
