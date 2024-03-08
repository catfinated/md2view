#pragma once

#include <treehh/tree.hh>

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

    std::filesystem::path const& fpath() const { return fpath_; }

    [[nodiscard]] bool is_directory() const { return fpath_.extension() != ".pak"; }

    std::ifstream open_ifstream(std::filesystem::path const& fpath) const;

    std::unordered_map<std::string, Node> const& entries() const { return entries_; }

private:
    [[nodiscard]] bool init();
    [[nodiscard]] bool init_from_file();
    void init_from_directory();

    std::filesystem::path fpath_;
    std::unordered_map<std::string, Node> entries_;
};
