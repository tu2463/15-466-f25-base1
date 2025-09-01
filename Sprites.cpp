#include "Sprites.hpp"

// the code is inspired by Prof Jim's code from Discord channel 2-assets
Sprites Sprites::load(std::string const &filename) {
    std::ifstream file(filename, std::ios::binary);

    // data is stored in three arrays of plain-old-data types
    std::vector< char > names;
    std::vector< Sprite::TileRef > refs;
    struct StoredSprite {
        uint32_t name_begin, name_end; // name is names[name_begin, name_end)
        uint32_t ref_begin, ref_end; // refs are refs[ref_begin, ref_end)
    };
    std::vector< StoredSprite > sprites;

    // copy, no parsing (file -> memory)
    read_chunk(file, "name", &names);
    read_chunk(file, "refs ", &refs);
    read_chunk(file, "sprt", &sprites);

    char junk;
    if (file.read(&junk, 1)) {
        throw std::runtime_error("Trailing junk at the end of sprites file");
    }

    // now the stored sprites can be quickly turned into a Sprites object:
    Sprites ret;

    for (StoredSprite const &stored : sprites) {
        if (!(stored.name_begin < stored.name_end && stored.name_end <= uint32_t(names.size()))) { // < or <= //??
            throw std::runtime_error("Sprite with bad name range");
        }
        if (!(stored.ref_begin < stored.ref_end && stored.ref_end <= uint32_t(refs.size()))) { // < or <= //??
            throw std::runtime_error("Sprite with bad refs range");
        }

        // copy name into a string
        // second copy (memory to memory) could avoid with std::string_view (and keeping "names" around).
        // similar for refs.
        // but memory copy is cheap and convenient.
        std::string name(
            names.begin() + stored.name_begin,
            names.begin() + stored.name_end
        )
        Sprite sprite;
        // copy the stored refs into the sprite
        sprite.tiles.assign(
            refs.begin() + stored.ref_begin,
            refs.begin() + stored.ref_end
        );

        // add the sprite to the returned sprites object
        auto result = ret.sprites.emplace(name, sprite);
        if (!result.second) {
            throw std::runtime_error("Duplicate sprite name: " + name);
        }
    }

    return ret;
}