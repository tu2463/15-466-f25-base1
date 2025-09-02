#include "PlayMode.hpp"
#include "Sprites.hpp"
#include "data_path.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

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

	// First character palette: 110a18, transparent, 5f6268, 484a4f
	std::array<glm::u8vec4,4> char1_palette_rgba = {
		glm::u8vec4(0x00,0x00,0x00,0x00), // 0 transparent
		glm::u8vec4(0x11,0x0a,0x18,0xff), // 1
		glm::u8vec4(0x5f,0x62,0x68,0xff), // 2
		glm::u8vec4(0x48,0x4a,0x4f,0xff)  // 3
	};
	// Second character palette: 110a18, transparent, 7e1f23, 641316
	std::array<glm::u8vec4,4> char2_palette_rgba = {
		glm::u8vec4(0x00,0x00,0x00,0x00), // 0 transparent
		glm::u8vec4(0x11,0x0a,0x18,0xff), // 1
		glm::u8vec4(0x7e,0x1f,0x23,0xff), // 2
		glm::u8vec4(0x64,0x13,0x16,0xff)  // 3
	};

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

    // Pack exactly one tile for each character from the same PNG.
	// First character is at tile (0,0); second character at tile (1,0).
	PPU466::Tile char1 = Sprites::pack_png_single_tile(
		data_path("assets/char_tiles.png"), char1_palette_rgba, /*tile_x=*/0, /*tile_y=*/0, /*flip_y=*/true);
	PPU466::Tile char2 = Sprites::pack_png_single_tile(
		data_path("assets/char_tiles.png"), char2_palette_rgba, /*tile_x=*/1, /*tile_y=*/0, /*flip_y=*/true);

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
    // const size_t obst_copy_count = std::min(obst_packed.tiles.size(), ppu.tile_table.size() - obst_tile_base);

    for (size_t i = 0; i < bg_copy_count;   ++i) ppu.tile_table[bg_tile_base   + i] = bg_packed.tiles[i];
    // for (size_t i = 0; i < obst_copy_count; ++i) ppu.tile_table[obst_tile_base + i] = obst_packed.tiles[i];

	// make tile 0 be a fully transparent tile (so solid background color shows):
    PPU466::Tile blank{};
    for (int r = 0; r < 8; ++r) { blank.bit0[r] = 0; blank.bit1[r] = 0; }
    ppu.tile_table[0] = blank;

	ppu.tile_table[char_tile_base + 0] = char1; // index 32
	ppu.tile_table[char_tile_base + 1] = char2; // index 33

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

	// Install character palettes: slot 7 for char1 (used now), slot 5 for char2 (ready for later)
	ppu.palette_table[7][0] = char1_palette_rgba[0];
	ppu.palette_table[7][1] = char1_palette_rgba[1];
	ppu.palette_table[7][2] = char1_palette_rgba[2];
	ppu.palette_table[7][3] = char1_palette_rgba[3];

	ppu.palette_table[5][0] = char2_palette_rgba[0];
	ppu.palette_table[5][1] = char2_palette_rgba[1];
	ppu.palette_table[5][2] = char2_palette_rgba[2];
	ppu.palette_table[5][3] = char2_palette_rgba[3];

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
	// visible viewport in tiles:
	const uint32_t VISIBLE_W = PPU466::ScreenWidth  / 8;
	const uint32_t VISIBLE_H = PPU466::ScreenHeight / 8;

	// choose the wall tile you uploaded (first BG tile at base)
	const uint8_t wall_tile_index = uint8_t(bg_tile_base); // choose first wall tile you uploaded

	// clear to blank tile (so the solid background_color shows through)
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			ppu.background[x + PPU466::BackgroundWidth * y] = 0;
		}
	}

	// ---- draw borders on the *visible* viewport edges ----
	const uint32_t TOP_ROW    = VISIBLE_H - 1;  // top of screen (because origin is bottom-left)
	const uint32_t BOTTOM_ROW = 0;
	const uint32_t LEFT_COL   = 0;
	const uint32_t RIGHT_COL  = VISIBLE_W - 1;

	// top & bottom
	for (uint32_t x = 0; x < VISIBLE_W; ++x) {
		ppu.background[x + PPU466::BackgroundWidth * TOP_ROW   ] = wall_tile_index;
		ppu.background[x + PPU466::BackgroundWidth * BOTTOM_ROW] = wall_tile_index;
	}
	// left & right
	for (uint32_t y = 0; y < VISIBLE_H; ++y) {
		ppu.background[LEFT_COL  + PPU466::BackgroundWidth * y] = wall_tile_index;
		ppu.background[RIGHT_COL + PPU466::BackgroundWidth * y] = wall_tile_index;
	}

	// ---- fill the "10th" and "20th" rows, counted from the TOP ----
	// (You originally described them as human "10th"/"20th" from the top.)
	auto row_from_top = [&](uint32_t top_index)->std::optional<uint32_t>{
		if (top_index >= VISIBLE_H) return std::nullopt;
		return VISIBLE_H - 1 - top_index; // convert "top row index" -> bottom-left origin
	};

	if (auto y10 = row_from_top(10); y10) {
		for (uint32_t x = 0; x < VISIBLE_W; ++x) {
			ppu.background[x + PPU466::BackgroundWidth * *y10] = wall_tile_index;
		}
	}
	if (auto y20 = row_from_top(20); y20) {
		for (uint32_t x = 0; x < VISIBLE_W; ++x) {
			ppu.background[x + PPU466::BackgroundWidth * *y20] = wall_tile_index;
		}
	}

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
	// ppu.sprites[0].x = int8_t(player_at.x);
	// ppu.sprites[0].y = int8_t(player_at.y);
	// ppu.sprites[0].index = 32;
	// ppu.sprites[0].attributes = 7;

	// --- draw first character as a 3x3 meta-sprite at (row 5, col 16) counted from TOP-LEFT ---
	{
		const uint32_t VISIBLE_H = PPU466::ScreenHeight / 8;
		const int tile_col_from_left = 16;
		const int tile_row_from_top  = 5;

		const int base_x = tile_col_from_left * 8;
		const int base_y = int((VISIBLE_H - (tile_row_from_top + 1)) * 8); // +1 because sprite is 1 tile tall

		player_at.x = float(base_x);
    	player_at.y = float(base_y);

		ppu.sprites[0].x = int8_t(std::round(player_at.x));
		ppu.sprites[0].y = int8_t(std::round(player_at.y));
		ppu.sprites[0].index = 32; // char_tile_base + 0
		ppu.sprites[0].attributes = 7; // palette slot for first character
	}


	// //some other misc sprites:
	// for (uint32_t i = 1; i < 63; ++i) {
	// 	float amt = (i + 2.0f * background_fade) / 62.0f;
	// 	ppu.sprites[i].x = int8_t(0.5f * float(PPU466::ScreenWidth) + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player_at.x) * 0.4f * float(PPU466::ScreenWidth));
	// 	ppu.sprites[i].y = int8_t(0.5f * float(PPU466::ScreenHeight) + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player_at.y) * 0.4f * float(PPU466::ScreenWidth));
	// 	ppu.sprites[i].index = 64;
	// 	ppu.sprites[i].attributes = 6;
	// 	if (i % 2) ppu.sprites[i].attributes |= 0x80; //'behind' bit
	// }

	//--- actually draw ---
	ppu.draw(drawable_size);
}
