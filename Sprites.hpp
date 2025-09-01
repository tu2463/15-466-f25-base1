// Sprites.hpp
// Used ChatGPT for assistance
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>

#include "PPU466.hpp"     // for PPU466::Tile, palette types
#include <glm/glm.hpp>    // for glm::u8vec4

struct Sprite {
    struct TileRef {
        uint8_t index;      // index into PPU tile_table
        int8_t  x_offset = 0;     // x offset from sprite origin (pixels)
        int8_t  y_offset = 0;     // y offset from sprite origin (pixels)
        uint8_t attributes = 0; // palette index + behind/flip bits if you use them
    };
    std::vector<TileRef> tiles; // meta-sprite = many 8×8 tiles with offsets
};


// // this code is auto-completed with Github Copilot
// struct Sprite {
//     struct TileRef {
//         uint8_t tile_index; // which tile (in the PPU's pattern table)
//         uint8_t palette_index; // which of the 4 palettes for this tile
//         bool x_flip; // whether to flip the tile horizontally
//         bool y_flip; // whether to flip the tile vertically
//         int8_t x_offset; // offset in pixels from the sprite's origin
//         int8_t y_offset; // offset in pixels from the sprite's origin
//     };
//     std::vector<TileRef> tiles; // one or more tiles that make up this sprite
// };

struct Sprites {
    // name -> sprite
    std::unordered_map<std::string, Sprite> sprites;

    // chunk loader (prof sample; we’ll finish it in Sprites.cpp):
    static Sprites load(std::string const &filename);

    // Lightweight “packer” that slices an RGBA PNG tilesheet into PPU tiles.
    // Assumptions: (a) image size is multiples of 8, (b) each tile uses <=4 distinct colors,
    // (c) colors must match exactly one of the 4 entries you pass in 'palette'.
    struct Packed {
        std::vector<PPU466::Tile> tiles;               // packed 2-bit tiles
        std::array<glm::u8vec4,4> palette;             // single 4-color palette you used
        uint16_t tiles_w = 0, tiles_h = 0;             // width/height in 8×8 tiles
    };
    static Packed pack_png_tileset(std::string const &png_path,
                                   std::array<glm::u8vec4,4> const &palette_rgba);
};
