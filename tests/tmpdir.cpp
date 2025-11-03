#include "tmpdir.hpp"

#include <filesystem>
#include <random>

namespace fs = std::filesystem;

TmpDir::TmpDir(std::string const& prefix) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);

    path_ =
        fs::temp_directory_path() / (prefix + "_" + std::to_string(dis(gen)));
    fs::create_directories(path_);
}

TmpDir::~TmpDir() {
    if (fs::exists(path_)) {
        fs::remove_all(path_);
    }
}
