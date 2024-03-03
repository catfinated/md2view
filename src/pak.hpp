#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stack>
#include <string>
#include <vector>

// https://quakewiki.org/wiki/.pak
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
        Node * parent;

        std::string name;
        std::string path;
        int32_t filepos;
        int32_t filelen;

        std::vector<std::unique_ptr<Node>> children;

        Node const * find(std::string const& name) const;

        Node * find(std::string const& name);

        void insert(Node * child);
    };

    explicit PAK(std::filesystem::path const& fpath);

    std::filesystem::path const& fpath() const { return fpath_; }

    Node const * find(std::string const&) const;

    template <typename Visitor>
    void visit(Visitor const& v) // preorder
    {
        std::stack<Node *> stack;
        stack.push(&root_);

        while (!stack.empty()) {
            auto curr = stack.top();
            stack.pop();
            v(curr);

            for (auto& child : curr->children) {
                stack.push(child.get());
            }
        }
    }

private:
    [[nodiscard]] bool init();

    std::filesystem::path fpath_;
    Node root_;
};
