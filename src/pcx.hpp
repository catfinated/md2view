#pragma once

//#include <QString>
//#include <QDataStream>
//#include <QColor>
//#include <QImage>

#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <memory>

class PcxFile
{
public:
#pragma pack(push, 1)
    struct Header
    {
        uint8_t identifier;
        uint8_t version;
        uint8_t encoding;
        uint8_t bits_per_pixel;
        uint16_t xstart;
        uint16_t ystart;
        uint16_t xend;
        uint16_t yend;
        uint16_t horzres;
        uint16_t vertres;
        std::array<uint8_t, 48> palette;
        uint8_t reserved1;
        uint8_t num_bit_planes;
        uint16_t bytes_per_line;
        uint16_t palette_type;
        uint16_t horz_screen_size;
        uint16_t vert_screen_size;
        std::array<uint8_t, 54> reserved2;
    };
#pragma pack(pop)

    struct Color {
        uint8_t r;
        uint8_t g;
        uint8_t b;

        Color(int32_t _r, int32_t _g, int32_t _b)
            : r(_r)
            , g(_g)
            , b(_b)
        {}
    };

    PcxFile() = default;
    PcxFile(std::istream&);

    Header const& header() const { return header_; }

    //QImage const& image() const { assert(image_); return *image_; }
    std::vector<unsigned char> const& image() const { return image_; }

    std::vector<Color> const& colors() const { return colors_; }

    int height() const { return height_; }
    int width() const { return width_; }

private:
    using ScanLine = std::vector<uint8_t>;
    ScanLine readScanLine(std::istream& ds, int32_t length);
    std::vector<Color> readPalette(std::istream&);

private:
    Header header_;
    std::vector<unsigned char> image_;
    std::vector<Color> colors_;
    int width_ = 0;
    int height_ = 0;
};

std::ostream& operator<<(std::ostream&, PcxFile::Header const&);