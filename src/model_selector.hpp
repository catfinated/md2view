#pragma once

#include "md2.hpp"

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

    std::vector<std::string> const& items() const { return items_; }

    void set_model_index(int item) { current_ = item; load_model(); }
    int model_index() const { return current_; }

    std::string const& model_name() const { if (selected_) { return selected_->name; } else { return items_[current_]; } }
    std::string model_path() const { if (selected_) { return selected_->path; } else { return std::string{}; } }

    blue::md2::Model& model() { assert(model_); return *model_; }

    void draw_ui();

    template <typename RandEngine>
    void select_random_model(RandEngine& eng);

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

    void load_model();
    bool load_model_node(Node& n);

    std::string path_;
    std::vector<std::string> paths_;
    std::vector<std::string> items_;
    int current_ = 0;
    blue::md2::Model * model_ = nullptr;
    std::unordered_map<int, std::unique_ptr<blue::md2::Model>> models_;
    Node root_;
    Node * selected_ = nullptr;

};

inline void ModelSelector::init(std::string const& path, blue::EngineBase& eb)
{
    path_ = path;
    root_.path = path;
    boost::filesystem::path p(path_);

    if (!boost::filesystem::exists(p)) {
        throw std::runtime_error("invalid models path: " + path_);
    }

    if (boost::filesystem::is_directory(p)) {
        auto iter_range = boost::make_iterator_range(boost::filesystem::directory_iterator(p), {});

        for (auto const& entry : iter_range) {
            if (boost::filesystem::is_directory(entry)) {
                paths_.emplace_back(entry.path().string());
                items_.emplace_back(entry.path().stem().string());
                std::cout << entry.path().stem() << '\n';
            }
        }

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

    select_random_model(eb.random_engine());

    //load_model();
}

template <typename RandEngine>
void ModelSelector::select_random_model(RandEngine& eng)
{
    Node * curr = &root_;

    while (curr && !selected_) {
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
            }
        }
    }
}

inline void ModelSelector::load_model()
{
    auto iter = models_.find(current_);

    if (iter != models_.end()) {
        model_ = iter->second.get();
        return;
    }

    assert(current_ < items_.size());
    assert(current_ < paths_.size());

    //auto const& name = items_[current_];
    auto const& dirname = paths_[current_];

    boost::filesystem::path p(dirname);
    p /= "tris.md2";
    auto m = std::unique_ptr<blue::md2::Model>(new blue::md2::Model{});

    std::cout << "loading model " << p << '\n';

    BLUE_EXPECT(m->load(p.string()));
    m->set_animation(0);
    models_[current_] = std::move(m);
    model_ = models_[current_].get();

    std::cout << "loaded model " << p << ' ' << current_ << '\n';

    //if (!anim_name_.empty()) {
    //    model_.set_animation(anim_name_);
    //}
    //else {
    //    model_.set_animation(0);
    //}
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

    BLUE_EXPECT(m->load(p.string()));
    m->set_animation(0);
    node.model = std::move(m);
    //model_ = models_[current_].get();
    model_ = node.model.get();

    std::cout << "loaded model " << p << ' ' << current_ << '\n';
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
    /*
    int mindex = model_index();

    ImGui::ListBox("Model", &mindex,
                   [](void * data, int idx, char const ** out) -> bool {
                       auto ms = reinterpret_cast<ModelSelector const *>(data);
                       if (idx < 0 || idx >= ms->items().size()) { return false; }
                       *out = ms->items()[idx].c_str();
                       return true;
                   },
                   reinterpret_cast<void *>(this),
                   items().size());

    //set_model_index(mindex);
    */
    if (ImGui::TreeNode("Models")) {
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
                    std::cout << "seelected model=" << curr->name << ' ' << curr->path << '\n';

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
}
