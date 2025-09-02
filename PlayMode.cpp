#include "PlayMode.hpp"
#include "Sprites.hpp"
#include "data_path.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

// PlayMode::PlayMode() {
// 	//TODO:
// 	// you *must* use an asset pipeline of some sort to generate tiles.
// 	// don't hardcode them like this!
// 	// or, at least, if you do hardcode them like this,
// 	//  make yourself a script that spits out the code that you paste in here
// 	//   and check that script into your repository.

// 	//Also, *don't* use these tiles in your game:

// 	{ //use tiles 0-16 as some weird dot pattern thing:
// 		std::array< uint8_t, 8*8 > distance;
// 		for (uint32_t y = 0; y < 8; ++y) {
// 			for (uint32_t x = 0; x < 8; ++x) {
// 				float d = glm::length(glm::vec2((x + 0.5f) - 4.0f, (y + 0.5f) - 4.0f));
// 				d /= glm::length(glm::vec2(4.0f, 4.0f));
// 				distance[x+8*y] = uint8_t(std::max(0,std::min(255,int32_t( 255.0f * d ))));
// 			}
// 		}
// 		for (uint32_t index = 0; index < 16; ++index) {
// 			PPU466::Tile tile;
// 			uint8_t t = uint8_t((255 * index) / 16);
// 			for (uint32_t y = 0; y < 8; ++y) {
// 				uint8_t bit0 = 0;
// 				uint8_t bit1 = 0;
// 				for (uint32_t x = 0; x < 8; ++x) {
// 					uint8_t d = distance[x+8*y];
// 					if (d > t) {
// 						bit0 |= (1 << x);
// 					} else {
// 						bit1 |= (1 << x);
// 					}
// 				}
// 				tile.bit0[y] = bit0;
// 				tile.bit1[y] = bit1;
// 			}
// 			ppu.tile_table[index] = tile;
// 		}
// 	}

// 	//use sprite 32 as a "player":
// 	ppu.tile_table[32].bit0 = {
// 		0b01111110,
// 		0b11111111,
// 		0b11111111,
// 		0b11111111,
// 		0b11111111,
// 		0b11111111,
// 		0b11111111,
// 		0b01111110,
// 	};
// 	ppu.tile_table[32].bit1 = {
// 		0b00000000,
// 		0b00000000,
// 		0b00011000,
// 		0b00100100,
// 		0b00000000,
// 		0b00100100,
// 		0b00000000,
// 		0b00000000,
// 	};

// 	//makes the outside of tiles 0-16 solid:
// 	ppu.palette_table[0] = {
// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
// 		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
// 		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
// 	};

// 	//makes the center of tiles 0-16 solid:
// 	ppu.palette_table[1] = {
// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
// 		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
// 		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
// 	};

// 	//used for the player:
// 	ppu.palette_table[7] = {
// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
// 		glm::u8vec4(0xff, 0xff, 0x00, 0xff),
// 		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
// 		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
// 	};

// 	//used for the misc other sprites:
// 	ppu.palette_table[6] = {
// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
// 		glm::u8vec4(0x88, 0x88, 0xff, 0xff),
// 		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
// 	};

// }

// Credit: Used ChatGPT for assistance
PlayMode::PlayMode() {
    // --- 1) Define palettes from your hex colors ---

    // Background palette (no transparency needed; duplicate one color as the 4th slot):
    // BG: 301e21, 463537, 110a18
    std::array<glm::u8vec4,4> bg_palette_rgba = {
        glm::u8vec4(0x00,0x00,0x00,0x00), // 0 = transparent (lets background_color show)
        glm::u8vec4(0x30,0x1e,0x21,0xff), // 1
        glm::u8vec4(0x46,0x35,0x37,0xff), // 2
        glm::u8vec4(0x11,0x0a,0x18,0xff)  // 3
    };

    // // Character palette (index 0 must be transparent for sprites):
    // // Characters: 110a18, 5f6268, 7e1f23, transparent
    // std::array<glm::u8vec4,4> char_palette_rgba = {
    //     glm::u8vec4(0x00,0x00,0x00,0x00), // 0 transparent
    //     glm::u8vec4(0x11,0x0a,0x18,0xff), // 1
    //     glm::u8vec4(0x5f,0x62,0x68,0xff), // 2
    //     glm::u8vec4(0x7e,0x1f,0x23,0xff)  // 3
    // };

    // // Obstacle palette (index 0 transparent; 4th slot duplicated):
    // // Obstacles: 7e1f23, 5f6268, transparent
    // std::array<glm::u8vec4,4> obst_palette_rgba = {
    //     glm::u8vec4(0x00,0x00,0x00,0x00), // 0 transparent
    //     glm::u8vec4(0x7e,0x1f,0x23,0xff), // 1
    //     glm::u8vec4(0x5f,0x62,0x68,0xff), // 2
    //     glm::u8vec4(0x5f,0x62,0x68,0xff)  // 3 (duplicate, unused filler)
    // };

    // --- 2) Pack three tilesheets, PNG -> PPU ---

    auto bg_packed = Sprites::pack_png_tileset(
        data_path("assets/bg_tiles.png"),
        bg_palette_rgba
    );

    // auto char_packed = Sprites::pack_png_tileset(
    //     data_path("assets/char_tiles.png"),
    //     char_palette_rgba
    // );

    // auto obst_packed = Sprites::pack_png_tileset(
    //     data_path("assets/obst_tiles.png"),
    //     obst_palette_rgba
    // );

    // --- 3) Upload into fixed tile table ranges (0, 32, 64) ---
    //    Should truncate for Game1 if there are. >256 tiles.
    const size_t bg_tile_base   = 1; // walls begin at tile index 1 (tile 0 will be blank)
    const size_t char_tile_base = 32; // keep 32 so draw() can continue using index=32
    // const size_t obst_tile_base = 64;

	// calculates the number of tiles to copy into tile_table, ensuring it does not exceed the size of tile_table
    const size_t bg_copy_count   = std::min(bg_packed.tiles.size(),   ppu.tile_table.size() - bg_tile_base);
    // const size_t char_copy_count = std::min(char_packed.tiles.size(), ppu.tile_table.size() - char_tile_base);
    // const size_t obst_copy_count = std::min(obst_packed.tiles.size(), ppu.tile_table.size() - obst_tile_base);

    for (size_t i = 0; i < bg_copy_count;   ++i) ppu.tile_table[bg_tile_base   + i] = bg_packed.tiles[i];
    // for (size_t i = 0; i < char_copy_count; ++i) ppu.tile_table[char_tile_base + i] = char_packed.tiles[i];
    // for (size_t i = 0; i < obst_copy_count; ++i) ppu.tile_table[obst_tile_base + i] = obst_packed.tiles[i];

	// make tile 0 be a fully transparent tile (so solid background color shows):
    PPU466::Tile blank{};
    for (int r = 0; r < 8; ++r) { blank.bit0[r] = 0; blank.bit1[r] = 0; }
    ppu.tile_table[0] = blank;

    // --- 4) Install palettes into PPU slots (BG=0, Characters=7, Obstacles=6) ---
	// NOTE that packed.palette is exactly the same as my_palette, refer to the pack_png_tileset function - is it correct?
	/* from PPU466.hpp:
	typedef std::array< glm::u8vec4, 4 > Palette;
	std::array< Palette, 8 > palette_table; */

    // BG -> palette slot 0:
    ppu.palette_table[0][0] = bg_palette_rgba[0];
    ppu.palette_table[0][1] = bg_palette_rgba[1];
    ppu.palette_table[0][2] = bg_palette_rgba[2];
    ppu.palette_table[0][3] = bg_palette_rgba[3];

    // // Characters -> palette slot 7:
    // ppu.palette_table[7][0] = char_palette_rgba[0];
    // ppu.palette_table[7][1] = char_palette_rgba[1];
    // ppu.palette_table[7][2] = char_palette_rgba[2];
    // ppu.palette_table[7][3] = char_palette_rgba[3];

    // // Obstacles -> palette slot 6:
    // ppu.palette_table[6][0] = obst_palette_rgba[0];
    // ppu.palette_table[6][1] = obst_palette_rgba[1];
    // ppu.palette_table[6][2] = obst_palette_rgba[2];
    // ppu.palette_table[6][3] = obst_palette_rgba[3];

    // --- 5) Static background ---
    for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
        for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
            // This just cycles across the first row of your bg_tiles.png so you can see them:
            // uint8_t idx_in_row = uint8_t( ((x/2) + (y/2)) % std::max<uint32_t>(1, bg_packed.tiles_w) );
            // ppu.background[x + PPU466::BackgroundWidth * y] = uint8_t(bg_tile_base + idx_in_row);
			ppu.background[x + PPU466::BackgroundWidth * y] = 0; // blank everywhere
        }
    }

	// --- Wall tiles
	// choose which specific wall tile to use:
    const uint8_t wall_tile_index = uint8_t(bg_tile_base); // first wall tile you uploaded

    // 5.1) Four borders of the viewport:
    for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
        // top & bottom rows:
        ppu.background[x + PPU466::BackgroundWidth * 0] = wall_tile_index;
        ppu.background[x + PPU466::BackgroundWidth * (PPU466::BackgroundHeight - 1)] = wall_tile_index;
    }
    for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
        // left & right columns:
        ppu.background[0 + PPU466::BackgroundWidth * y] = wall_tile_index;
        ppu.background[(PPU466::BackgroundWidth - 1) + PPU466::BackgroundWidth * y] = wall_tile_index;
    }

    // 5.2) Fill the 10th row of tiles:
    if (PPU466::BackgroundHeight > 10) {
        const uint32_t y = 10; // 0-based "10th" row
        for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
            ppu.background[x + PPU466::BackgroundWidth * y] = wall_tile_index;
        }
    }

    // 5.3) Fill the 20th row of tiles:
    if (PPU466::BackgroundHeight > 20) {
        const uint32_t y = 20; // 0-based "20th" row
        for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
            ppu.background[x + PPU466::BackgroundWidth * y] = wall_tile_index;
        }
    }

    // --- 6) Player sprite: keep palette 7 and use the first character tile at index 32 ---

    ppu.sprites[0].index = uint8_t(char_tile_base); // 32
    ppu.sprites[0].attributes = 7;                  // character palette

    // Optional background color (kept from your version):
    ppu.background_color = glm::u8vec4(0x11,0x0a,0x18,0xff);
}


/* "~"" indicates that this function is called automatically when a PlayMode object is destroyed, 
either when it goes out of scope or is explicitly deleted. */
PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	constexpr float PlayerSpeed = 30.0f;
	if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	if (up.pressed) player_at.y += PlayerSpeed * elapsed;

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	// ppu.background_color = glm::u8vec4(
	// 	std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 0.0f / 3.0f) ) ) ))),
	// 	std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 1.0f / 3.0f) ) ) ))),
	// 	std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 2.0f / 3.0f) ) ) ))),
	// 	0xff
	// );

	ppu.background_color = glm::u8vec4(0x11,0x0a,0x18,0xff);

	// //tilemap gets recomputed every frame as some weird plasma thing:
	// //NOTE: don't do this in your game! actually make a map or something :-)
	// for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
	// 	for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
	// 		//TODO: make weird plasma thing
	// 		ppu.background[x+PPU466::BackgroundWidth*y] = ((x+y)%16);
	// 	}
	// }

	// //background scroll:
	// ppu.background_position.x = int32_t(-0.5f * player_at.x);
	// ppu.background_position.y = int32_t(-0.5f * player_at.y);

	ppu.background_position.x = 0;
    ppu.background_position.y = 0;

	//player sprite:
	ppu.sprites[0].x = int8_t(player_at.x);
	ppu.sprites[0].y = int8_t(player_at.y);
	ppu.sprites[0].index = 32;
	ppu.sprites[0].attributes = 7;

	//some other misc sprites:
	for (uint32_t i = 1; i < 63; ++i) {
		float amt = (i + 2.0f * background_fade) / 62.0f;
		ppu.sprites[i].x = int8_t(0.5f * float(PPU466::ScreenWidth) + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player_at.x) * 0.4f * float(PPU466::ScreenWidth));
		ppu.sprites[i].y = int8_t(0.5f * float(PPU466::ScreenHeight) + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player_at.y) * 0.4f * float(PPU466::ScreenWidth));
		ppu.sprites[i].index = 64;
		ppu.sprites[i].attributes = 6;
		if (i % 2) ppu.sprites[i].attributes |= 0x80; //'behind' bit
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
