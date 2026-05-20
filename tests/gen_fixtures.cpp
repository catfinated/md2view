// Generates minimal valid test fixture files.
// Usage: gen_fixtures <output_dir>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>

static void write_u8(std::ofstream& f, uint8_t v) {
    f.put(static_cast<char>(v));
}

static void write_u16le(std::ofstream& f, uint16_t v) {
    f.put(static_cast<char>(v & 0xFF));
    f.put(static_cast<char>((v >> 8) & 0xFF));
}

static void write_i32le(std::ofstream& f, int32_t v) {
    f.put(static_cast<char>(v & 0xFF));
    f.put(static_cast<char>((v >> 8) & 0xFF));
    f.put(static_cast<char>((v >> 16) & 0xFF));
    f.put(static_cast<char>((v >> 24) & 0xFF));
}

// Writes a minimal valid 2x2 PCX file.
//
// Layout:
//   128-byte header
//   4 bytes  scan line data  (2 rows x 2 bytes, RLE single-byte values)
//   1 byte   palette marker  (0x0C)
//   768 bytes palette        (256 RGB triples)
//
// Pixel layout (palette indices):
//   (0,0)=0  (1,0)=1
//   (0,1)=1  (1,1)=0
//
// Palette:
//   index 0 = red   (255,   0,   0)
//   index 1 = blue  (  0,   0, 255)
//   index 2..255 = black
static void write_pcx(std::filesystem::path const& path) {
    std::ofstream f(path, std::ios::binary);

    // --- Header (128 bytes) ---
    write_u8(f, 0x0A); // identifier
    write_u8(f, 5);    // version
    write_u8(f, 1);    // encoding = RLE
    write_u8(f, 8);    // bits per pixel
    write_u16le(f, 0); // xstart
    write_u16le(f, 0); // ystart
    write_u16le(f, 1); // xend   (width  = xend - xstart + 1 = 2)
    write_u16le(f, 1); // yend   (height = yend - ystart + 1 = 2)
    write_u16le(f, 2); // horzres
    write_u16le(f, 2); // vertres
    for (int i = 0; i < 48; ++i)
        write_u8(f, 0); // palette (unused for 8-bit)
    write_u8(f, 0);     // reserved1
    write_u8(f, 1);     // num_bit_planes
    write_u16le(f, 2);  // bytes_per_line (= width)
    write_u16le(f, 1);  // palette_type
    write_u16le(f, 0);  // horz_screen_size
    write_u16le(f, 0);  // vert_screen_size
    for (int i = 0; i < 54; ++i)
        write_u8(f, 0); // reserved2

    // --- Scan lines (scan_line_length = num_bit_planes * bytes_per_line = 2)
    // --- Values 0x00-0xBF are stored as-is (no RLE encoding needed).
    write_u8(f, 0x00);
    write_u8(f, 0x01); // row 0: index 0, index 1
    write_u8(f, 0x01);
    write_u8(f, 0x00); // row 1: index 1, index 0

    // --- Palette ---
    write_u8(f, 0x0C); // palette marker
    // index 0 = red
    write_u8(f, 255);
    write_u8(f, 0);
    write_u8(f, 0);
    // index 1 = blue
    write_u8(f, 0);
    write_u8(f, 0);
    write_u8(f, 255);
    // index 2..255 = black
    for (int i = 2; i < 256; ++i) {
        write_u8(f, 0);
        write_u8(f, 0);
        write_u8(f, 0);
    }
}

// Writes a minimal valid PAK file containing one .md2 entry.
//
// Layout:
//   12 bytes  PAK header  (magic "PACK", dirofs, dirlen)
//    5 bytes  file data   ("HELLO")
//   64 bytes  directory   (one entry: "models/player/tris.md2")
static void write_pak(std::filesystem::path const& path) {
    std::ofstream f(path, std::ios::binary);

    char const* content = "HELLO";
    int32_t content_ofs = 12; // immediately after 12-byte header
    int32_t content_len = 5;
    int32_t dir_ofs = content_ofs + content_len; // = 17
    int32_t dir_len = 64;                        // one 64-byte entry

    // Header
    f.write("PACK", 4);
    write_i32le(f, dir_ofs);
    write_i32le(f, dir_len);

    // File data
    f.write(content, content_len);

    // Directory entry (64 bytes: 56-byte name + int32 filepos + int32 filelen)
    std::array<char, 56> name{};
    std::strncpy(name.data(), "models/player/tris.md2", 55);
    f.write(name.data(), 56);
    write_i32le(f, content_ofs);
    write_i32le(f, content_len);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return 1;
    }
    std::filesystem::path dir{argv[1]};
    std::filesystem::create_directories(dir);
    write_pcx(dir / "minimal.pcx");
    write_pak(dir / "minimal.pak");
    return 0;
}
