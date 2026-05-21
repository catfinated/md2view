#pragma once

#include <boost/algorithm/string/predicate.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <string>
#include <unordered_map>

/// Quake II PAK archive reader.
///
/// Supports two modes transparently:
/// - **Archive mode**: reads a real `.pak` file (binary format with a flat
///   directory of named entries).
/// - **Directory mode**: treats a plain filesystem directory as though it were
///   a PAK file, enabling development without a real archive.
///
/// Callers use the same `open_ifstream()` interface in both modes.
///
/// @see https://quakewiki.org/wiki/.pak
class PAK {
public:
    /// A single entry in the archive directory.
    struct Node {
        std::string name;  ///< Entry name (key used for lookup).
        std::string path;  ///< Full path or archive-relative path.
        int32_t filepos{}; ///< Byte offset within the `.pak` file (archive mode
                           ///< only).
        uintmax_t filelen{}; ///< Entry size in bytes.
    };

    /// Construct a PAK from a file or directory path.
    ///
    /// @param fpath Path to a `.pak` file or a directory to treat as one.
    /// @throws std::runtime_error if @p fpath does not exist or the archive
    ///         header has an invalid magic number.
    explicit PAK(std::filesystem::path fpath);

    /// Path that was used to construct this PAK.
    std::filesystem::path const& fpath() const noexcept { return fpath_; }

    /// Returns true if this PAK wraps a directory rather than a `.pak` file.
    [[nodiscard]] bool is_directory() const noexcept {
        return fpath_.extension() != ".pak";
    }

    /// Open a read stream for a named entry.
    ///
    /// In archive mode the stream is positioned at the entry's data. In
    /// directory mode it opens the corresponding file from the filesystem.
    ///
    /// @param fpath Archive-relative path of the entry (e.g.
    /// `"models/player/tris.md2"`).
    /// @return An open `std::ifstream` positioned at the start of the entry.
    std::ifstream open_ifstream(std::filesystem::path const& fpath) const;

    /// Lazy range of all `.md2` model entries in the archive.
    auto models() const {
        return entries() | std::ranges::views::values |
               std::ranges::views::filter([](auto const& n) {
                   return boost::algorithm::ends_with(n.path, ".md2");
               });
    }

    /// Returns true if the archive contains at least one `.md2` entry.
    [[nodiscard]] bool has_models() const noexcept { return !models().empty(); }

private:
    [[nodiscard]] bool init();
    [[nodiscard]] bool init_from_file();
    void init_from_directory();
    std::unordered_map<std::string, Node> const& entries() const {
        return entries_;
    }

    std::filesystem::path fpath_;
    std::unordered_map<std::string, Node> entries_;
};
