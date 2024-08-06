#pragma once

namespace md5 {

inline MD5& MD5::Reset() {
    ctx_.A = value::kA;
    ctx_.B = value::kB;
    ctx_.C = value::kC;
    ctx_.D = value::kD;
    ctx_.size = 0;
    buffer_size_ = 0;
    return *this;
}

inline MD5& MD5::Final() {
    FinalImpl(buffer_, buffer_size_);
    return *this;
}

inline MD5& MD5::Update(const std::string_view &data) {
    return Update(data.data(), data.size());
}

inline std::string MD5::Hash(const std::string_view &data) {
    return Hash(data.data(), data.size());
}

inline std::string MD5::Hash(const void *data, const uint64_t len) {
    MD5 md5;
    md5.FinalImpl(data, len);
    return md5.Digest();
}

constexpr std::array<char, 32> MD5::HashCE(const std::string_view &data) {
    return HashCE(data.data(), data.size());
}

constexpr std::array<char, 32> MD5::HashCE(const char *data, const uint64_t len) {
    return ce::Hash(data, len);
}

} // namespace md5
