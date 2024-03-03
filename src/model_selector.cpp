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

void ModelSelector::add_node(std::filesystem::path const& path, std::filesystem::path const& root)
{
    auto parent = tree_.begin();
    spdlog::debug("add node for {}", path.string());
    MD2V_EXPECT(path.extension() == ".md2");
    // pak seperator is always '/' but for windows it will be '\'
    auto const sep = root.empty() ? "/" : "\\"; 

    std::string curr;
    for (auto& part : path.lexically_relative(root)) {
        if (!curr.empty()) curr.append(sep); 
        curr += part.string();
        auto fullpath = (root / curr).string();
        auto child = std::find_if(parent, tree_.end(), [&](auto const& node) { return node.path == fullpath; });

        if (child == tree_.end()) {
            Node newNode;
            newNode.name = part.string();
            newNode.path = fullpath;
            spdlog::debug("new node {} {} {}", newNode.name, newNode.path, parent->name);
            child = tree_.append_child(parent, std::move(newNode));
        }
        parent = child;
    }
}

void ModelSelector::init(std::filesystem::path const& path)
{
    selected_ = tree_.end();
    path_ = path;

    if (!std::filesystem::exists(path_)) {
        throw std::runtime_error("invalid models path: " + path_.string());
    }

    Node node;
    node.path = path_.string();
    tree_.insert(tree_.begin(), std::move(node));

    if (std::filesystem::is_directory(path_)) {
        std::filesystem::recursive_directory_iterator iter(path_), end;

        for (; iter != end; ++iter ) {
            if (".md2" == iter->path().extension().string()) {
                spdlog::debug("{} {}", iter->path().string(), iter->path().extension().string());
                add_node(iter->path(), path_);
            }
        }
    }
    else if (".pak" == path_.extension()) {
        pak_ = std::make_unique<PAK>(path_);
        pak_->visit([this](PAK::Node const& pakNode) {
                     if (".md2" == std::filesystem::path(pakNode.path).extension()) {
                         this->add_node(std::filesystem::path(pakNode.path), {});
                     }
                 });

    }
    else {
        throw std::runtime_error("models must be in dir or a pak file");
    }

    select_random_model();
}

 std::string ModelSelector::model_name() const 
 { 
    if (selected_ == tree_.end()) {
        return {};
    } 
    return selected_->name; 
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
    if (ImGui::TreeNode("Select Model")) {
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
        ImGui::TreePop();
    }

    ImGui::TextColored(ImVec4(0.0f, 1.0f ,0.0f, 1.0f), "Model: %s", selected_->path.c_str());

    int index = model().animation_index();

    ImGui::Combo("Animation", &index,
                 [](void * data, int idx, char const ** out_text) -> bool {
                     MD2 const * model = reinterpret_cast<MD2 const *>(data);
                     assert(model);
                     if (idx < 0 || static_cast<size_t>(idx) >= model->animations().size()) { return false; }
                     *out_text = model->animations()[idx].name.c_str();
                     return true;
                 },
                 reinterpret_cast<void *>(&model()),
                 model().animations().size());

    model().set_animation(static_cast<size_t>(index));

    int sindex = model().skin_index();

    ImGui::Combo("Skin", &sindex,
                 [](void * data, int idx, char const ** out_text) -> bool {
                     MD2 const * model = reinterpret_cast<MD2 const *>(data);
                     assert(model);
                     if (idx < 0 || static_cast<size_t>(idx) >= model->skins().size()) { return false; }
                     *out_text = model->skins()[idx].name.c_str();
                     return true;
                 },
                 reinterpret_cast<void *>(&model()),
                 model().skins().size());

    model().set_skin_index(static_cast<size_t>(sindex));

    float fps = model().frames_per_second();
    ImGui::InputFloat("Animation FPS", &fps, 1.0f, 5.0f, "%.3f");
    model().set_frames_per_second(fps);
}
