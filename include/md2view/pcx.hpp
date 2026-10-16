/// @file
/// PCX image decoder
#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <utility>
#include <vector>

/// PCX image decoder.
///
/// Decodes a PCX image from a stream into a flat RGBA pixel buffer. Only the
/// 8-bit indexed variant (256-colour VGA palette) is supported, which covers
/// all Quake II skin textures. The alpha channel is always set to opaque.
///
/// @see https://www.fileformat.info/format/pcx/egff.htm
class PCX {
public:
    /// Raw 128-byte PCX file header. Packed to match the on-disk layout
    /// exactly.
#pragma pack(push, 1)
    struct Header {
        uint8_t identifier;   ///< Must be 0x0A for a valid PCX file.
        uint8_t version;      ///< PCX version number.
        uint8_t encoding;     ///< 1 = RLE encoded.
        uint8_t bitsPerPixel; ///< Bits per pixel per plane (8 for 256-colour).
        uint16_t xstart;      ///< Left edge of the image.
        uint16_t ystart;      ///< Top edge of the image.
        uint16_t xend;        ///< Right edge; width = xend - xstart + 1.
        uint16_t yend;        ///< Bottom edge; height = yend - ystart + 1.
        uint16_t horzres;     ///< Horizontal resolution (DPI).
        uint16_t vertres;     ///< Vertical resolution (DPI).
        std::array<uint8_t, 48> palette; ///< EGA palette (unused for 8-bit).
        uint8_t reserved1;
        uint8_t numBitPlanes;  ///< Number of colour planes.
        uint16_t bytesPerLine; ///< Bytes per scan line per plane.
        uint16_t paletteType;  ///< 1 = colour, 2 = greyscale.
        uint16_t horzScreenSize;
        uint16_t vertScreenSize;
        std::array<uint8_t, 54> reserved2;
    };
#pragma pack(pop)

    /// A single RGB colour entry from the 256-entry VGA palette.
    struct Color {
        uint8_t r; ///< Red channel (0–255).
        uint8_t g; ///< Green channel (0–255).
        uint8_t b; ///< Blue channel (0–255).

        Color(uint8_t r, uint8_t g, uint8_t b)
            : r(r)
            , g(g)
            , b(b) {}
    };

    /// Construct an empty PCX with no image data.
    PCX() = default;

    /// Decode a PCX image from a stream.
    ///
    /// @param is An open, readable stream positioned at the start of PCX data.
    /// @throws gsl_lite::fail_fast if the stream is not in a good state.
    /// @throws std::runtime_error if format unsupported or data is malformed.
    explicit PCX(std::istream& is);

    /// Decoded pixel data as packed RGBA8, row-major.
    /// Size is width * height * 4 bytes
    [[nodiscard]] std::vector<unsigned char> const& image() const& {
        return image_;
    }

    /// Decoded pixel data as packed RGBA8, row-major.
    /// Size is width * height * 4 bytes
    [[nodiscard]] std::vector<unsigned char> image() && {
        return std::move(image_);
    }

    /// The 256-entry VGA palette decoded from the end of the file.
    [[nodiscard]] std::vector<Color> const& colors() const { return colors_; }

    [[nodiscard]] int height() const { return height_; }
    [[nodiscard]] int width() const { return width_; }

private:
    using ScanLine = std::vector<uint8_t>;
    static ScanLine readScanLine(std::istream& ds, int32_t length);
    static std::vector<Color> readPalette(std::istream& is);

    std::vector<unsigned char> image_;
    std::vector<Color> colors_;
    int width_{};
    int height_{};
};

std::ostream& operator<<(std::ostream& os, PCX::Header const& hdr);
