#pragma once

#include <filesystem>

class TmpDir {
public:
    TmpDir(std::string const& prefix = "test");
    ~TmpDir();

    TmpDir(TmpDir const&) = delete;
    TmpDir& operator=(TmpDir const&) = delete;

    TmpDir(TmpDir&&) noexcept = default;
    TmpDir& operator=(TmpDir&&) noexcept = default;

    [[nodiscard]] std::filesystem::path const& path() const { return path_; }

private:
    std::filesystem::path path_;
};
