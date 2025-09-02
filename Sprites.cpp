// Sprites.cpp
#include "Sprites.hpp"

#include <fstream>
#include <stdexcept>
#include <cassert>
#include <algorithm>

#include "read_write_chunk.hpp" // for read_chunk(...) used by the prof sample
#include "load_save_png.hpp"    // for load_png(...)
#include "data_path.hpp"        // if you want to resolve relative paths later

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// -----------------------------
// 1) Chunk-based loader:
// Credit: built upon Prof Jim's code from Discord channel 2-assets
// -----------------------------
Sprites Sprites::load(std::string const &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("Couldn't open sprites file: " + filename);

    // raw arrays from file:
    // data is stored_sprites in three arrays of plain-old-data types
    std::vector<char> names;
    std::vector<Sprite::TileRef> refs;

    struct StoredSprite {
        uint32_t name_begin, name_end; // name is names[name_begin, name_end)
        uint32_t ref_begin,  ref_end;  // refs are refs[ref_begin,  ref_end)
    };
    std::vector<StoredSprite> stored_sprites;

    read_chunk(file, "name", &names);
    read_chunk(file, "refs ", &refs);
    read_chunk(file, "sprt", &stored_sprites);

    char junk;
    if (file.read(&junk, 1)) {
        throw std::runtime_error("Trailing junk at the end of sprites file");
    }

    // assemble Sprites object:
    // now the stored sprites can be quickly turned into a Sprites object:
    Sprites out;

    for (StoredSprite const &s : stored_sprites) {
        // if (!(stored.name_begin < stored.name_end && stored.name_end <= uint32_t(names.size()))) { // < or <= //??
        //     throw std::runtime_error("Sprite with bad name range");
        // }
        // if (!(stored.ref_begin < stored.ref_end && stored.ref_end <= uint32_t(refs.size()))) { // < or <= //??
        //     throw std::runtime_error("Sprite with bad refs range");
        // }

        auto bad_range = [](uint32_t begin, uint32_t end, uint32_t size) {
            return !(begin < end && end <= size);
        };
        if (bad_range(s.name_begin, s.name_end, (uint32_t)names.size()))
            throw std::runtime_error("Sprite has bad name range");
        if (bad_range(s.ref_begin, s.ref_end,  (uint32_t)refs.size()))
            throw std::runtime_error("Sprite has bad refs range");

        // require non-empty name + at least one tile:
        if (s.name_begin == s.name_end)  throw std::runtime_error("Sprite has empty name");
        if (s.ref_begin  == s.ref_end )  throw std::runtime_error("Sprite has no tile refs");

         // copy name into a string
        // second copy (memory to memory) could avoid with std::string_view (and keeping "names" around).
        // similar for refs.
        // but memory copy is cheap and convenient.
        std::string name(
            names.begin() + s.name_begin,
            names.begin() + s.name_end
        );

        Sprite sprite;
        // copy the stored refs into the sprite
        sprite.tiles.assign(
            refs.begin() + s.ref_begin,
            refs.begin() + s.ref_end
        );

        // add the sprite to the returned sprites object
        /* std::move(sprite) transfers ownership of the sprite data into the map, 
        which is more efficient because it avoids copying the contents of sprite (suggested by GitHub Copilot).
        After the move, sprite is left in a valid but unspecified state. */
        auto ins = out.sprites.emplace(name, std::move(sprite));
        if (!ins.second) {
            throw std::runtime_error("Duplicate sprite name: " + name);
        }
    }

    return out;
}

// -----------------------------
// 2) PNG → PPU tiles packer:
// Credit: Used ChatGPT for assistance; used Github Copilot to add comments to make sure I understand it
// -----------------------------
Sprites::Packed Sprites::pack_png_tileset(std::string const &png_path,
                                          std::array<glm::u8vec4,4> const &palette_rgba,
                                          bool flip_y) 
{
    // load PNG (RGBA, row-major):

    /* The glm::uvec2 type comes from the GLM (OpenGL Mathematics) library and represents a two-component vector of unsigned integers. 
    Typically, such a vector is used to store dimensions, such as the width and height of a sprite or texture. */
    glm::uvec2 size{}; // width and height of the loaded PNG image in pixels

    /* glm::u8vec4 type is a four-component vector of unsigned 8-bit integers, 
    commonly used to represent color values in the RGBA (red, green, blue, alpha) format.*/
    std::vector<glm::u8vec4> rgba; // will later be filled withcolor of every pixel in RGBA format (red, green, blue, alpha)

    // void load_png(std::string filename, glm::uvec2 *size, std::vector< glm::u8vec4 > *data, OriginLocation origin)
    load_png(png_path, &size, &rgba, OriginLocation::UpperLeftOrigin); // fills size and rgba

    if ((size.x % 8) || (size.y % 8))
        throw std::runtime_error("Tilesheet must be multiples of 8 pixels: " + png_path);

    const uint16_t tiles_w = uint16_t(size.x / 8);
    const uint16_t tiles_h = uint16_t(size.y / 8);
    const uint32_t tile_count = uint32_t(tiles_w) * uint32_t(tiles_h);

    std::vector<PPU466::Tile> tiles(tile_count);

    // helper: takes an RGBA color and returns its index in the provided 4-color palette.
    auto color_to_palette_index = [&](glm::u8vec4 color) -> uint8_t {
        // for (uint8_t palette_i = 0; palette_i < 4; ++palette_i) {
        //     if (palette_rgba[palette_i] == color) return palette_i;
        // }

        // fully transparent => index 0
        if (color.a == 0) return 0;

        // otherwise match by RGB only (ignore alpha to avoid export quirks)
         for (uint8_t palette_i = 0; palette_i < 4; ++palette_i) {
            if (palette_rgba[palette_i] == color) return palette_i;
        }

        // If the PNG has semi-transparent pixels, snap very low alpha to transparent:
        if (color.a < 128) return 0;
        
        throw std::runtime_error(
            "Found a pixel color not in the 4-color palette while packing '" + png_path + "'."
        );
    };

    // walk tiles; pack bitplanes:
    // tile_x/y: horizontal/vertical tile index within the tilesheet, indicates which tile we are processing
    // pixel_x/y: horizontal/vertical pixel index within the current tile, indicates which pixel
    // pos_x/y: absolute horizontal/vertical position of the current pixel within the entire image
    for (uint16_t tile_y = 0; tile_y < tiles_h; ++tile_y) {
        for (uint16_t tile_x = 0; tile_x < tiles_w; ++tile_x) {
            /*
                h,w=0 w=1   (x)     w=2 w=3 
                h=1
                (y)   (y*w+x)
                h=2
                h=3
            */
            const uint32_t tile_index = tile_y * tiles_w + tile_x;
            PPU466::Tile tile{};
            for (int pixel_y = 0; pixel_y < 8; ++pixel_y) {
                uint8_t bitplane0 = 0, bitplane1 = 0;
                const uint32_t src_row_in_tile = flip_y ? (7 - pixel_y) : pixel_y;
                for (int pixel_x = 0; pixel_x < 8; ++pixel_x) {
                    const uint32_t pos_x = uint32_t(tile_x * 8 + pixel_x);
                    const uint32_t pos_y = uint32_t(tile_y * 8 + src_row_in_tile);
                    const glm::u8vec4 pixel = rgba[pos_y * size.x + pos_x]; // row-major index
                    const uint8_t palette_index = color_to_palette_index(pixel) & 3; // masks the result so only the lowest two bits are kept

                    /* extracts the least significant bit and the next significant bit of palette_index,
                    shifts it to the correct position for the current pixel (7 - pixel_x),
                    then sets the corresponding bit in bitplane */
                    bitplane0 |= (palette_index & 1)     << (7 - pixel_x);
                    bitplane1 |= ((palette_index >> 1)&1)<< (7 - pixel_x);
                }
                tile.bit0[pixel_y] = bitplane0;
                tile.bit1[pixel_y] = bitplane1;
            }
            tiles[tile_index] = tile;
        }
    }

    Sprites::Packed packed;
    packed.tiles = std::move(tiles); // std::move function casts tiles to an rvalue reference, allowing its contents to be moved rather than copied.
    packed.palette = palette_rgba; // NOTE that palette_rgba is not modifed in the function - is it correct?
    packed.tiles_w = tiles_w;
    packed.tiles_h = tiles_h;
    return packed;
}

PPU466::Tile Sprites::pack_png_single_tile(std::string const &png_path,
                                           std::array<glm::u8vec4,4> const &palette_rgba,
                                           uint16_t tile_x, uint16_t tile_y,
                                           bool flip_y)
{
    glm::uvec2 size{};
    std::vector<glm::u8vec4> rgba;
    load_png(png_path, &size, &rgba, OriginLocation::UpperLeftOrigin);

    if ((size.x % 8) || (size.y % 8))
        throw std::runtime_error("Tilesheet must be multiples of 8 pixels: " + png_path);

    const uint16_t tiles_w = uint16_t(size.x / 8);
    const uint16_t tiles_h = uint16_t(size.y / 8);
    if (tile_x >= tiles_w || tile_y >= tiles_h)
        throw std::runtime_error("Requested tile (" + std::to_string(tile_x) + "," + std::to_string(tile_y) +
                                 ") is outside PNG tile grid for: " + png_path);

    auto color_to_index = [&](glm::u8vec4 c) -> uint8_t {
        if (c.a == 0) return 0; // transparent
        for (uint8_t i = 0; i < 4; ++i) {
            const auto p = palette_rgba[i];
            if (p.r == c.r && p.g == c.g && p.b == c.b) return i; // RGB match, ignore alpha
        }
        if (c.a < 128) return 0; // snap faint pixels to transparent
        throw std::runtime_error("Pixel color not in 4-color palette while packing single tile: " + png_path);
    };

    PPU466::Tile t{};
    for (int y = 0; y < 8; ++y) {
        uint8_t b0 = 0, b1 = 0;
        const uint32_t src_row_in_tile = flip_y ? (7 - y) : y;
        for (int x = 0; x < 8; ++x) {
            const uint32_t ix = uint32_t(tile_x * 8 + x);
            const uint32_t iy = uint32_t(tile_y * 8 + src_row_in_tile);
            const glm::u8vec4 px = rgba[iy * size.x + ix];
            const uint8_t idx = color_to_index(px) & 3;
            b0 |= (idx & 1)      << (7 - x);
            b1 |= ((idx >> 1)&1) << (7 - x);
        }
        t.bit0[y] = b0;
        t.bit1[y] = b1;
    }
    return t;
}
