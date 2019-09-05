#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <stack>
#include <string>
#include <vector>

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

        Node const * find(std::string const& name) const
        {
            auto citer = std::find_if(children.begin(), children.end(),
                                      [&name](std::unique_ptr<Node> const& child) {
                                          return child->name == name; });

            if (citer != children.end()) {
                return citer->get();
            }

            return nullptr;
        }

        Node * find(std::string const& name)
        {
            auto cnode = const_cast<Node const *>(this)->find(name);
            return const_cast<Node *>(cnode);
        }

        void insert(Node * child)
        {
            assert(child);
            child->parent = this;
            children.emplace_back(child);
        }
    };

    explicit PAK(std::string const&);

    std::string const& filename() const { return filename_; }

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
    bool init(std::string const& filename);

    std::string filename_;
    Node root_;
};
