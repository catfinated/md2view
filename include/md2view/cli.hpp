#pragma once

#include <spdlog/common.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <span>

/// Command line arguments
struct CliArgs {
    int width{1280};
    int height{800};
    std::optional<std::filesystem::path> pak;
    spdlog::level::level_enum logLevel{spdlog::level::info};
};

/// Parse command line arguments from @p args
///
/// Prints usage and error messages to @p out
///
/// @return nullopt if the program should exit
[[nodiscard]] std::optional<CliArgs> parseCli(std::span<char const*> args,
                                              std::ostream& out = std::cerr);
