#include "pak.hpp"
#include "common.hpp"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <vector>

static_assert(sizeof(PAK::Header) == 12, "unexpected PackHeader size");
static_assert(sizeof(PAK::Entry) == 64, "unexpected PackFile size");

PAK::PAK(std::filesystem::path const& fpath)
    : fpath_(fpath)
{
    if (!init()) {
        throw std::runtime_error("failed to load PAK file");
    }
}

bool PAK::init()
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

    tree_.insert(tree_.begin(), Node{});

    for (size_t i = 0; i < num_entries; ++ i) {
        MD2V_EXPECT(inf);
        Entry entry;
        inf.read(reinterpret_cast<char *>(&entry), sizeof(entry));
        // TODO: ensure filepos/filelen converted from little endian to host

        auto fullname = std::string(entry.name.data());

        spdlog::debug("file: {} {} {}",
            fullname,
            entry.filepos,
            entry.filelen);

        std::vector<std::string> parts;
        boost::algorithm::split(parts, fullname, boost::algorithm::is_any_of("/"));

        auto parent = tree_.begin();
        std::string node_name;
        for (auto j = 0u; j < parts.size(); ++j) {
            auto const& part = parts.at(j);
            if (!node_name.empty()) node_name.append("/");
            node_name += part;

            auto child = std::find_if(parent, tree_.end(), [&](auto const& node) { return node.path == node_name; });

            if (child == tree_.end()) {
                Node node;
                node.name = part;
                node.path = node_name;
                node.filepos = entry.filepos;
                node.filelen = entry.filelen;
                child = tree_.append_child(parent, std::move(node));
            }
            parent = child;
        }
    }

    return true;
}

PAK::Node const * PAK::find(std::string const& name) const
{
    auto iter = std::find_if(tree_.begin(), tree_.end(), [&](auto const& node) { return node.path == name; });

    if (iter == tree_.end()) {
        return nullptr;
    }
    return std::addressof(*iter);
}

