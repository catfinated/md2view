#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

// https://www.fileformat.info/format/pcx/egff.htm
class PCX {
public:
#pragma pack(push, 1)
    struct Header {
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

        Color(uint8_t _r, uint8_t _g, uint8_t _b)
            : r(_r)
            , g(_g)
            , b(_b) {}
    };

    PCX() = default;
    PCX(std::istream&);

    std::vector<unsigned char> const& image() const { return image_; }

    std::vector<Color> const& colors() const { return colors_; }

    int height() const { return height_; }
    int width() const { return width_; }

private:
    using ScanLine = std::vector<uint8_t>;
    static ScanLine read_scan_line(std::istream& ds, int32_t length);
    static std::vector<Color> read_palette(std::istream&);

private:
    std::vector<unsigned char> image_;
    std::vector<Color> colors_;
    int width_{};
    int height_{};
};

std::ostream& operator<<(std::ostream&, PCX::Header const&);
