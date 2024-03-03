#pragma once

#include <treehh/tree.hh>

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
    ModelSelector()
      : mt_(std::random_device{}())
      {}

    void init(std::filesystem::path const& path);

    std::string model_name() const;

    MD2& model() const;

    void draw_ui();

    void select_random_model();

    PAK const * pak() const { return pak_.get(); }

private:
    struct Node {
        std::string name;
        std::string path;
        std::unique_ptr<MD2> model;
    };

    void load_model_node(Node& n);

    void add_node(std::filesystem::path const& path, std::filesystem::path const& root);

    std::mt19937 mt_;
    std::filesystem::path path_;
    std::unique_ptr<PAK> pak_;
    tree<Node> tree_;
    tree<Node>::iterator selected_;

};
