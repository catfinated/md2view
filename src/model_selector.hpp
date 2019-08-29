#pragma once

#include "md2.hpp"

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include <exception>
#include <iostream>
#include <unordered_map>

class ModelSelector
{
public:
    explicit ModelSelector(std::string const& path)
        : path_(path)
    {}

    void init();

    std::vector<std::string> const& items() const { return items_; }

    void set_model_index(int item) { current_ = item; load_model(); }
    int model_index() const { return current_; }

    std::string const& model_name() const { return items_[current_]; }

    blue::md2::Model& model() { assert(model_); return *model_; }

private:
    void load_model();

    std::string path_;
    std::vector<std::string> paths_;
    std::vector<std::string> items_;
    int current_ = 0;
    blue::md2::Model * model_ = nullptr;
    std::unordered_map<int, std::unique_ptr<blue::md2::Model>> models_;
};

inline void ModelSelector::init()
{
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
    }

    load_model();
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
