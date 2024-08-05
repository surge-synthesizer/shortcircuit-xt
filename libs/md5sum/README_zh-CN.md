# MD5

+ ✔︎ 纯 C++ 接口及实现。

+ ✔︎ 计算性能比 coreutils 更高。

+ ✔︎ 基于常量表达式实现完整的算法。

+ ✔︎ 完善的单元测试及性能基准套件。

+ ✔︎ 支持编译期 MD5 哈希值计算。

## 快速开始

首先，您需要在项目中引入本仓库：

```bash
> mkdir my_project && cd ./my_project/
> git clone https://github.com/dnomd343/md5sum.git
```

编写一份源代码用于测试，例如 `main.cc` 文件：

```c++
#include "md5.h"
#include <iostream>

using md5::MD5;

int main() {
    std::cout << MD5::Hash("hello world") << std::endl;
}
```

同时，您需要一份 CMake 配置来驱动它，创建 `CMakeLists.txt` 文件：

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_project LANGUAGES CXX)

add_subdirectory(md5sum)

add_executable(my_demo main.cc)
target_link_libraries(my_demo PRIVATE md5sum::md5)
```

最后，我们将剩下的工作交给编译器，执行以下命令：

```bash
> cmake -Bcmake-build && cmake --build cmake-build
> ./cmake-build/my_demo
5eb63bbbe01eeed093cb22bb8f5acdc3
```

## 哈希接口

所有的哈希计算接口都集中在 `MD5` 这个类中，它分为两种：直接计算和流式更新。前者是一元调用，传入数据并得到哈希结果，后者允许您多次将数据传入并不断更新哈希值，并最终得到结果，这在计算大文件哈希时特别有用。

以下接口用于直接计算，返回字符串形式的哈希结果：

```c++
// Calculate the md5 hash value of the specified data.
static std::string Hash(const std::string_view &data);
static std::string Hash(const void *data, uint64_t len);
```

以下接口允许流式计算哈希值，使用 `Update` 接口传入数据，调用 `Final` 接口完成计算，并通过 `Digest` 接口得到字符串形式的哈希结果，最后，您可能需要调用 `Reset` 为下一轮计算初始化：

> 返回值 `MD5&` 是类自身的引用，它使得链式调用场景更为方便。

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

请注意，在调用 `Final` 后不再应该使用 `Update` 接口，在进行下一轮计算前，请务必调用 `Reset` 接口，否则将得到错误的结果。以下是一个简单的示例：

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

## 编译期哈希

这是一个很有趣的特性，C++ 允许我们在编译的时候进行一些常量表达式计算，您可以直接将常量二进制数据传入，并得到它的 MD5 常量值。

不过由于编译器限制，在当前并不支持构造 `std::string` 作为常量表达式，作为替代，它返回 `std::array<char, 32>` 类型的结果，函数原型如下：

```c++
// Calculate the md5 hash value of the specified data with constexpr.
static constexpr std::array<char, 32> HashCE(const std::string_view &data);
static constexpr std::array<char, 32> HashCE(const char *data, uint64_t len);
```

使用常量表达式意味着，哈希过程将在编译期进行，MD5 结果将作为常量记录到编译产物中。下面是一个例子：

```c++
#include "md5.h"
#include <iostream>

using md5::MD5;

int main() {
    constexpr auto my_hash = MD5::HashCE("hello world");
    std::cout << std::string { my_hash.data(), 32 } << std::endl; // 5eb63bbbe01eeed093cb22bb8f5acdc3
}
```

## 测试与基准

对于一个鲁棒的项目，单元测试和性能基准是很有必要的，`md5sum` 同样提供了这些。在开始之前，我们需要先克隆这些第三方库代码：

```bash
> cd ./md5sum/
> git submodule update --init
Submodule 'third_party/benchmark' (https://github.com/google/benchmark.git) registered for path 'third_party/benchmark'
Submodule 'third_party/googletest' (https://github.com/google/googletest.git) registered for path 'third_party/googletest'
···
```

然后，执行以下命令进行编译：

```bash
> cmake -DMD5_ENABLE_TESTING=ON -DMD5_ENABLE_BENCHMARK=ON -Bcmake-build
> cmake --build cmake-build
```

让我们执行单元测试：

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

还有性能基准的测试：

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

这些数据意味着，在这颗 CPU 上，导出一次 MD5 字符串需要约 10 纳秒，完成 64 字节的更新需要 78.6 纳秒，完成 4 KiB 的哈希计算需要 5.133 微秒。

哈希速度与 CPU 的单核性能有直接关系，绝大多数场景下，性能瓶颈在于 CPU 而非 IO 部分。如果您需要校验大量数据，xxHash 或者 BLAKE3 将会是更合适的选择。

## 二进制示例

本项目同时提供了一个演示样例，它可以实现对文件的 MD5 值计算，您需要使用以下命令编译：

```bash
> cmake -DMD5_BUILD_DEMO=ON -Bcmake-build
> cmake --build cmake-build
```

生成一个 8GiB 的空文件用于测试：

```bash
> dd if=/dev/zero of=test.dat bs=1GiB count=8
8+0 records in
8+0 records out
8589934592 bytes (8.6 GB, 8.0 GiB) copied, 6.7279 s, 1.3 GB/s

> du -b test.dat
8589934592	test.dat
```

统计它们各自的用时：

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

## 高级选项

在项目的 CMake 配置中预留了以下选项，您可以按需进行开关。

+ `MD5_BUILD_DEMO` ：是否构建演示二进制，默认关闭

+ `MD5_SHARED_LIB` ：是否构建为动态库，默认关闭

+ `MD5_ENABLE_LTO` ：是否开启 LTO 优化，默认打开

+ `MD5_ENABLE_TESTING` ：是否构建项目单元测试，默认关闭

+ `MD5_ENABLE_BENCHMARK` ：是否构建性能基准套件，默认关闭

> 注意：如果您使用 Clang 编译器和 ld 链接器，由于 GNU 工具并不认识 LLVM 字节码，需要关闭 LTO 才能正常链接，或者您可以增加 `-fuse-ld=lld` 选项切换到 lld 链接器。普遍情况下，Linux 用户并不建议使用 Clang 编译本项目，在当前的性能基准下，如果未开启 `-march=native` 优化，在 Clang18 与 g++12 的对比中，它通常会落后 20% 左右。

此外，在构建为动态库时，项目内部的符号将被隐藏，这意味着 strip 以后，仅有以下符号暴露：

> 由于部分哈希接口在头文件内联实现，因此 `FinalImpl` 符号将对外暴露。

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

## 许可证

MIT ©2024 [@dnomd343](https://github.com/dnomd343)
