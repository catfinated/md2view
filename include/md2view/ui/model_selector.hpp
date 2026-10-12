#pragma once

#include <treehh/tree.hh>

#include <filesystem>
#include <random>
#include <string>

class PAK;

class ModelSelector {
public:
    explicit ModelSelector(PAK const& pak)
        : mt_(std::random_device{}()) {
        init(pak);
    }

    ~ModelSelector() = default;
    ModelSelector(ModelSelector const&) = delete;
    ModelSelector& operator=(ModelSelector const&) = delete;
    ModelSelector(ModelSelector&&) noexcept = default;
    ModelSelector& operator=(ModelSelector&&) noexcept = default;

    [[nodiscard]] std::string modelPath() const;

    /// @brief  Draw the model selection ui
    /// @return True if the selected model changed
    [[nodiscard]] bool drawUi();

    void selectRandomModel();

private:
    struct Node {
        std::string name;
        std::string path;
    };

    void init(PAK const& pak);
    void addNode(std::filesystem::path const& path);

    std::mt19937 mt_;
    tree<Node> tree_;
    tree<Node>::iterator selected_;
};
