#include "pak.hpp"
#include "common.hpp"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <spdlog/spdlog.h>

#include <fstream>
#include <vector>

static_assert(sizeof(PAK::Header) == 12, "unexpected PackHeader size");
static_assert(sizeof(PAK::Entry) == 64, "unexpected PackFile size");

PAK::PAK(std::string const& filename)
{
    init(filename);
}

bool PAK::init(std::string const& filename)
{
    filename_ = filename;

    std::ifstream inf(filename_.c_str(), std::ios_base::in | std::ios_base::binary);

    if (!inf) {
        spdlog::info("failed to open pak file: {}", filename);
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
        filename,
        hdr.dirofs,
        hdr.dirlen,
        num_entries);

    inf.seekg(hdr.dirofs);

    for (size_t i = 0; i < num_entries; ++ i) {
        MD2V_EXPECT(inf);
        Entry entry;
        inf.read(reinterpret_cast<char *>(&entry), sizeof(entry));
        // TODO: ensure filepos/filelen converted from little endian to host

        auto fullname = std::string(entry.name.data());

        spdlog::info("file: {} {} {}",
            fullname,
            entry.filepos,
            entry.filelen);

        std::vector<std::string> parts;
        boost::algorithm::split(parts, fullname, boost::algorithm::is_any_of("/"));

        auto parent = &root_;
        for (auto j = 0u; j < parts.size(); ++j) {
            auto const& part = parts.at(j);
            //qDebug() << part;
            auto child = parent->find(part);

            if (!child) {
                if (j != parts.size() - 1) {
                    child = new Node{};
                    child->name = part;
                    child->parent = parent;
                    child->path = fullname;
                }
                else {
                    child = new Node{};
                    child->name = part;
                    child->parent = parent;
                    child->path = fullname;
                    child->filepos = entry.filepos;
                    child->filelen = entry.filelen;
                }
                parent->insert(child);
            }
            parent = child;
            child = nullptr;
        }
    }

    return true;
}

PAK::Node const * PAK::find(std::string const& name) const
{
    if (name.empty()) {
        return nullptr;
    }

    std::vector<std::string> parts;
    boost::algorithm::split(parts, name, boost::algorithm::is_any_of("/"));

    Node const * current = std::addressof(root_);
    auto iter = parts.begin();

    while (current && (iter != parts.end())) {
        current = current->find(*iter);
        ++iter;
    }

    if (iter == parts.end()) {
        return current;
    }

    return nullptr;
}
