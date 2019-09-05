#include "pcx.hpp"

#include <iostream>
#include <iomanip>

PCX::PCX(std::istream& ds)
{
    std::cout << "hdr size " << sizeof(header_) << '\n';
    auto header_ptr = std::addressof(header_);
    memset(header_ptr, 0, sizeof(header_));
    ds.read(reinterpret_cast<char *>(header_ptr), sizeof(header_));

    auto width = header_.xend - header_.xstart + 1;
    auto length = header_.yend - header_.ystart + 1;
    auto scan_line_length = header_.num_bit_planes * header_.bytes_per_line;
    auto line_padding_size = (scan_line_length * (8 / header_.bits_per_pixel)) - width;

    std::cout << "width: " << width << " length: " << length << '\n';
    std::cout << "scan line length: " << scan_line_length << '\n';
    std::cout << "line padding size: " << line_padding_size << '\n';

    std::cout << "reading scan lines\n";
    std::vector<ScanLine> scan_lines;

    for (auto i = 0; i < length; ++i) {
        scan_lines.push_back(readScanLine(ds, scan_line_length));
    }
    std::cout << "done reading scan lines " << scan_lines.size() << '\n';

    colors_ = readPalette(ds);
    std::cout << "num colors in palette: " << colors_.size() << '\n';

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

PCX::ScanLine PCX::readScanLine(std::istream& ds, int32_t length)
{
    ScanLine scan_line;
    scan_line.resize(static_cast<size_t>(length));

    auto index = decltype(length)(0);

    uint8_t runcount;
    uint8_t runvalue;
    uint32_t total(0);
    bool runny{false};
    bool alwaysrunny{true};
    uint32_t bytecount(0);

    while (index < length) {
        uint8_t byte;
        ds.read((char*)&byte, 1);
        //ds >> byte;
        //assert(ds.gcount() == 1);
        ++bytecount;
        if ((byte & 0xc0) == 0xc0) {
            runcount = byte  & 0x3f;
            //ds >> byte;
            ds.read((char*)&byte, 1);
            runvalue = byte;
            runny = true;
            ++bytecount;
        }
        else {
            runcount = 1;
            runvalue = byte;
            alwaysrunny = false;
        }

        for (total += runcount; runcount && index < length; --runcount, ++index) {
            scan_line[static_cast<size_t>(index)] = runvalue;
        }
    }

    return scan_line;
    //qDebug() << "runny: " << runny << "alwaysrunny" << alwaysrunny << "bytes: " << bytecount << "total: " << total;
}

std::vector<PCX::Color> PCX::readPalette(std::istream& ds)
{
    std::vector<PCX::Color> colors;

    if (ds.eof()) {
        return colors;
    }

    uint64_t count(1);
    uint8_t byte;
    //ds >> byte;
    ds.read((char *)&byte, 1);

    std::cout << "palette byte:  " << static_cast<int>(byte) << '\n';

    while (!ds.eof()) {
        uint8_t rgb[3];
        //ds >> r >> g >> b;
        ds.read((char*)rgb, 3);
        count += 3;
        //qDebug() << r << g << b;

        colors.emplace_back(rgb[0], rgb[1], rgb[2]);
        //qDebug() << colors.back();
    }

    std::cout << "leftover bytes: " << count << '\n';
    return colors;
}

template <size_t N>
std::ostream& operator<<(std::ostream& os, std::array<uint8_t, N> const& ary)
{
    constexpr auto cols_per_line = 8u;
    auto col = 0u;

    std::ios_base::fmtflags flags(os.flags());

    //os << std::hex << std::setfill('0');

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

std::ostream& operator<<(std::ostream& os, PCX::Header const& hdr)
{
    os << "identifier:\t " << static_cast<uint32_t>(hdr.identifier)
       << "\nversion:\t" << static_cast<uint32_t>(hdr.version)
       << "\nencoding:\t" << static_cast<uint32_t>(hdr.encoding)
       << "\nbits per pixel:\t" << static_cast<uint32_t>(hdr.bits_per_pixel)
       << "\nxstart:\t" << hdr.xstart
       << "\nystart:\t" << hdr.ystart
       << "\nxend:\t" << hdr.xend
       << "\nyend:\t" << hdr.yend
       << "\nhorzres:\t" << hdr.horzres
       << "\nvertres:\t" << hdr.vertres
       << "\npalette:\n" << hdr.palette
       << "\nreserved1:\t" << static_cast<uint32_t>(hdr.reserved1)
       << "\nnum bit planes:\t" << static_cast<uint32_t>(hdr.num_bit_planes)
       << "\nbytes per line:\t" << hdr.bytes_per_line
       << "\npalette type:\t" << hdr.palette_type
       << "\nhorz screen size:\t" << hdr.horz_screen_size
       << "\nvert screensize:\t" << hdr.vert_screen_size
       << "\nreserved2:\n" << hdr.reserved2;

    return os;
}
