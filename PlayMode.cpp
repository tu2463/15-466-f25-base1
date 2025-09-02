#include "PlayMode.hpp"
#include "Sprites.hpp"
#include "data_path.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

// Credit: Used ChatGPT for assistance
struct DoorInstance { int8_t x; int8_t y; uint8_t index; uint8_t attributes; };
static std::vector<DoorInstance> g_door_sprites;

// door tile indices (filled in constructor after uploading door tiles)
static uint8_t s_door_up_red    = 0;
static uint8_t s_door_down_red  = 0;
static uint8_t s_door_up_grey   = 0;
static uint8_t s_door_down_grey = 0;
static uint8_t s_char_red_idx = 0;       // will be set in the constructor
static uint8_t s_char_red_palette = 5;   // your red character palette slot

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

    // --- 2) Pack three tilesheets, PNG -> PPU ---
    auto bg_packed = Sprites::pack_png_tileset(
        data_path("assets/bg_tiles.png"),
        bg_palette_rgba
    );

	auto char1_packed = Sprites::pack_png_tileset(
		data_path("assets/char_red_tiles.png"),
        char2_palette_rgba
    );

	auto char2_packed = Sprites::pack_png_tileset(
        data_path("assets/char_red_tiles.png"),
        char2_palette_rgba
    );

    // // Pack exactly one tile for each character from the same PNG.
	// // First character is at tile (0,0); second character at tile (1,0).
	// PPU466::Tile char1 = Sprites::pack_png_single_tile(
	// 	data_path("assets/char_tiles.png"), char1_palette_rgba, /*tile_x=*/0, /*tile_y=*/0, /*flip_y=*/true);
	// PPU466::Tile char2 = Sprites::pack_png_single_tile(
	// 	data_path("assets/char_tiles.png"), char2_palette_rgba, /*tile_x=*/1, /*tile_y=*/0, /*flip_y=*/true);

    // --- 3) Upload into fixed tile table ranges ---
    //    Should truncate for Game1 if there are. >256 tiles.
    const size_t bg_tile_base   = 1;
    const size_t char1_tile_base = 10;
    const size_t char2_tile_base = 20;
    // const size_t obst_tile_base = 64;

	// calculates the number of tiles to copy into tile_table, ensuring it does not exceed the size of tile_table
    const size_t bg_copy_count   = std::min(bg_packed.tiles.size(),   ppu.tile_table.size() - bg_tile_base);
    const size_t char1_copy_count   = std::min(char1_packed.tiles.size(),   ppu.tile_table.size() - char1_tile_base);
    const size_t char2_copy_count   = std::min(char2_packed.tiles.size(),   ppu.tile_table.size() - char2_tile_base);

    for (size_t i = 0; i < bg_copy_count;   ++i) ppu.tile_table[bg_tile_base   + i] = bg_packed.tiles[i];
    for (size_t i = 0; i < char1_copy_count;   ++i) ppu.tile_table[char1_tile_base   + i] = char1_packed.tiles[i];
    for (size_t i = 0; i < char2_copy_count;   ++i) ppu.tile_table[char2_tile_base   + i] = char2_packed.tiles[i];
	
	// first red character tile from the red sheet:
	s_char_red_idx = uint8_t(char2_tile_base + 0);
	// palette for that red character:
	s_char_red_palette = 5;

	// make tile 0 be a fully transparent tile (so solid background color shows):
    PPU466::Tile blank{};
    for (int r = 0; r < 8; ++r) { blank.bit0[r] = 0; blank.bit1[r] = 0; }
    ppu.tile_table[0] = blank;

	// ppu.tile_table[char_tile_base + 0] = char1;
	// ppu.tile_table[char_tile_base + 1] = char2;

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

	// ---- fill rows counted from TOP, using 1-based "row1" ----
	// y index for a top-left "row1" (1..VISIBLE_H):
	auto y_from_top_row1 = [&](uint32_t row1)->std::optional<uint32_t>{
		if (row1 == 0 || row1 > VISIBLE_H) return std::nullopt;
		return (VISIBLE_H - row1); // row1=1 -> top row (VISIBLE_H-1), row1=VISIBLE_H -> 0
	};

	// rows 10 and 20 from the TOP (1-based):
	if (auto y10 = y_from_top_row1(10); y10) {
		for (uint32_t x = 0; x < VISIBLE_W; ++x) {
			ppu.background[x + PPU466::BackgroundWidth * *y10] = wall_tile_index;
		}
	}
	if (auto y20 = y_from_top_row1(20); y20) {
		for (uint32_t x = 0; x < VISIBLE_W; ++x) {
			ppu.background[x + PPU466::BackgroundWidth * *y20] = wall_tile_index;
		}
	}


    // Optional background color (kept from your version):
    ppu.background_color = glm::u8vec4(0x11,0x0a,0x18,0xff);

	// Start the player at (row 5, col 16) measured from TOP-LEFT, once:
	{
		const uint32_t VISIBLE_H = PPU466::ScreenHeight / 8;
		const int tile_col_from_left = 16;
		const int tile_row_from_top  = 5;

		const int start_x = tile_col_from_left * 8;
		const int start_y = int((VISIBLE_H - (tile_row_from_top + 1)) * 8); // 1 tile tall

		player_at.x = float(start_x);
		player_at.y = float(start_y);
	}

	// -------- Doors: four 8x8 tiles with shared palette: transparent, 7e1f23, 5f6268 --------
	std::array<glm::u8vec4,4> door_palette_rgba = {
		glm::u8vec4(0x00,0x00,0x00,0x00), // 0 transparent
		glm::u8vec4(0x7e,0x1f,0x23,0xff), // 1 red   (#7e1f23)
		glm::u8vec4(0x5f,0x62,0x68,0xff), // 2 grey  (#5f6268)
		glm::u8vec4(0x5f,0x62,0x68,0xff)  // 3 duplicate (unused filler)
	};

	// load the 4 door tiles; assumes each file is a single 8x8 PNG
	PPU466::Tile door_up_red   = Sprites::pack_png_single_tile(data_path("assets/door_up_red.png"),   door_palette_rgba, 0, 0, /*flip_y=*/true);
	PPU466::Tile door_down_red = Sprites::pack_png_single_tile(data_path("assets/door_down_red.png"), door_palette_rgba, 0, 0, /*flip_y=*/true);
	PPU466::Tile door_up_grey  = Sprites::pack_png_single_tile(data_path("assets/door_up_grey.png"),  door_palette_rgba, 0, 0, /*flip_y=*/true);
	PPU466::Tile door_down_grey= Sprites::pack_png_single_tile(data_path("assets/door_down_grey.png"),door_palette_rgba, 0, 0, /*flip_y=*/true);

	// upload into safe tile slots (doesn't collide with your bases 1,10,20)
	const size_t door_tile_base = 30;

	ppu.tile_table[door_tile_base + 0] = door_up_red;
	ppu.tile_table[door_tile_base + 1] = door_down_red;
	ppu.tile_table[door_tile_base + 2] = door_up_grey;
	ppu.tile_table[door_tile_base + 3] = door_down_grey;

	// put the door colors in palette slot 6 (independent of BG palette 0)
	ppu.palette_table[6][0] = door_palette_rgba[0];
	ppu.palette_table[6][1] = door_palette_rgba[1];
	ppu.palette_table[6][2] = door_palette_rgba[2];
	ppu.palette_table[6][3] = door_palette_rgba[3];

	// ---- Precompute sprite positions for doors (row/col start at 1, counted from TOP-LEFT) ----
	{
		const uint32_t VISIBLE_H = PPU466::ScreenHeight / 8;

		auto add_door_row = [&](uint8_t tile_index, int row1, int col_start1, int col_end1) {
			// convert top-left (row1) to bottom-left pixel y:
			int y_pixels = int((VISIBLE_H - row1) * 8);
			for (int c = col_start1; c <= col_end1; ++c) {
				int x_pixels = (c - 1) * 8;
				g_door_sprites.push_back(DoorInstance{
					int8_t(x_pixels), int8_t(y_pixels),
					tile_index, /*attributes=*/6 // palette slot 6, in front (no behind bit)
				});
			}
		};

		// remember door tile indices for collision/trigger logic:
		s_door_up_red    = uint8_t(door_tile_base + 0);
		s_door_down_red  = uint8_t(door_tile_base + 1);
		s_door_up_grey   = uint8_t(door_tile_base + 2);
		s_door_down_grey = uint8_t(door_tile_base + 3);

		// more door placements (rows/cols 1-based, from TOP-LEFT):
		// door_up_red: row 9 col 6-9, row 9 col 16-19, row 19 col 9-12
		add_door_row(s_door_up_red,   /*row*/9,  /*c0*/6,  /*c1*/9);
		add_door_row(s_door_up_red,   /*row*/9,  /*c0*/16, /*c1*/19);
		add_door_row(s_door_up_red,   /*row*/19, /*c0*/9,  /*c1*/12);

		// door_up_grey: row 9 col 26-29, row 19 col 21-24
		add_door_row(s_door_up_grey,  /*row*/9,  /*c0*/26, /*c1*/29);
		add_door_row(s_door_up_grey,  /*row*/19, /*c0*/21, /*c1*/24);

		// door_down_red: row 11 col 16-19, row 11 col 26-29, row 19 col 21-24
		add_door_row(s_door_down_red, /*row*/11, /*c0*/16, /*c1*/19);
		add_door_row(s_door_down_red, /*row*/11, /*c0*/26, /*c1*/29);
		add_door_row(s_door_down_red, /*row*/21, /*c0*/21, /*c1*/24);

		// door_down_grey: row 11 col 6-9, row 19 col 9-12
		add_door_row(s_door_down_grey,/*row*/11, /*c0*/6,  /*c1*/9);
		add_door_row(s_door_down_grey,/*row*/21, /*c0*/9,  /*c1*/12);
	}
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
    constexpr float PlayerSpeed = 60.0f;
    constexpr int   TileSize    = 8;

    // desired motion this frame:
    float dx = 0.0f, dy = 0.0f;
    if (left.pressed)  {
		dx -= PlayerSpeed * elapsed; 
		// printf("left, dx: %f\n", dx);
	}
    if (right.pressed) {
		dx += PlayerSpeed * elapsed; 
		// printf("right, dx: %f\n", dx);
	}
    if (down.pressed) {
		dy -= PlayerSpeed * elapsed;
		// printf("down, dy: %f\n", dy);
	}
    if (up.pressed)    {
		dy += PlayerSpeed * elapsed;
		// printf("up, dy: %f\n", dy);
	}

    // visible viewport in tiles (your walls live here):
    const int VISIBLE_W = int(PPU466::ScreenWidth  / TileSize);
    const int VISIBLE_H = int(PPU466::ScreenHeight / TileSize);

    // helper: treat outside the viewport as solid; inside: solid if background tile != 0
    auto is_solid_tile = [&](int tx, int ty) -> bool {
        if (tx < 0 || ty < 0 || tx >= VISIBLE_W || ty >= VISIBLE_H) return true;
        uint8_t idx = ppu.background[tx + PPU466::BackgroundWidth * ty];
        return (idx != 0); // you reserved tile 0 as blank; all others are walls for now
    };

    // --- move along X, then resolve collisions ---
    if (dx != 0.0f) {
        float new_x = player_at.x + dx;
        // compute the rows overlapped at current Y:
        int y0 = int(std::floor(player_at.y / float(TileSize)));
        int y1 = int(std::floor((player_at.y + (TileSize - 1)) / float(TileSize)));

        if (dx > 0.0f) {
            int right_edge = int(std::floor((new_x + (TileSize - 1)) / float(TileSize)));
            bool blocked = false;
            for (int ty = y0; ty <= y1; ++ty) {
                if (is_solid_tile(right_edge, ty)) { blocked = true; break; }
            }
            if (blocked) {
                // place flush-left against the blocking tile:
                new_x = float(right_edge * TileSize - TileSize);
            }
        } else { // dx < 0
            int left_edge = int(std::floor(new_x / float(TileSize)));
            bool blocked = false;
            for (int ty = y0; ty <= y1; ++ty) {
                if (is_solid_tile(left_edge, ty)) { blocked = true; break; }
            }
            if (blocked) {
                // place flush-right against the blocking tile:
                new_x = float((left_edge + 1) * TileSize);
            }
        }
        player_at.x = new_x;
		printf("player_at.x after collision: %f\n", player_at.x);
    }

    // --- move along Y, then resolve collisions ---
    if (dy != 0.0f) {
        float new_y = player_at.y + dy;
        // compute the columns overlapped at current X:
        int x0 = int(std::floor(player_at.x / float(TileSize)));
        int x1 = int(std::floor((player_at.x + (TileSize - 1)) / float(TileSize)));

        if (dy > 0.0f) {
            int top_edge = int(std::floor((new_y + (TileSize - 1)) / float(TileSize)));
            bool blocked = false;
            for (int tx = x0; tx <= x1; ++tx) {
                if (is_solid_tile(tx, top_edge)) { blocked = true; break; }
            }
            if (blocked) {
                // place flush-below the blocking tile:
                new_y = float(top_edge * TileSize - TileSize);
            }
        } else { // dy < 0
            int bottom_edge = int(std::floor(new_y / float(TileSize)));
            bool blocked = false;
            for (int tx = x0; tx <= x1; ++tx) {
                if (is_solid_tile(tx, bottom_edge)) { blocked = true; break; }
            }
            if (blocked) {
                // place flush-above the blocking tile:
                new_y = float((bottom_edge + 1) * TileSize);
            }
        }
        player_at.y = new_y;
		printf("player_at.y after collision: %f\n", player_at.y);
    }

    // reset button press counters (keep your original behavior):
    left.downs = right.downs = up.downs = down.downs = 0;

	// ---- door triggers: touching a door moves the player ±3 rows (from TOP-LEFT semantics) ----
	{
		constexpr int TileSize = 8;
		const int VISIBLE_H = int(PPU466::ScreenHeight / TileSize);

		// player AABB (8x8) at current position:
		int px0 = int(std::floor(player_at.x));
		int py0 = int(std::floor(player_at.y));
		int px1 = px0 + TileSize;
		int py1 = py0 + TileSize;

		auto aabb_overlap = [&](int ax0,int ay0,int ax1,int ay1, int bx0,int by0,int bx1,int by1)->bool{
			return (ax0 < bx1 && ax1 > bx0 && ay0 < by1 && ay1 > by0);
		};

		// check overlap with any door sprite (each 8x8)
		for (const auto& d : g_door_sprites) {
			int dx0 = int(d.x), dy0 = int(d.y);
			int dx1 = dx0 + TileSize, dy1 = dy0 + TileSize;
			if (!aabb_overlap(px0,py0,px1,py1, dx0,dy0,dx1,dy1)) {
				printf("player is at (%d, %d), no overlap with door at (%d,%d)\n", px0, py0, int(d.x), int(d.y));
				continue;
			}
			printf("player (%d %d) TOUCHING door at (%d,%d) tile index %d\n", px0, py0, int(d.x), int(d.y), int(d.index));

			// touching a door: change row by ±3 (1 row = 8 px). TOP-LEFT counting means:
			// row += 3  -> move downward in pixels -> y -= 24
			// row -= 3  -> move upward   in pixels -> y += 24
			if (d.index == s_door_up_red || d.index == s_door_up_grey) {
				player_at.y -= 3 * TileSize;
			} else if (d.index == s_door_down_red || d.index == s_door_down_grey) {
				player_at.y += 3 * TileSize;
			}

			// clamp to visible vertical range (keep fully on-screen)
			if (player_at.y < 0.0f) player_at.y = 0.0f;
			float maxY = float((VISIBLE_H - 1) * TileSize);
			if (player_at.y > maxY) player_at.y = maxY;

			// stop after the first door we touch this frame
			break;
		}
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	ppu.background_color = glm::u8vec4(0x11,0x0a,0x18,0xff);

	ppu.background_position.x = 0;
    ppu.background_position.y = 0;

	//player sprite:
	ppu.sprites[0].x = int8_t(int(player_at.x + 0.5f));
	ppu.sprites[0].y = int8_t(int(player_at.y + 0.5f));
	ppu.sprites[0].index = 10;     // first character tile
	ppu.sprites[0].attributes = 7; // character palette

	// draw door sprites starting at hardware sprite slot 1:
	{
		const size_t base_slot = 1;
		const size_t max_slots = 63; // PPU has 64 hardware sprites (0..63)
		const size_t count = std::min(g_door_sprites.size(), max_slots - base_slot);

		for (size_t i = 0; i < count; ++i) {
			const auto &d = g_door_sprites[i];
			ppu.sprites[base_slot + i].x = d.x;
			ppu.sprites[base_slot + i].y = d.y;
			ppu.sprites[base_slot + i].index = d.index;
			ppu.sprites[base_slot + i].attributes = d.attributes;
		}
		// (optional) clear any remaining sprite slots if you previously used them
	}

	// add a red character sprite at the bottom, centered:
	{
		// use the last hardware sprite slot to avoid colliding with doors (0 is player, 1.. are doors)
		const size_t red_char_slot = 63;

		// center horizontally for an 8x8 tile; bottom row (y = 0 in PPU coordinates)
		const int center_x = int((PPU466::ScreenWidth - 8) / 2);

		const int row_from_bottom = 5;
		const int bottom_y = (row_from_bottom - 1) * 8;

		ppu.sprites[red_char_slot].x = int8_t(center_x);
		ppu.sprites[red_char_slot].y = int8_t(bottom_y);
		ppu.sprites[red_char_slot].index = s_char_red_idx;       // e.g., 20
		ppu.sprites[red_char_slot].attributes = s_char_red_palette; // 5
	}


	//--- actually draw ---
	ppu.draw(drawable_size);
}
