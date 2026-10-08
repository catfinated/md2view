// Generates minimal valid test fixture files.
// Usage: gen_fixtures <output_dir>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

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

static void write_f32le(std::ofstream& f, float v) {
    uint32_t bits{};
    std::memcpy(&bits, &v, sizeof(bits));
    f.put(static_cast<char>(bits & 0xFF));
    f.put(static_cast<char>((bits >> 8) & 0xFF));
    f.put(static_cast<char>((bits >> 16) & 0xFF));
    f.put(static_cast<char>((bits >> 24) & 0xFF));
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

// --- PNG ---
//
// Minimal PNG encoder using only the standard library. Pixel data goes in a
// zlib stream made of a single uncompressed ("stored") deflate block, so no
// compression library is needed; only CRC-32 and Adler-32 checksums.

using Bytes = std::vector<uint8_t>;

static void append_u32be(Bytes& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(v & 0xFF));
}

static uint32_t crc32(Bytes const& data) {
    uint32_t crc = 0xFFFFFFFFU;
    for (uint8_t byte : data) {
        crc ^= byte;
        for (int k = 0; k < 8; ++k) {
            crc = (crc & 1U) != 0 ? (crc >> 1) ^ 0xEDB88320U : crc >> 1;
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

static uint32_t adler32(Bytes const& data) {
    uint32_t a = 1;
    uint32_t b = 0;
    for (uint8_t byte : data) {
        a = (a + byte) % 65521U;
        b = (b + a) % 65521U;
    }
    return (b << 16) | a;
}

static void
write_png_chunk(std::ofstream& f, char const (&type)[5], Bytes const& data) {
    Bytes type_and_data(type, type + 4);
    type_and_data.insert(type_and_data.end(), data.begin(), data.end());

    Bytes header;
    append_u32be(header, static_cast<uint32_t>(data.size()));
    f.write(reinterpret_cast<char const*>(header.data()),
            static_cast<std::streamsize>(header.size()));
    f.write(reinterpret_cast<char const*>(type_and_data.data()),
            static_cast<std::streamsize>(type_and_data.size()));

    Bytes crc;
    append_u32be(crc, crc32(type_and_data));
    f.write(reinterpret_cast<char const*>(crc.data()),
            static_cast<std::streamsize>(crc.size()));
}

// Writes an 8-bit PNG. `rows` holds the raw pixel bytes of each row (palette
// indices for color type 3, RGBA for color type 6). `palette` is RGB triples
// and is only written for color type 3.
static void write_png(std::filesystem::path const& path,
                      uint32_t width,
                      uint32_t height,
                      uint8_t color_type,
                      std::vector<Bytes> const& rows,
                      Bytes const& palette = {}) {
    std::ofstream f(path, std::ios::binary);
    constexpr std::array<uint8_t, 8> signature{0x89, 'P',  'N',  'G',
                                               0x0D, 0x0A, 0x1A, 0x0A};
    f.write(reinterpret_cast<char const*>(signature.data()), signature.size());

    Bytes ihdr;
    append_u32be(ihdr, width);
    append_u32be(ihdr, height);
    ihdr.push_back(8);          // bit depth
    ihdr.push_back(color_type); // 3 = palette, 6 = RGBA
    ihdr.push_back(0);          // compression = deflate
    ihdr.push_back(0);          // filter method
    ihdr.push_back(0);          // no interlace
    write_png_chunk(f, "IHDR", ihdr);

    if (color_type == 3) {
        write_png_chunk(f, "PLTE", palette);
    }

    // each scanline is prefixed by its filter type (0 = none)
    Bytes raw;
    for (auto const& row : rows) {
        raw.push_back(0);
        raw.insert(raw.end(), row.begin(), row.end());
    }

    // a single stored block holds at most 65535 bytes, plenty for fixtures
    auto const len = static_cast<uint16_t>(raw.size());
    // NB: cast back to 16 bits; ~ promotes to int and would set the high bits
    auto const nlen = static_cast<uint16_t>(~len);
    Bytes idat{0x78, 0x01}; // zlib header: deflate, no preset dictionary
    idat.push_back(0x01);   // final block, stored (uncompressed)
    // LEN and its one's complement NLEN, both little-endian (RFC 1951 3.2.4)
    idat.push_back(static_cast<uint8_t>(len & 0xFF));
    idat.push_back(static_cast<uint8_t>(len >> 8));
    idat.push_back(static_cast<uint8_t>(nlen & 0xFF));
    idat.push_back(static_cast<uint8_t>(nlen >> 8));
    idat.insert(idat.end(), raw.begin(), raw.end());
    append_u32be(idat, adler32(raw));
    write_png_chunk(f, "IDAT", idat);

    write_png_chunk(f, "IEND", {});
}

// Writes a 2x2 palette PNG (color type 3) with the same layout and colors as
// minimal.pcx. stb_image reports 3 channels in the file for it, which is the
// case that broke loading drfreak.png.
static void write_palette_png(std::filesystem::path const& path) {
    write_png(path, 2, 2, 3, {{0, 1}, {1, 0}},
              {255, 0, 0, /* red */ 0, 0, 255 /* blue */});
}

// Writes a 2x2 RGBA PNG (color type 6) with distinct alpha values, to check
// that alpha survives loading.
//   (0,0) = red, opaque       (1,0) = green, alpha 128
//   (0,1) = blue, alpha 64    (1,1) = white, transparent
static void write_rgba_png(std::filesystem::path const& path) {
    write_png(
        path, 2, 2, 6,
        {{255, 0, 0, 255, 0, 255, 0, 128}, {0, 0, 255, 64, 255, 255, 255, 0}});
}

// Writes a PAK archive with one entry, `models/test/skin.pcx`, whose content
// is the bytes of an existing PCX file. Real Quake II paks only contain PCX
// images, so this covers loading an image from a real archive.
static void write_pcx_pak(std::filesystem::path const& path,
                          std::filesystem::path const& pcx_path) {
    std::ifstream in(pcx_path, std::ios::binary);
    Bytes const content{std::istreambuf_iterator<char>(in),
                        std::istreambuf_iterator<char>()};

    std::ofstream f(path, std::ios::binary);
    auto const content_ofs = int32_t{12};
    auto const content_len = static_cast<int32_t>(content.size());

    f.write("PACK", 4);
    write_i32le(f, content_ofs + content_len); // dirofs
    write_i32le(f, 64);                        // dirlen: one entry

    f.write(reinterpret_cast<char const*>(content.data()), content_len);

    std::array<char, 56> name{};
    std::strncpy(name.data(), "models/test/skin.pcx", 55);
    f.write(name.data(), 56);
    write_i32le(f, content_ofs);
    write_i32le(f, content_len);
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

// Writes an MD2 header and shared geometry (texcoords + triangle) for 3
// vertices / 1 triangle.  Returns the file offset just after the triangle so
// the caller can write frames immediately.
static int32_t write_md2_header_and_geometry(std::ofstream& f,
                                             int32_t num_frames) {
    int32_t const ident = 844121161; // "IDP2"
    int32_t const version = 8;
    int32_t const skinwidth = 2;
    int32_t const skinheight = 2;
    int32_t const num_skins = 0;
    int32_t const num_xyz = 3;
    int32_t const num_st = 3;
    int32_t const num_tris = 1;
    int32_t const num_glcmds = 0;
    int32_t const framesize = 12 + 12 + 16 + num_xyz * 4; // 52

    int32_t const offset_skins = 68;
    int32_t const offset_st = offset_skins;
    int32_t const offset_tris = offset_st + num_st * 4;
    int32_t const offset_frames = offset_tris + num_tris * 12;
    int32_t const offset_glcmds = offset_frames + num_frames * framesize;
    int32_t const offset_end = offset_glcmds;

    write_i32le(f, ident);
    write_i32le(f, version);
    write_i32le(f, skinwidth);
    write_i32le(f, skinheight);
    write_i32le(f, framesize);
    write_i32le(f, num_skins);
    write_i32le(f, num_xyz);
    write_i32le(f, num_st);
    write_i32le(f, num_tris);
    write_i32le(f, num_glcmds);
    write_i32le(f, num_frames);
    write_i32le(f, offset_skins);
    write_i32le(f, offset_st);
    write_i32le(f, offset_tris);
    write_i32le(f, offset_frames);
    write_i32le(f, offset_glcmds);
    write_i32le(f, offset_end);

    // Texcoords: (0,0),(1,0),(0,1) → scaled (0,0),(0.5,0),(0,0.5)
    int16_t texcoords[3][2] = {{0, 0}, {1, 0}, {0, 1}};
    f.write(reinterpret_cast<char*>(texcoords), sizeof(texcoords));

    // Triangle: vertices [0,1,2], texcoords [0,1,2]
    uint16_t tri[6] = {0, 1, 2, 0, 1, 2};
    f.write(reinterpret_cast<char*>(tri), sizeof(tri));

    return offset_frames;
}

// Write one MD2 keyframe. Each vertex is {v[0], v[1], v[2], normal}.
// Loader remaps: v[0]→x, v[1]→z, v[2]→y with scale=(1,1,1) translate=(0,0,0).
static void
write_md2_frame(std::ofstream& f, char const* name, uint8_t verts[3][4]) {
    // scale = (1,1,1), translate = (0,0,0)
    write_f32le(f, 1.0f);
    write_f32le(f, 1.0f);
    write_f32le(f, 1.0f);
    write_f32le(f, 0.0f);
    write_f32le(f, 0.0f);
    write_f32le(f, 0.0f);
    std::array<char, 16> fname{};
    std::strncpy(fname.data(), name, 15);
    f.write(fname.data(), 16);
    f.write(reinterpret_cast<char*>(verts[0]), 12); // 3 vertices × 4 bytes
}

// Two-frame MD2: animation "stand" with frames 0..1.
// Frame 0 vertices (world): (0,0,0),(2,0,0),(0,2,0)
// Frame 1 vertices (world): (10,0,0),(12,0,0),(10,10,0)
// At interpolation t=0.5: vertex[0] ≈ (5,0,0)
static void write_two_frame_md2(std::filesystem::path const& path) {
    std::ofstream f(path, std::ios::binary);
    write_md2_header_and_geometry(f, 2);
    uint8_t f0[3][4] = {{0, 0, 0, 0}, {2, 0, 0, 0}, {0, 0, 2, 0}};
    uint8_t f1[3][4] = {{10, 0, 0, 0}, {12, 0, 0, 0}, {10, 0, 10, 0}};
    write_md2_frame(f, "stand0", f0);
    write_md2_frame(f, "stand1", f1);
}

// Four-frame MD2: animations "stand" (frames 0..1) and "run" (frames 2..3).
static void write_two_anim_md2(std::filesystem::path const& path) {
    std::ofstream f(path, std::ios::binary);
    write_md2_header_and_geometry(f, 4);
    uint8_t verts[3][4] = {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}};
    write_md2_frame(f, "stand0", verts);
    write_md2_frame(f, "stand1", verts);
    write_md2_frame(f, "run0", verts);
    write_md2_frame(f, "run1", verts);
}

// Writes a minimal valid MD2 file with 1 frame, 1 triangle, 3 vertices.
//
// Animation: one frame named "stand0" → animation id "stand"
//
// After loading, scaled_texcoords() == [(0,0),(0.5,0),(0,0.5)]
// After loading, interpolated_vertices() == [(0,0,0),(1,0,0),(0,1,0)]
//   (MD2 loader remaps: v[0]→x, v[1]→z, v[2]→y)
static void write_md2(std::filesystem::path const& path) {
    std::ofstream f(path, std::ios::binary);
    write_md2_header_and_geometry(f, 1);
    uint8_t verts[3][4] = {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}};
    write_md2_frame(f, "stand0", verts);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return 1;
    }
    std::filesystem::path dir{argv[1]};
    std::filesystem::create_directories(dir);
    write_pcx(dir / "minimal.pcx");
    write_pak(dir / "minimal.pak");
    write_md2(dir / "minimal.md2");
    write_two_frame_md2(dir / "two_frame.md2");
    write_two_anim_md2(dir / "two_anim.md2");
    write_palette_png(dir / "palette.png");
    write_rgba_png(dir / "rgba.png");
    write_pcx_pak(dir / "skin.pak", dir / "minimal.pcx");
    return 0;
}
