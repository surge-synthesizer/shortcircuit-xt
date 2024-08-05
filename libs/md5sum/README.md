# MD5

+ ✔︎ Pure C++ interface and implementation.

+ ✔︎ Computational performance is higher than coreutils.

+ ✔︎ Implement complete algorithms based on constant expressions.

+ ✔︎ Complete unit testing and performance benchmark suite.

+ ✔︎ Supports compile-time MD5 hash calculation.

[简体中文](./README_zh-CN.md)

## Quick Start

First, you need to introduce this repository into your project:

```bash
> mkdir my_project && cd ./my_project/
> git clone https://github.com/dnomd343/md5sum.git
```

Create a source code for testing, such as the `main.cc` file:

```c++
#include "md5.h"
#include <iostream>

using md5::MD5;

int main() {
    std::cout << MD5::Hash("hello world") << std::endl;
}
```

Next, you need a CMake configuration, create the `CMakeLists.txt` file:

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_project LANGUAGES CXX)

add_subdirectory(md5sum)

add_executable(my_demo main.cc)
target_link_libraries(my_demo PRIVATE md5sum::md5)
```

Finally, we leave the remaining work to the compiler and execute the following command:

```bash
> cmake -Bcmake-build && cmake --build cmake-build
> ./cmake-build/my_demo
5eb63bbbe01eeed093cb22bb8f5acdc3
```

## Hash Functions

All hash calculation interfaces are concentrated in the `MD5` class, which is divided into two types: direct calculation and streaming update. The former is a unary call, passing in data and getting the hash result, while the latter allows you to pass in the data multiple times and constantly update the hash value, and finally get the result, which is especially useful when calculating hash of large file.

The following interfaces are used for direct calculation and returns the hash result in string form:

```c++
// Calculate the md5 hash value of the specified data.
static std::string Hash(const std::string_view &data);
static std::string Hash(const void *data, uint64_t len);
```

The following interfaces allow streaming calculation of hash values. Use the `Update` interface to pass in data, call the `Final` interface to complete the calculation, and get the hash result in string form through the `Digest` interface. Finally, you may need to call `Reset` for the next round of calculation:

> The return value `MD5&` is a reference to the class itself, which makes chain calling scenarios more convenient.

```c++
// Update md5 hash with specified data.
MD5& Update(const std::string_view &data);
MD5& Update(const void *data, uint64_t len);

// Stop streaming updates and calculate result.
MD5& Final();

// Get the string result of md5.
std::string Digest() const;

// Reset for next round of hashing.
MD5& Reset();
```

Please note that the `Update` interface should no longer be used after calling `Final`. Before the next round of calculation, be sure to call the `Reset` interface, otherwise you will get incorrect results. Here's a simple example:

```c++
#include "md5.h"
#include <iostream>

using md5::MD5;

int main() {
    MD5 hash;

    hash.Update("hello")
        .Update(" ")
        .Update("world")
        .Final();
    std::cout << hash.Digest() << std::endl; // 5eb63bbbe01eeed093cb22bb8f5acdc3

    hash.Reset();
    hash.Update("hello world").Final();
    std::cout << hash.Digest() << std::endl; // 5eb63bbbe01eeed093cb22bb8f5acdc3

    std::cout << MD5::Hash("hello world") << std::endl; // 5eb63bbbe01eeed093cb22bb8f5acdc3
}
```

## Compile-time Hash

This is a very interesting feature. C++ allows us to perform some constant expression calculations during compilation. You can directly pass in constant binary data and get its MD5 constant value.

However, due to compiler limitations, constructing `std::string` as a constant expression is currently not supported. Instead, it returns a result of type `std::array<char, 32>` . The function prototypes are as follows:

```c++
// Calculate the md5 hash value of the specified data with constexpr.
static constexpr std::array<char, 32> HashCE(const std::string_view &data);
static constexpr std::array<char, 32> HashCE(const char *data, uint64_t len);
```

Using constant expressions means that the hashing process will be performed at compile-time and the MD5 result will be recorded as a constant in the compilation product. Below is an example:

```c++
#include "md5.h"
#include <iostream>

using md5::MD5;

int main() {
    constexpr auto my_hash = MD5::HashCE("hello world");
    std::cout << std::string { my_hash.data(), 32 } << std::endl; // 5eb63bbbe01eeed093cb22bb8f5acdc3
}
```

## Unit-Test and Benchmark

For a robust project, unit tests and performance benchmarks are necessary, and `md5sum` also provides these. Before we begin, we need to clone these third-party library codes:

```bash
> cd ./md5sum/
> git submodule update --init
Submodule 'third_party/benchmark' (https://github.com/google/benchmark.git) registered for path 'third_party/benchmark'
Submodule 'third_party/googletest' (https://github.com/google/googletest.git) registered for path 'third_party/googletest'
···
```

Then, execute the following commands to compile:

```bash
> cmake -DMD5_ENABLE_TESTING=ON -DMD5_ENABLE_BENCHMARK=ON -Bcmake-build
> cmake --build cmake-build
```

Let's execute the unit tests:

```bash
> ./cmake-build/md5_test
Running main() from ···
[==========] Running 5 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 5 tests from md5sum
[ RUN      ] md5sum.hash
[       OK ] md5sum.hash (0 ms)
[ RUN      ] md5sum.hash_ce
[       OK ] md5sum.hash_ce (0 ms)
[ RUN      ] md5sum.empty
[       OK ] md5sum.empty (0 ms)
[ RUN      ] md5sum.simple
[       OK ] md5sum.simple (0 ms)
[ RUN      ] md5sum.stream
[       OK ] md5sum.stream (30 ms)
[----------] 5 tests from md5sum (31 ms total)

[----------] Global test environment tear-down
[==========] 5 tests from 1 test suite ran. (31 ms total)
[  PASSED  ] 5 tests.
```

There are also performance benchmark tests:

```bash
> ./cmake-build/md5_benchmark
Running ./cmake-build/md5sum/md5_benchmark
Run on (4 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x2)
  L1 Instruction 32 KiB (x2)
  L2 Unified 1280 KiB (x2)
  L3 Unified 6144 KiB (x1)
Load Average: 1.07, 0.62, 0.85
----------------------------------------------------------
Benchmark                Time             CPU   Iterations
----------------------------------------------------------
MD5_Digest            9.98 ns         9.97 ns     69955773
MD5_Update/64         78.7 ns         78.6 ns      8892747
MD5_Update/256         315 ns          315 ns      2224685
MD5_Update/1024       1259 ns         1258 ns       556361
MD5_Update/4096       5034 ns         5034 ns       139048
MD5_Hash/0             101 ns          101 ns      6883388
MD5_Hash/64            190 ns          190 ns      3686157
MD5_Hash/256           414 ns          414 ns      1690953
MD5_Hash/1024         1358 ns         1358 ns       515488
MD5_Hash/4096         5137 ns         5133 ns       136354
```

These figures mean that on this CPU, it takes about 10 nanoseconds to export an MD5 string, 78.6 nanoseconds to complete a 64-byte update, and 5.133 microseconds to complete a 4 KiB hash calculation.

Hash speed is directly related to the single-core performance of the CPU. In most scenarios, the performance bottleneck lies in the CPU rather than the IO part. If you need to verify a large amount of data, xxHash or BLAKE3 will be a more suitable choice.

## Binary Demo

This project also provides a demonstration sample, which can calculate the MD5 value of a file. You need to use the following command to compile:

```bash
> cmake -DMD5_BUILD_DEMO=ON -Bcmake-build
> cmake --build cmake-build
```

Generate an 8GiB empty file for testing:

```bash
> dd if=/dev/zero of=test.dat bs=1GiB count=8
8+0 records in
8+0 records out
8589934592 bytes (8.6 GB, 8.0 GiB) copied, 6.7279 s, 1.3 GB/s

> du -b test.dat
8589934592	test.dat
```

Count their respective times:

```bash
> time ./cmake-build/md5_demo test.dat
b770351fadae5a96bbaf9702ed97d28d

real    0m10.849s
user    0m10.588s
sys     0m0.260s

> time md5sum test.dat
b770351fadae5a96bbaf9702ed97d28d  test.dat

real    0m11.854s
user    0m10.721s
sys     0m1.132s

> time openssl md5 test.dat
MD5(test.dat)= b770351fadae5a96bbaf9702ed97d28d

real    0m11.497s
user    0m10.243s
sys     0m1.252s
```

## Advanced

The following options are reserved in the project's CMake configuration, and you can switch them as needed.

+ `MD5_BUILD_DEMO` : Whether to build demo binary, disabled by default.

+ `MD5_SHARED_LIB` ：Whether to build as a dynamic library, disabled by default.

+ `MD5_ENABLE_LTO` ：Whether to enable LTO optimization, enabled by default.

+ `MD5_ENABLE_TESTING` ：Whether to build project unit tests, disabled by default.

+ `MD5_ENABLE_BENCHMARK` ：Whether to build performance benchmark suites, disabled by default.

> Note: If you use the Clang compiler and ld linker, since the GNU tools do not understand LLVM bytecode, you need to turn off LTO to link normally, or you can add the `-fuse-ld=lld` option to switch to the lld linker. Generally speaking, Linux users are not recommended to use Clang to compile this project. Under the current performance benchmark, if the `-march=native` optimization is not enabled, in comparisons of Clang18 versus g++12, it typically lags behind by around 20%.

In addition, when building as a dynamic library, symbols inside the project will be hidden, which means that after stripping, only the following symbols are exposed:

> Since part of the hash interfaces are implemented inline in the header file, the `FinalImpl` symbol will be exposed.

```bash
> cmake -DMD5_SHARED_LIB=ON -Bcmake-build
> cmake --build cmake-build
> nm -CD ./cmake-build/libmd5sum.so
                 w __cxa_finalize@GLIBC_2.2.5
                 w __gmon_start__
                 w _ITM_deregisterTMCloneTable
                 w _ITM_registerTMCloneTable
                 U memcpy@GLIBC_2.14
                 U __stack_chk_fail@GLIBC_2.4
0000000000001cc0 T md5::MD5::Update(void const*, unsigned long)
0000000000001840 T md5::MD5::FinalImpl(void const*, unsigned long)
0000000000001a80 T md5::MD5::Digest[abi:cxx11]() const
                 U operator new(unsigned long)@GLIBCXX_3.4
```

## License

MIT ©2024 [@dnomd343](https://github.com/dnomd343)
