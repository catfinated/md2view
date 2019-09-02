#include "pak.hpp"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <iostream>
#include <fstream>

static_assert(sizeof(PakHeader) == 12, "unexpected PackHeader size");
static_assert(sizeof(PakEntry) == 64, "unexpected PackFile size");

PakFile::PakFile(std::string const& filename)
{
    init(filename);
}

bool PakFile::init(std::string const& filename)
{
    filename_ = filename;

    std::ifstream inf(filename_.c_str(), std::ios_base::in | std::ios_base::binary);

    if (!inf) {
        std::cout << "failed to open pak file: " << filename;
        return false;
    }

    PakHeader hdr;
    inf.read(reinterpret_cast<char *>(&hdr), sizeof(hdr));

    std::cout << hdr.id[0] << hdr.id[1] << hdr.id[2] << hdr.id[3]
              << " " << hdr.dirofs
              << " " << hdr.dirlen
              << '\n';

    std::string magic(hdr.id.data(), hdr.id.size());

    if (magic != std::string{"PACK"}) {
        throw std::runtime_error("not a valid pak file");
    }

    //hdr.dirofs = hdr.dirofs; // le32toh
    //hdr.dirlen = hdr.dirlen; // le32toh
    auto num_entries = static_cast<size_t>(hdr.dirlen) / sizeof(PakEntry);

    std::cout << "loaded pak file: " << filename
              << " " << hdr.dirofs
              << " " << hdr.dirlen
              << " " << num_entries
              << '\n';

    inf.seekg(hdr.dirofs);

    for (size_t i = 0; i < num_entries; ++ i) {
        PakEntry entry;
        inf.read(reinterpret_cast<char *>(&entry), sizeof(entry));
        // ltoh
        //packfile.filepos = qFromLittleEndian(packfile.filepos);
        //packfile.filelen = qFromLittleEndian(packfile.filelen);

        //asser(entry.filepos <= file_.size());

        auto fullname = std::string(entry.name.data());

        std::cout << "file: "
                  << fullname
                  << " " << entry.filepos
                  << " " << entry.filelen
                  << '\n';

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

/*
QByteArray PakModel::extractFile(PakFileItem const& item) const
{
    if (item.filelen() < 1) {
        return QByteArray();
    }

    QFile file(filename_);
    qDebug() << item.filepos() << file.size();
    file.open(QIODevice::ReadOnly);
    file.seek(item.filepos());
    return file.read(static_cast<qint64>(item.filelen()));
}
*/

PakFile::Node const * PakFile::find(std::string const& name) const
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
