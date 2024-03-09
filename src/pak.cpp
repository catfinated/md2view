#include "pak.hpp"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <gsl/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <vector>

#pragma pack(push, 1)
struct Header
{
    std::array<char, 4> id;
    int32_t dirofs;
    int32_t dirlen;
};

struct Entry
{
    std::array<char, 56> name;
    int32_t filepos;
    int32_t filelen;
};
#pragma pack(pop)

static_assert(sizeof(Header) == 12, "unexpected PackHeader size");
static_assert(sizeof(Entry) == 64, "unexpected PackFile size");

PAK::PAK(std::filesystem::path const& fpath)
    : fpath_(fpath)
{
    if (!init()) {
        throw std::runtime_error("failed to load PAK file");
    }
}

bool PAK::init()
{
    if (!std::filesystem::exists(fpath_)) {
        spdlog::error("'{}' does not exist!", fpath_.string());
        return false;
    }

    if (std::filesystem::is_regular_file(fpath_)) {
        return init_from_file();
    }
    init_from_directory();
    return true;
}

void PAK::init_from_directory()
{
    for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(fpath_ )) {
        if (std::filesystem::is_regular_file(dir_entry.path())) {
            auto path = dir_entry.path().lexically_relative(fpath_).string();
            std::replace(path.begin(), path.end(), '\\', '/');
            Node node;
            node.name = dir_entry.path().stem().string();
            node.path = path;
            node.filepos = 0;
            node.filelen = std::filesystem::file_size(dir_entry.path());
            spdlog::info("{} {} {}", node.name, node.path, node.filelen);
            entries_.emplace(path, std::move(node));
            has_models_ |= dir_entry.path().extension() == ".md2";
        }
    }
}

bool PAK::init_from_file()
{
    std::ifstream inf(fpath_, std::ios_base::in | std::ios_base::binary);

    if (!inf) {
        spdlog::info("failed to open pak file: {}", fpath_.string());
        return false;
    }

    Header hdr;
    inf.read(reinterpret_cast<char *>(&hdr), sizeof(hdr));

    spdlog::info("{}{}{}{} {} {}", 
        hdr.id[0],
        hdr.id[1],
        hdr.id[2],
        hdr.id[3],
        hdr.dirofs,
        hdr.dirlen);

    std::string magic(hdr.id.data(), hdr.id.size());

    if (magic != std::string{"PACK"}) {
        throw std::runtime_error("not a valid pak file");
    }

    // TODO: ensure dirofs/dirlen converted from little endian to host
    auto num_entries = static_cast<size_t>(hdr.dirlen) / sizeof(Entry);

    spdlog::info("loaded pak file: {} {} {} {}", 
        fpath_.string(),
        hdr.dirofs,
        hdr.dirlen,
        num_entries);

    inf.seekg(hdr.dirofs);

    for (size_t i = 0; i < num_entries; ++ i) {
        gsl_Assert(inf);
        Entry entry;
        inf.read(reinterpret_cast<char *>(&entry), sizeof(entry));
        // TODO: ensure filepos/filelen converted from little endian to host

        auto const fullname = std::string(entry.name.data());
        auto const path = std::filesystem::path(fullname);

        spdlog::debug("file: {} {} {}",
            fullname,
            entry.filepos,
            entry.filelen);

        Node node;
        node.name = path.stem().string();
        node.path = fullname;
        node.filepos = entry.filepos;
        node.filelen = entry.filelen;
        entries_.emplace(fullname, std::move(node));
        has_models_ |= path.extension() == ".md2";
    }
    return true;
}

std::ifstream PAK::open_ifstream(std::filesystem::path const& filename) const
{
    auto const flags = std::ios_base::in | std::ios_base::binary;

    if (!is_directory()) {
        auto const& node = entries().at(filename.string());
        std::ifstream inf(fpath_, flags);
        inf.seekg(node.filepos);
        return inf;
    }

    auto const p = fpath_ / filename;
    spdlog::info("open file {}", p.string());
    return std::ifstream(p, flags);
}



