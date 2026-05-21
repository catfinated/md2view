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
    return 0;
}
