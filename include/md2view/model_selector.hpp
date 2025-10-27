#pragma once

#include <treehh/tree.hh>

#include <filesystem>
#include <random>

class PAK;

class ModelSelector {
public:
    ModelSelector(PAK const& pak)
        : mt_(std::random_device{}()) {
        init(pak);
    }

    ~ModelSelector() = default;
    ModelSelector(ModelSelector const&) = delete;
    ModelSelector& operator=(ModelSelector const&) = delete;
    ModelSelector(ModelSelector&&) noexcept = default;
    ModelSelector& operator=(ModelSelector&&) noexcept = default;

    std::string model_path() const;

    /// @brief  Draw the model selection ui
    /// @return True if the selected model changed
    [[nodiscard]] bool draw_ui();

    void select_random_model();

private:
    struct Node {
        std::string name;
        std::string path;
    };

    void init(PAK const& pak);
    void add_node(std::filesystem::path const& path);

    std::mt19937 mt_;
    tree<Node> tree_;
    tree<Node>::iterator selected_;
};
