#pragma once

#include <cassert>
#include <filesystem>
#include <memory>
#include <random>
#include <vector>

class MD2;
class PAK;
class EngineBase;

class ModelSelector
{
public:
    ModelSelector() = default;

    void init(std::string const& path, EngineBase& eb);

    std::string model_name() const { if (selected_) { return selected_->name; } else { return std::string{}; } }

    MD2& model() { assert(model_); return *model_; }

    void draw_ui();

    void select_random_model(std::mt19937& eng);

    PAK const * pak() const { return pak_.get(); }

private:
    struct Node {
        std::string name;
        std::string path;

        bool selected = false;
        Node * parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;
        std::unique_ptr<MD2> model;

        Node const * find(std::string const& name) const;
        Node * find(std::string const& name);
        void insert(Node * child);
    };

    template <typename Visitor>
    void preorder(Visitor& v);

    bool load_model_node(Node& n);

    void add_node(std::filesystem::path const& path);

    std::string path_;
    MD2 * model_ = nullptr;
    Node root_;
    Node * selected_ = nullptr;
    std::unique_ptr<PAK> pak_;
};
