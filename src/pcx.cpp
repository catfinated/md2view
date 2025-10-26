#include "md2view/pcx.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>

PCX::PCX(std::istream& ds) {
    Header header{};
    ds.read(reinterpret_cast<char*>(std::addressof(header)), sizeof(header));

    auto width = header.xend - header.xstart + 1;
    auto length = header.yend - header.ystart + 1;
    auto scan_line_length = header.num_bit_planes * header.bytes_per_line;

    std::vector<ScanLine> scan_lines;
    scan_lines.reserve(length);

    for (auto i = 0; i < length; ++i) {
        scan_lines.emplace_back(read_scan_line(ds, scan_line_length));
    }
    spdlog::info("done reading scan lines {}", scan_lines.size());

    colors_ = read_palette(ds);
    spdlog::info("num colors in palette: {}", colors_.size());

    width_ = width;
    height_ = length;
    image_.resize(width * length * 3);

    for (auto i = 0; i < length; ++i) {
        for (auto j = 0; j < width; ++j) {
            auto index = scan_lines[i][j];
            assert(index < colors_.size());
            auto loc = (j + (i * width)) * 3;
            auto& color = colors_[index];
            image_[loc + 0] = color.r;
            image_[loc + 1] = color.g;
            image_[loc + 2] = color.b;
        }
    }
}

PCX::ScanLine PCX::read_scan_line(std::istream& ds, int32_t length) {
    ScanLine scan_line;
    scan_line.resize(static_cast<size_t>(length));

    auto index = decltype(length)(0);

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

std::vector<PCX::Color> PCX::read_palette(std::istream& ds) {
    std::vector<PCX::Color> colors;

    if (ds.eof()) {
        return colors;
    }

    char byte{}; // marker byte
    ds.read(std::addressof(byte), 1);

    while (!ds.eof()) {
        std::array<char, 3> rgb{};
        ds.read(rgb.data(), rgb.size());
        colors.emplace_back(static_cast<uint8_t>(rgb[0]),
                            static_cast<uint8_t>(rgb[1]),
                            static_cast<uint8_t>(rgb[2]));
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
