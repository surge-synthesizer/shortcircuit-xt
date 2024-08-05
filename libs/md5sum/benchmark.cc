#include "md5.h"
#include "benchmark/benchmark.h"

using md5::MD5;

std::string build_test_data() {
    std::string data(65536, 0x00);
    for (uint32_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<char>(i & 0xff);
    }
    return data;
}

static void MD5_Digest(benchmark::State &state) {
    constexpr MD5 md5;
    for (auto _ : state) {
        auto volatile holder = md5.Digest();
    }
}

static void MD5_Update(benchmark::State &state) {
    MD5 md5;
    const auto data = build_test_data();
    for (auto _ : state) {
        md5.Update(data.c_str(), state.range(0));
    }
}

static void MD5_Hash(benchmark::State &state) {
    const auto data = build_test_data();
    for (auto _ : state) {
        MD5::Hash(data.c_str(), state.range(0));
    }
}

BENCHMARK(MD5_Digest);

BENCHMARK(MD5_Update)->RangeMultiplier(4)->Range(64, 4096);

BENCHMARK(MD5_Hash)->Arg(0);
BENCHMARK(MD5_Hash)->RangeMultiplier(4)->Range(64, 4096);

BENCHMARK_MAIN();
