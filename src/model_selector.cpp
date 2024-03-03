#include "model_selector.hpp"
#include "engine.hpp"
#include "md2.hpp"
#include "pak.hpp"

#include <imgui.h>
#include <spdlog/spdlog.h>

#include <cassert>
#include <exception>
#include <stack>

void ModelSelector::add_node(std::filesystem::path const& path)
{
    auto parent = &root_;

    for (auto& part : path) {
        auto child = parent->find(part.string());

        if (!child) {
            if (part.extension() == ".md2") {
                child = new Node{};
                child->name = part.string();
                child->path = path.string();
            }
            else {
                child = new Node{};
                child->name = part.string();
            }
            parent->insert(child);
        }

        parent = child;
        child = nullptr;
    }
}

void ModelSelector::init(std::string const& path, EngineBase& eb)
{
    path_ = path;
    root_.path = path;
    std::filesystem::path p(path_);

    if (!std::filesystem::exists(p)) {
        throw std::runtime_error("invalid models path: " + path_);
    }

    if (std::filesystem::is_directory(p)) {
        std::filesystem::recursive_directory_iterator iter(p), end;

        for (; iter != end; ++iter ) {
            if (".md2" == iter->path().extension().string()) {
                spdlog::info("{} {}", iter->path().string(), iter->path().extension().string());

                auto parent = &root_;

                for (auto& part : iter->path().lexically_relative(p)) {
                    auto child = parent->find(part.string());

                    if (!child) {
                        if (part.extension() == ".md2") {
                            child = new Node{};
                            child->name = part.string();
                            child->path = iter->path().string();
                        }
                        else {
                            child = new Node{};
                            child->name = part.string();
                        }
                        parent->insert(child);
                    }

                    parent = child;
                    child = nullptr;
                }
            }
        }
    }
    else if (".pak" == p.extension()) {
        pak_.reset(new PAK{p.string()});
        pak_->visit([this](PAK::Node const * n) {
                     if (".md2" == std::filesystem::path(n->path).extension()) {
                         spdlog::debug("{}", n->path);
                         this->add_node(std::filesystem::path(n->path));
                     }
                 });

    }
    else {
        throw std::runtime_error("models must be in dir or a pak file");
    }

    select_random_model(eb.random_engine());
}

void ModelSelector::select_random_model(std::mt19937& eng)
{
    spdlog::info("selecting random model");

    auto last_selected = selected_;

    while (selected_ == last_selected) {
        if (selected_) {
            selected_->selected = false;
        }

        Node * curr = &root_;

        while (curr) {
            if (curr->children.empty()) {
                throw std::runtime_error("no models available to select from");
            }

            std::uniform_int_distribution<> dist(0, curr->children.size() - 1);
            auto idx = dist(eng);
            curr = curr->children[idx].get();

            if (curr->children.empty()) {
                if (load_model_node(*curr)) {
                    selected_ = curr;
                    curr->selected = true;
                    spdlog::info("selected random model={}", selected_->path);
                    break;
                }
            }
        }

        //spdlog::info("selected={} curr={} last={}", selected_, curr, last_selected);
    }
}

bool ModelSelector::load_model_node(Node& node)
{
    if (node.model) {
        model_ = node.model.get();
        return true;
    }

    spdlog::info("loading model {}", node.path);
    auto m = std::unique_ptr<MD2>(new MD2{node.path, pak_.get()});
    node.model = std::move(m);
    model_ = node.model.get();
    spdlog::info("loaded model {}", node.path);

    return true;
}

template <typename Visitor>
void ModelSelector::preorder(Visitor& v)
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

void ModelSelector::draw_ui()
{
    if (ImGui::TreeNode("Select Model")) {
        ImGui::Text("%s", path_.c_str());
        std::stack<Node *> stack;

        for (auto& child : root_.children) {
            stack.push(child.get());
        }

        while (!stack.empty()) {
            auto curr = stack.top();
            stack.pop();

            if (!curr) { ImGui::TreePop(); continue; }

            // if curr has no chidlren it is selectable leafat

            if (curr->children.empty()) {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (curr->selected) {
                    flags |=  ImGuiTreeNodeFlags_Selected;
                }
                ImGui::TreeNodeEx(curr->name.c_str(), flags);

                if (ImGui::IsItemClicked()) {
                    spdlog::info("selected model={} {}", curr->name, curr->path);

                    if (load_model_node(*curr)) {
                        if (selected_) {
                            selected_->selected = false;
                        }
                        curr->selected = true;
                        selected_ = curr;
                    }
                }
            }
            else if (ImGui::TreeNode(curr->name.c_str())) {
                stack.push(nullptr);
                for (auto& child : curr->children) {
                    stack.push(child.get());
                }
            }
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

ModelSelector::Node const * ModelSelector::Node::find(std::string const& name) const
{
    auto citer = std::find_if(children.begin(), children.end(),
                                [&name](std::unique_ptr<Node> const& child) {
                                    return child->name == name; });

    if (citer != children.end()) {
        return citer->get();
    }

    return nullptr;
}

ModelSelector::Node * ModelSelector::Node::find(std::string const& name)
{
    auto cnode = const_cast<Node const *>(this)->find(name);
    return const_cast<Node *>(cnode);
}

void ModelSelector::Node::insert(Node * child)
{
    assert(child);
    child->parent = this;
    children.emplace_back(child);
}