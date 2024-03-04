#include "model_selector.hpp"
#include "common.hpp"
#include "engine.hpp"
#include "md2.hpp"
#include "pak.hpp"

#include <imgui.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <exception>
#include <stack>

void ModelSelector::add_node(std::filesystem::path const& path)
{
    auto parent = tree_.begin();
    spdlog::debug("add node for {}", path.string());
    MD2V_EXPECT(path.extension() == ".md2");
    // pak seperator is always '/'
    auto const sep = "/";

    std::string curr;
    for (auto& part : path) {
        if (!curr.empty()) curr.append(sep); 
        curr += part.string();
        auto fullpath = curr;
        auto child = std::find_if(parent, tree_.end(), [&](auto const& node) { return node.path == fullpath; });

        if (child == tree_.end()) {
            Node newNode;
            newNode.name = part.string();
            newNode.path = fullpath;
            spdlog::debug("new model node {} {} {}", newNode.name, newNode.path, parent->name);
            child = tree_.append_child(parent, std::move(newNode));
        }
        parent = child;
    }
}

void ModelSelector::init(std::filesystem::path const& path)
{
    selected_ = tree_.end();
    path_ = path;
    pak_ = std::make_unique<PAK>(path_);

    Node node;
    node.path = path_.string();
    tree_.insert(tree_.begin(), std::move(node));

    for (auto const& key_value : pak_->entries()) {
        auto const path = std::filesystem::path(key_value.first);
        if (".md2" == path.extension()) {
            add_node(path);
        }
    }

    select_random_model();
}

 std::string ModelSelector::model_name() const 
 { 
    if (selected_ == tree_.end()) {
        return {};
    } 
    return selected_->path; 
 }

 MD2& ModelSelector::model() const
 { 
    assert(selected_ != tree_.end()); 
    assert(selected_->model);
    return *(selected_->model); 
}

void ModelSelector::select_random_model()
{
    spdlog::info("selecting random model");

    std::vector<tree<Node>::iterator> iters;
    for (auto i = tree_.begin_leaf(); i != tree_.end_leaf(); ++i) {
        if (i != selected_) {
            iters.push_back(i);
        }
    }

    if (iters.empty()) return;

    std::uniform_int_distribution<> dist(0, iters.size() - 1);
    auto idx = dist(mt_);
    auto iter = iters[idx];
    spdlog::info("selected random model='{}' '{}'", iter->path, iter->name);
    load_model_node(*iter);
    selected_ = iter;
}

void ModelSelector::load_model_node(Node& node)
{
    if (!node.model) {
        spdlog::debug("loading model {}", node.path);
        node.model = std::make_unique<MD2>(node.path, pak_.get());
        spdlog::info("loaded model {}", node.path);
    }
}

void ModelSelector::draw_ui()
{
    if (ImGui::Button("Random Model")) {
        select_random_model();
    }

    ImGui::Text("%s", path_.string().c_str());
    std::stack<tree<Node>::iterator> stack;

    auto top = tree_.begin();

    // NB: treehh end_fixed does not seem to be working yet
    // https://github.com/kpeeters/tree.hh/blob/master/src/tree.hh#L854
    // we need to go down level be level but only push nodes to the stack
    // if they are expanded in the UI

    for (auto curr = tree_.begin(top); curr != tree_.end(top); ++curr) {
        stack.push(curr);
    }

    unsigned int lastDepth{0};
    while (!stack.empty()) {
        auto curr = stack.top();
        stack.pop();
        auto depth = tree_.depth(curr);

        while (depth < lastDepth && lastDepth > 1) {
            ImGui::TreePop();
            --lastDepth;
        }   
        lastDepth = depth;

        // if curr has no children it is selectable leafat
        ImGuiTreeNodeFlags flags{0};
        auto const is_leaf = tree_.number_of_children(curr) == 0U;

        if (is_leaf) {
            flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (curr == selected_) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }     
        }

        //if (depth == 1) {
        //    ImGui::SetNextItemOpen(true);
        //}

        if (ImGui::TreeNodeEx(curr->name.c_str(), flags)) {
            if (!is_leaf) {
                for (auto iter = tree_.begin(curr); iter != tree_.end(curr); ++iter) {
                    stack.push(iter);
                }
            } else if (ImGui::IsItemClicked()) {
                spdlog::info("selected model={} {}", curr->name, curr->path);
                load_model_node(*curr);
                selected_ = curr;
            }
        }
    }
    while (lastDepth > 1) {
        ImGui::TreePop();
        --lastDepth;
    }
}
