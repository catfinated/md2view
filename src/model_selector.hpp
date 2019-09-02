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

    void init(std::string const& path, blue::EngineBase& eb);

    std::string model_name() const { if (selected_) { return selected_->name; } else { return std::string{}; } }

    blue::md2::Model& model() { assert(model_); return *model_; }

    void draw_ui();

    template <typename RandEngine>
    void select_random_model(RandEngine& eng);

    PakFile const * pak() const { return pak_.get(); }

private:
    struct Node {
        std::string name;
        std::string path;

        bool selected = false;
        Node * parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;
        std::unique_ptr<blue::md2::Model> model;

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
    blue::md2::Model * model_ = nullptr;
    Node root_;
    Node * selected_ = nullptr;

    std::unique_ptr<PakFile> pak_;

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

inline void ModelSelector::init(std::string const& path, blue::EngineBase& eb)
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
        pak_.reset(new PakFile{p.string()});
        pak_->visit([this](PakFile::Node const * n) {
                     //std::cout << n->path << '\n';
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

    boost::filesystem::path p(node.path);
    auto m = std::unique_ptr<blue::md2::Model>(new blue::md2::Model{});

    std::cout << "loading model " << p << '\n';

    if (pak_) {
        BLUE_EXPECT(m->load(*pak_, p.string()));
    }
    else {
        BLUE_EXPECT(m->load(p.string()));
    }

    m->set_animation(0);
    node.model = std::move(m);
    model_ = node.model.get();

    std::cout << "loaded model " << p << '\n';
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
                     blue::md2::Model const * model = reinterpret_cast<blue::md2::Model const *>(data);
                     assert(model);
                     if (idx < 0 || idx >= model->animations().size()) { return false; }
                     *out_text = model->animations()[idx].name.c_str();
                     return true;
                 },
                 reinterpret_cast<void *>(&model()),
                 model().animations().size());

    model().set_animation(static_cast<size_t>(index));

    int sindex = model().skin_index();

    ImGui::Combo("Skin", &sindex,
                 [](void * data, int idx, char const ** out_text) -> bool {
                     blue::md2::Model const * model = reinterpret_cast<blue::md2::Model const *>(data);
                     assert(model);
                     if (idx < 0 || idx >= model->skins().size()) { return false; }
                     *out_text = model->skins()[idx].name.c_str();
                     return true;
                 },
                 reinterpret_cast<void *>(&model()),
                 model().skins().size());

    model().set_skin_index(static_cast<size_t>(sindex));

    float fps = model().frames_per_second();
    ImGui::InputFloat("Animation FPS", &fps, 1.0f, 5.0f, 1);
    model().set_frames_per_second(fps);
}
