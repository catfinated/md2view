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

// Writes a minimal valid MD2 file with 1 frame, 1 triangle, 3 vertices.
//
// Layout:
//   68 bytes  header   (17 x int32)
//   12 bytes  texcoords (3 x {int16 s, int16 t})
//   12 bytes  triangle  (uint16 vertex[3], uint16 st[3])
//   52 bytes  frame     (scale[3], translate[3], name[16], 3 x Vertex{v[3],
//   normal})
//
// Animation: one frame named "stand0" → animation id "stand"
//
// After loading, scaled_texcoords() == [(0,0),(0.5,0),(0,0.5)]
// After loading, interpolated_vertices() == [(0,0,0),(1,0,0),(0,1,0)]
//   (MD2 loader remaps: v[0]→x, v[1]→z, v[2]→y)
static void write_md2(std::filesystem::path const& path) {
    std::ofstream f(path, std::ios::binary);

    int32_t const ident = 844121161; // "IDP2"
    int32_t const version = 8;
    int32_t const skinwidth = 2;
    int32_t const skinheight = 2;
    int32_t const num_skins = 0;
    int32_t const num_xyz = 3;
    int32_t const num_st = 3;
    int32_t const num_tris = 1;
    int32_t const num_glcmds = 0;
    int32_t const num_frames = 1;

    // framesize = scale(12) + translate(12) + name(16) + num_xyz *
    // sizeof(Vertex)(4)
    int32_t const framesize = 12 + 12 + 16 + num_xyz * 4;

    int32_t const offset_skins = 68;        // after header
    int32_t const offset_st = offset_skins; // 0 skins
    int32_t const offset_tris = offset_st + num_st * 4;
    int32_t const offset_frames = offset_tris + num_tris * 12;
    int32_t const offset_glcmds = offset_frames + num_frames * framesize;
    int32_t const offset_end = offset_glcmds;

    // Header (17 x int32 = 68 bytes)
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

    // Texcoords: 3 x {int16 s, int16 t}
    // With skinwidth=2, skinheight=2: scaled = (s/2, t/2)
    // tc0=(0,0)→(0.0,0.0)  tc1=(1,0)→(0.5,0.0)  tc2=(0,1)→(0.0,0.5)
    int16_t texcoords[3][2] = {{0, 0}, {1, 0}, {0, 1}};
    f.write(reinterpret_cast<char*>(texcoords), sizeof(texcoords));

    // Triangle: uint16 vertex[3] + uint16 st[3]
    uint16_t tri[6] = {0, 1, 2,  // vertex indices
                       0, 1, 2}; // texcoord indices
    f.write(reinterpret_cast<char*>(tri), sizeof(tri));

    // Frame: scale[3] + translate[3] + name[16] + vertices
    // Loader maps: v[0]→x, v[1]→z, v[2]→y
    // v0=(0,0,0)→(0,0,0)  v1=(1,0,0)→(1,0,0)  v2=(0,0,1)→(0,1,0)
    write_f32le(f, 1.0f);
    write_f32le(f, 1.0f);
    write_f32le(f, 1.0f); // scale
    write_f32le(f, 0.0f);
    write_f32le(f, 0.0f);
    write_f32le(f, 0.0f); // translate
    std::array<char, 16> name{};
    std::strncpy(name.data(), "stand0", 15);
    f.write(name.data(), 16);
    // vertices: {v[0], v[1], v[2], normal_index}
    uint8_t verts[3][4] = {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}};
    f.write(reinterpret_cast<char*>(verts), sizeof(verts));
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
    return 0;
}
