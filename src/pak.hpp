#pragma once

#include <treehh/tree.hh>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

// https://quakewiki.org/wiki/.pak
// Supports loading from a real .pak file
// or treating a directory as though it were a .pak file 
class PAK
{
public:
    #pragma pack(push, 1)
    struct Header
    {
        std::array<char, 4> id;
        int32_t dirofs;
        int32_t dirlen;
    };

    struct Entry
    {
        std::array<char, 56> name;
        int32_t filepos;
        int32_t filelen;
    };
    #pragma pack(pop)

    struct Node
    {
        std::string name;
        std::string path;
        int32_t filepos;
        int32_t filelen;
    };

    explicit PAK(std::filesystem::path const& fpath);

    std::filesystem::path const& fpath() const { return fpath_; }

    [[nodiscard]] bool isDirectory() const { return fpath_.extension() != ".pak"; }

    Node const * find(std::string const&) const;

    template <typename Visitor>
    void visit(Visitor const& visit) // preorder
    {
        for (auto const& node : tree_) {
            visit(node);
        }
    }

private:
    [[nodiscard]] bool init();
    [[nodiscard]] bool init_from_file();
    void init_from_directory();

    std::filesystem::path fpath_;
    tree<Node> tree_;
};
