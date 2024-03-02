#pragma once

#include "md2.hpp"
#include "pak.hpp"

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include <exception>
#include <iostream>
#include <unordered_map>
#include <stack>

class ModelSelector
{
public:
    ModelSelector() = default;

    void init(std::string const& path, EngineBase& eb);

    std::string model_name() const { if (selected_) { return selected_->name; } else { return std::string{}; } }

    MD2& model() { assert(model_); return *model_; }

    void draw_ui();

    template <typename RandEngine>
    void select_random_model(RandEngine& eng);

    PAK const * pak() const { return pak_.get(); }

private:
    struct Node {
        std::string name;
        std::string path;

        bool selected = false;
        Node * parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;
        std::unique_ptr<MD2> model;

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

    template <typename Visitor>
    void preorder(Visitor& v);

    bool load_model_node(Node& n);

    void add_node(boost::filesystem::path const& path);

    std::string path_;
    MD2 * model_ = nullptr;
    Node root_;
    Node * selected_ = nullptr;

    std::unique_ptr<PAK> pak_;
};

inline void ModelSelector::add_node(boost::filesystem::path const& path)
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

inline void ModelSelector::init(std::string const& path, EngineBase& eb)
{
    path_ = path;
    root_.path = path;
    boost::filesystem::path p(path_);

    if (!boost::filesystem::exists(p)) {
        throw std::runtime_error("invalid models path: " + path_);
    }

    if (boost::filesystem::is_directory(p)) {
        boost::filesystem::recursive_directory_iterator iter(p), end;

        for (; iter != end; ++iter ) {
            if (".md2" == iter->path().extension().string()) {
                std::cout << iter->path() << ' ' << iter->path().extension() << '\n';

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
                     if (".md2" == boost::filesystem::path(n->path).extension()) {
                         std::cout << n->path << '\n';
                         this->add_node(boost::filesystem::path(n->path));
                     }
                 });

    }
    else {
        throw std::runtime_error("models must be in dir or a pak file");
    }

    select_random_model(eb.random_engine());
}

template <typename RandEngine>
void ModelSelector::select_random_model(RandEngine& eng)
{
    std::cout << "selecting random\n";

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
                    std::cout << "selected random model=" << selected_->path << '\n';
                    break;
                }
            }
        }

        std::cout << " selected=" << selected_ << " curr=" << curr << " last= " << last_selected << '\n';
    }
}

inline bool ModelSelector::load_model_node(Node& node)
{
    if (node.model) {
        model_ = node.model.get();
        return true;
    }

    std::cout << "loading model " << node.path << '\n';
    auto m = std::unique_ptr<MD2>(new MD2{node.path, pak_.get()});
    node.model = std::move(m);
    model_ = node.model.get();
    std::cout << "loaded model " << node.path << '\n';

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

inline void ModelSelector::draw_ui()
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
                    std::cout << "selected model=" << curr->name << ' ' << curr->path << '\n';

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
