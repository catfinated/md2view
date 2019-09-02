#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <stack>

#pragma pack(push, 1)
struct PakHeader
{
    std::array<char, 4> id;
    int32_t dirofs;
    int32_t dirlen;
};

struct PakEntry
{
    std::array<char, 56> name;
    int32_t filepos;
    int32_t filelen;
};
#pragma pack(pop)

class PakFile
{
public:
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

    PakFile(std::string const&);

    std::string const& filename() const { return filename_; }

    //bool open(std::string const& filename);
    //std::vector<uint8_t> read_file(std::string const& filename);

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
