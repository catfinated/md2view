#pragma once

#include <boost/algorithm/string/predicate.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/map.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

// https://quakewiki.org/wiki/.pak
// Supports loading from a real .pak file
// or treating a directory as though it were a .pak file 
class PAK
{
public:
    struct Node
    {
        std::string name;
        std::string path;
        int32_t filepos;
        int32_t filelen;
    };

    explicit PAK(std::filesystem::path const& fpath);

    std::filesystem::path const& fpath() const noexcept { return fpath_; }

    [[nodiscard]] bool is_directory() const noexcept { return fpath_.extension() != ".pak"; }

    std::ifstream open_ifstream(std::filesystem::path const& fpath) const;

    [[nodiscard]] bool has_models() const noexcept { return !models().empty(); }

    auto models() const 
    {
        return entries() | ranges::views::values | ranges::views::filter([](auto const& n) { 
            return boost::algorithm::ends_with(n.path, ".md2"); });
    }
    
private:
    [[nodiscard]] bool init();
    [[nodiscard]] bool init_from_file();
    void init_from_directory();
    std::unordered_map<std::string, Node> const& entries() const { return entries_; }

    std::filesystem::path fpath_;
    std::unordered_map<std::string, Node> entries_;
};
