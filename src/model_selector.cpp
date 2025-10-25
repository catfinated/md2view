#include "model_selector.hpp"
#include "pak.hpp"

#include <gsl/gsl-lite.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <stack>

void ModelSelector::add_node(std::filesystem::path const& path) {
    auto parent = tree_.begin();
    spdlog::debug("add node for {}", path.string());
    gsl_Assert(path.extension() == ".md2");
    // pak seperator is always '/'
    auto const sep = "/";

    std::string curr;
    for (auto& part : path) {
        if (!curr.empty())
            curr.append(sep);
        curr += part.string();
        auto fullpath = curr;
        auto child = std::find_if(parent, tree_.end(), [&](auto const& node) {
            return node.path == fullpath;
        });

        if (child == tree_.end()) {
            Node newNode;
            newNode.name = part.string();
            newNode.path = fullpath;
            spdlog::debug("new model node {} {} {}", newNode.name, newNode.path,
                          parent->name);
            child = tree_.append_child(parent, std::move(newNode));
        }
        parent = child;
    }
}

void ModelSelector::init(PAK const& pak) {
    selected_ = tree_.end();

    Node node;
    node.path = pak.fpath().string();
    node.name = node.path;
    tree_.insert(tree_.begin(), std::move(node));

    for (auto const& pak_entry : pak.models()) {
        add_node(pak_entry.path);
    }

    select_random_model();
}

std::string ModelSelector::model_path() const {
    if (selected_ == tree_.end()) {
        return {};
    }
    return selected_->path;
}

void ModelSelector::select_random_model() {
    spdlog::info("selecting random model");

    std::vector<tree<Node>::iterator> iters;
    for (auto i = tree_.begin_leaf(); i != tree_.end_leaf(); ++i) {
        if (i != selected_) {
            iters.emplace_back(i);
        }
    }

    if (iters.empty())
        return;

    std::uniform_int_distribution<> dist(0, iters.size() - 1);
    auto idx = dist(mt_);
    auto iter = iters[idx];
    spdlog::info("selected random model='{}' '{}'", iter->path, iter->name);
    selected_ = iter;
}

bool ModelSelector::draw_ui() {
    bool ret = false;
    if (ImGui::Button("Random Model")) {
        select_random_model();
        ret = true;
    }

    // NB: treehh end_fixed does not seem to be working yet
    // https://github.com/kpeeters/tree.hh/blob/master/src/tree.hh#L854
    // we need to go down level be level but only push nodes to the stack
    // if they are expanded in the UI
    std::stack<tree<Node>::iterator> stack;
    stack.push(tree_.begin());

    int lastDepth{0};
    while (!stack.empty()) {
        auto curr = stack.top();
        stack.pop();
        auto depth = tree_.depth(curr);

        while (depth < lastDepth) {
            ImGui::TreePop();
            --lastDepth;
        }
        lastDepth = depth;

        // if curr has no children it is selectable leafat
        ImGuiTreeNodeFlags flags{0};
        auto const is_leaf = tree_.number_of_children(curr) == 0U;

        if (is_leaf) {
            flags =
                ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (curr == selected_) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
        }

        if (depth == 0) {
            ImGui::SetNextItemOpen(true);
        }

        if (ImGui::TreeNodeEx(curr->name.c_str(), flags)) {
            if (!is_leaf) {
                for (auto iter = tree_.begin(curr); iter != tree_.end(curr);
                     ++iter) {
                    stack.emplace(iter);
                }
            } else if (ImGui::IsItemClicked()) {
                spdlog::info("selected model={} {}", curr->name, curr->path);
                selected_ = curr;
                ret = true;
            }
        }
    }
    while (lastDepth > 0) {
        ImGui::TreePop();
        --lastDepth;
    }
    return ret;
}
