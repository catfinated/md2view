#include "md2view/pcx.hpp"

#include <fmt/format.h>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
constexpr std::size_t kPaletteSize{256}; /// 256-color palette
constexpr uint8_t kPaletteMarker{0x0C};  ///< 256-color palette marker
constexpr uint8_t kEncodingRle{1};       ///< RLE encoding marker
} // namespace

PCX::PCX(std::istream& is) {
    Header header{};
    is.read(reinterpret_cast<char*>(std::addressof(header)), sizeof(header));
    gsl_Assert(is.gcount() == sizeof(header));

    if (header.encoding != kEncodingRle) {
        throw std::runtime_error(fmt::format(
            "unsupported PCX: encoding {}, only RLE (1) is supported",
            header.encoding));
    }
    if (header.bits_per_pixel != 8 || header.num_bit_planes != 1) {
        throw std::runtime_error(fmt::format(
            "unsupported PCX: {} colour plane(s) of {} bits per pixel, only "
            "8-bit paletted images (1 plane) are supported",
            header.num_bit_planes, header.bits_per_pixel));
    }

    auto width = header.xend - header.xstart + 1;
    auto length = header.yend - header.ystart + 1;
    auto scan_line_length = header.num_bit_planes * header.bytes_per_line;

    if (width <= 0 || length <= 0) {
        throw std::runtime_error(fmt::format(
            "malformed PCX: invalid image size {}x{}", width, length));
    }

    if (scan_line_length < width) {
        throw std::runtime_error(
            fmt::format("malformed PCX: scan lines are {} bytes, shorter than "
                        "the image width {}",
                        scan_line_length, width));
    }

    std::vector<ScanLine> scan_lines;
    scan_lines.reserve(length);

    for (auto i = 0; i < length; ++i) {
        scan_lines.emplace_back(read_scan_line(is, scan_line_length));
    }
    if (!is) {
        throw std::runtime_error("malformed PCX: image data is truncated");
    }
    spdlog::info("done reading scan lines {}", scan_lines.size());

    colors_ = read_palette(is);
    width_ = width;
    height_ = length;
    constexpr auto numChannels{4};
    image_.resize(width * length * numChannels);

    for (auto i = 0; i < length; ++i) {
        for (auto j = 0; j < width; ++j) {
            auto index = scan_lines[i][j];
            auto loc = (j + (i * width)) * numChannels;
            auto& color = colors_[index];
            image_[loc + 0] = color.r;
            image_[loc + 1] = color.g;
            image_[loc + 2] = color.b;
            image_[loc + 3] = 255;
        }
    }
}

PCX::ScanLine PCX::read_scan_line(std::istream& ds, int32_t length) {
    ScanLine scan_line;
    scan_line.resize(static_cast<size_t>(length));

    auto index = decltype(length){0};

    uint8_t runcount{};
    uint8_t runvalue{};

    while (index < length) {
        char byte{};
        ds.read(std::addressof(byte), 1);

        if ((byte & 0xc0) == 0xc0) {
            runcount = byte & 0x3f;
            ds.read(std::addressof(byte), 1);
            runvalue = static_cast<uint8_t>(byte);
        } else {
            runcount = 1;
            runvalue = static_cast<uint8_t>(byte);
        }

        while (runcount > 0 && index < length) {
            scan_line[static_cast<size_t>(index)] = runvalue;
            --runcount;
            ++index;
        }
    }

    return scan_line;
}

std::vector<PCX::Color> PCX::read_palette(std::istream& is) {
    char marker{};
    if (!is.read(std::addressof(marker), 1) ||
        static_cast<uint8_t>(marker) != kPaletteMarker) {
        throw std::runtime_error("malformed PCX: missing 256-colour palette");
    }

    std::array<char, kPaletteSize * 3> rgb{};
    if (!is.read(rgb.data(), rgb.size())) {
        throw std::runtime_error(
            fmt::format("malformed PCX: palette has {} colors, expected {}",
                        is.gcount() / 3, kPaletteSize));
    }

    std::vector<PCX::Color> colors;
    colors.reserve(kPaletteSize);
    for (std::size_t i = 0; i < rgb.size(); i += 3) {
        colors.emplace_back(static_cast<uint8_t>(rgb.at(i)),
                            static_cast<uint8_t>(rgb.at(i + 1)),
                            static_cast<uint8_t>(rgb.at(i + 2)));
    }
    return colors;
}

template <size_t N>
std::ostream& operator<<(std::ostream& os, std::array<uint8_t, N> const& ary) {
    constexpr auto cols_per_line = 8u;
    auto col = 0u;

    std::ios_base::fmtflags flags(os.flags());

    for (auto x : ary) {
        os << std::hex << std::setfill('0') << std::setw(2) << +x;
        os << ' ';
        if (++col == cols_per_line) {
            col = 0u;
            os << '\n';
        }
    }

    os.flags(flags);
    return os;
}

std::ostream& operator<<(std::ostream& os, PCX::Header const& hdr) {
    os << "identifier:\t " << static_cast<uint32_t>(hdr.identifier)
       << "\nversion:\t" << static_cast<uint32_t>(hdr.version)
       << "\nencoding:\t" << static_cast<uint32_t>(hdr.encoding)
       << "\nbits per pixel:\t" << static_cast<uint32_t>(hdr.bits_per_pixel)
       << "\nxstart:\t" << hdr.xstart << "\nystart:\t" << hdr.ystart
       << "\nxend:\t" << hdr.xend << "\nyend:\t" << hdr.yend << "\nhorzres:\t"
       << hdr.horzres << "\nvertres:\t" << hdr.vertres << "\npalette:\n"
       << hdr.palette << "\nreserved1:\t"
       << static_cast<uint32_t>(hdr.reserved1) << "\nnum bit planes:\t"
       << static_cast<uint32_t>(hdr.num_bit_planes) << "\nbytes per line:\t"
       << hdr.bytes_per_line << "\npalette type:\t" << hdr.palette_type
       << "\nhorz screen size:\t" << hdr.horz_screen_size
       << "\nvert screensize:\t" << hdr.vert_screen_size << "\nreserved2:\n"
       << hdr.reserved2;

    return os;
}
