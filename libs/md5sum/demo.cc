#include "md5.h"
#include <iostream>
#include <optional>

using md5::MD5;

#if not __linux__
std::optional<std::string> hash_file(const std::string_view &file_name) {
    auto *fp = std::fopen(file_name.data(), "rb");
    if (!fp) {
        std::perror("File open failed");
        return std::nullopt;
    }

    MD5 md5;
    size_t len;
    char buffer[BUFSIZ];
    while ((len = std::fread(buffer, sizeof(char), BUFSIZ, fp)) > 0) {
        md5.Update(buffer, len);
    }
    if (std::ferror(fp)) {
        std::perror("File read failed");
        std::fclose(fp);
        return std::nullopt;
    }
    std::fclose(fp);
    return md5.Final().Digest();
}

#else

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

std::optional<std::string> hash_file(const std::string_view &file_name) {
    auto fd = open(file_name.data(), O_RDONLY);
    if (fd < 0) {
        std::perror("File open failed");
        return std::nullopt;
    }

    struct stat st {};
    fstat(fd, &st);
    auto file_size = st.st_size;

    auto ptr = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (ptr == MAP_FAILED) {
        std::perror("File mapping failed");
        return std::nullopt;
    }
    auto result = MD5::Hash(ptr, file_size);
    if (munmap(ptr, file_size)) {
        std::perror("File unmapping failed");
    }
    return result;
}
#endif

constexpr auto NO_ERROR = 0;
constexpr auto ARG_ERROR = 1;
constexpr auto FILE_ERROR = 2;

int main(int argc, char *argv[]) {
    if (argc == 1) {
        std::cout << "Usage: " << argv[0] << " [FILE]" << std::endl;
        return NO_ERROR;
    } else if (argc != 2) {
        std::cout << "Invalid MD5 arguments" << std::endl;
        return ARG_ERROR;
    }

    if (auto result = hash_file(argv[1]); result.has_value()) {
        std::cout << result.value() << std::endl;
        return NO_ERROR;
    }
    return FILE_ERROR;
}
