#include "PlayMode.hpp"
#include "Sprites.hpp"
#include "data_path.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

// Credit: Used ChatGPT for assistance
// --- success / win flow ---
enum class GamePhase { Playing, WinWait, WinSlide };
static GamePhase g_phase = GamePhase::Playing;

const int center_x = int((PPU466::ScreenWidth - 8) / 2);
const int row_from_bottom = 5;
const int bottom_y = (row_from_bottom - 1) * 8;
static glm::vec2 s_target_red_pos = glm::vec2({center_x, bottom_y}); // bottom red character world pos

static constexpr float kWinDelaySeconds   = 1.0f;  // pause before sliding
static constexpr float kSlideStepSeconds  = 0.25f;  // move cadence
static constexpr int   kSlideStepPixels   = 8;     // 1 column

static float g_win_wait_remaining = 0.0f;
static float g_slide_accum = 0.0f;

static bool g_player_hidden = false; // hide once off-screen
static bool g_target_hidden = false;

// --- door  ---
enum class DoorColor { Grey, Red };

struct DoorInstance {
    int x;
    int y;
    uint8_t index;
    uint8_t attributes;
    DoorColor self_color;
    DoorColor paired_color;
    bool is_up; // true = up door, false = down door
};

static std::vector<DoorInstance> g_door_sprites;

// door tile indices (filled in constructor after uploading door tiles)
static uint8_t s_door_up_red    = 0;
static uint8_t s_door_down_red  = 0;
static uint8_t s_door_up_grey   = 0;
static uint8_t s_door_down_grey = 0;
static uint8_t s_char_grey_idx = 0;
static uint8_t s_char_grey_palette = 7;
static uint8_t s_char_red_idx = 0;
static uint8_t s_char_red_palette = 5;

enum class PlayerColor { Grey, Red };
static PlayerColor player_color = PlayerColor::Grey;  // default starting state

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

	// after uploading char1 (grey) tiles:
	const size_t char1_tile_base = 10;
	s_char_grey_idx = uint8_t(char1_tile_base + 0);
	s_char_grey_palette = 7;

	// after uploading char2 (red) tiles:
	const size_t char2_tile_base = 20;
	s_char_red_idx = uint8_t(char2_tile_base + 0);
	s_char_red_palette = 5;

	// calculates the number of tiles to copy into tile_table, ensuring it does not exceed the size of tile_table
    const size_t bg_copy_count   = std::min(bg_packed.tiles.size(),   ppu.tile_table.size() - bg_tile_base);
    const size_t char1_copy_count   = std::min(char1_packed.tiles.size(),   ppu.tile_table.size() - char1_tile_base);
    const size_t char2_copy_count   = std::min(char2_packed.tiles.size(),   ppu.tile_table.size() - char2_tile_base);

    for (size_t i = 0; i < bg_copy_count;   ++i) ppu.tile_table[bg_tile_base   + i] = bg_packed.tiles[i];
    for (size_t i = 0; i < char1_copy_count;   ++i) ppu.tile_table[char1_tile_base   + i] = char1_packed.tiles[i];
    for (size_t i = 0; i < char2_copy_count;   ++i) ppu.tile_table[char2_tile_base   + i] = char2_packed.tiles[i];

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

		auto add_door_row = [&](uint8_t tile_index,
								int row1, int col_start1, int col_end1,
								DoorColor self, DoorColor paired, bool is_up) {
			int y_pixels = int((VISIBLE_H - row1) * 8);
			for (int c = col_start1; c <= col_end1; ++c) {
				int x_pixels = (c - 1) * 8;
				g_door_sprites.push_back(DoorInstance{
					x_pixels, y_pixels,
					tile_index, /*attributes=*/6,
					self, paired, is_up
				});
			}
		};

		// remember door tile indices for collision/trigger logic:
		s_door_up_red    = uint8_t(door_tile_base + 0);
		s_door_down_red  = uint8_t(door_tile_base + 1);
		s_door_up_grey   = uint8_t(door_tile_base + 2);
		s_door_down_grey = uint8_t(door_tile_base + 3);

		// door_up_red: row 9 col 6-9, row 9 col 16-19, row 19 col 9-12
		add_door_row(s_door_up_red, 9, 6, 9, DoorColor::Red, DoorColor::Grey, true);
		add_door_row(s_door_up_red, 9, 16, 19, DoorColor::Red, DoorColor::Red, true);
		add_door_row(s_door_up_red, 19, 9, 12, DoorColor::Red, DoorColor::Grey, true);

		// door_up_grey: row 9 col 26-29, row 19 col 21-24
		add_door_row(s_door_up_grey, 9, 26, 29, DoorColor::Grey, DoorColor::Red, true);
		add_door_row(s_door_up_grey, 19, 21, 24, DoorColor::Grey, DoorColor::Red, true);

		// door_down_red: row 11 col 16-19, row 11 col 26-29, row 21 col 21-24
		add_door_row(s_door_down_red, 11, 16, 19, DoorColor::Red, DoorColor::Red, false);
		add_door_row(s_door_down_red, 11, 26, 29, DoorColor::Red, DoorColor::Grey, false);
		add_door_row(s_door_down_red, 21, 21, 24, DoorColor::Red, DoorColor::Grey, false);

		// door_down_grey: row 11 col 6-9, row 21 col 9-12
		add_door_row(s_door_down_grey, 11, 6, 9, DoorColor::Grey, DoorColor::Red, false);
		add_door_row(s_door_down_grey, 21, 9, 12, DoorColor::Grey, DoorColor::Red, false);
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
    constexpr float PlayerSpeed = 90.0f;
    constexpr int   TileSize    = 8;

	// --------- Playing phase: normal movement + collisions + doors ----------
    if (g_phase == GamePhase::Playing) {
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
			// printf("player_at.x after collision: %f\n", player_at.x);
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
			// printf("player_at.y after collision: %f\n", player_at.y);
		}

		// reset button press counters (keep your original behavior):
		left.downs = right.downs = up.downs = down.downs = 0;

		// ---- door triggers: touching a door moves the player ±3 rows (from TOP-LEFT semantics) ----
		{
			constexpr int TileSize = 8;
			// check overlap with any door sprite (each 8x8)
			for (auto const &door : g_door_sprites) {
				bool overlap = (player_at.x < door.x + TileSize &&
								player_at.x + TileSize > door.x &&
								player_at.y < door.y + TileSize &&
								player_at.y + TileSize > door.y);

				if (!overlap) continue;

				// check if player's current color matches this door's self color
				PlayerColor as_player_color = (player_color == PlayerColor::Grey) ? PlayerColor::Grey : PlayerColor::Red;
				DoorColor as_door_self = door.self_color;

				if ((as_player_color == PlayerColor::Grey && as_door_self == DoorColor::Grey) ||
					(as_player_color == PlayerColor::Red && as_door_self == DoorColor::Red)) {

					// move ±3 rows depending on up/down
					if (door.is_up) {
						// printf("Triggering UP door at (%d,%d)\n", door.x, door.y);
						player_at.y -= 4 * TileSize;
					} else {
						// printf("Triggering DOWN door at (%d,%d)\n", door.x, door.y);
						player_at.y += 4 * TileSize;
					}

					// change to the paired door’s color
					if (door.paired_color == DoorColor::Grey) {
						player_color = PlayerColor::Grey;
						// printf("Player color changed to GREY\n");
					} else {
						// printf("Player color changed to RED\n");
						player_color = PlayerColor::Red;
					}

					break; // only trigger one door per frame
				}
			}
		}

		// --- success trigger: red player touches target red character ---
		if (player_color == PlayerColor::Red)
		{
			bool overlap_target =
				(player_at.x < s_target_red_pos.x + TileSize) &&
				(player_at.x + TileSize > s_target_red_pos.x) &&
				(player_at.y < s_target_red_pos.y + TileSize) &&
				(player_at.y + TileSize > s_target_red_pos.y);

			if (overlap_target)
			{
				// enter win wait phase
				g_phase = GamePhase::WinWait;
				g_win_wait_remaining = kWinDelaySeconds;

				// stop controls immediately
				left.pressed = right.pressed = up.pressed = down.pressed = false;
				left.downs = right.downs = up.downs = down.downs = 0;
			}
			return;
		}
	}

	// --------- WinWait phase: freeze for 1 second ----------
    if (g_phase == GamePhase::WinWait) {
        g_win_wait_remaining -= elapsed;
        // lock out controls
        left.pressed = right.pressed = up.pressed = down.pressed = false;
        left.downs = right.downs = up.downs = down.downs = 0;

        if (g_win_wait_remaining <= 0.0f) {
            g_phase = GamePhase::WinSlide;
            g_slide_accum = 0.0f;
        }
        return;
    }

    // --------- WinSlide phase: move both right 1 col / 0.5s until off-screen ----------
    if (g_phase == GamePhase::WinSlide) {
        g_slide_accum += elapsed;
        while (g_slide_accum >= kSlideStepSeconds) {
            g_slide_accum -= kSlideStepSeconds;

            player_at.x       += float(kSlideStepPixels);
            s_target_red_pos.x += float(kSlideStepPixels);

            // hide once off right edge (prevents wrap visuals)
            if (player_at.x >= float(PPU466::ScreenWidth))  g_player_hidden = true;
            if (s_target_red_pos.x >= float(PPU466::ScreenWidth)) g_target_hidden = true;
        }

        // controls remain disabled
        left.pressed = right.pressed = up.pressed = down.pressed = false;
        left.downs = right.downs = up.downs = down.downs = 0;

        return;
    }
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	//--- set ppu state based on game state ---

	ppu.background_color = glm::u8vec4(0x11,0x0a,0x18,0xff);

	ppu.background_position.x = 0;
    ppu.background_position.y = 0;

	//player sprite:
	{
		const size_t player_slot = 0;

		uint8_t idx = (player_color == PlayerColor::Grey) ? s_char_grey_idx : s_char_red_idx;
		uint8_t pal = (player_color == PlayerColor::Grey) ? s_char_grey_palette : s_char_red_palette;

		int draw_x = int(player_at.x);
		int draw_y = int(player_at.y);

		if (g_player_hidden) draw_y = 255;

		ppu.sprites[player_slot].x = int8_t(draw_x);
		ppu.sprites[player_slot].y = int8_t(draw_y);
		ppu.sprites[player_slot].index = idx;
		ppu.sprites[player_slot].attributes = pal;
	}


	// draw door sprites starting at hardware sprite slot 1:
	{
		const size_t base_slot = 1;
		const size_t max_slots = 63; // PPU has 64 hardware sprites (0..63)
		const size_t count = std::min(g_door_sprites.size(), max_slots - base_slot);

		for (size_t i = 0; i < count; ++i) {
			ppu.sprites[base_slot + i].x = int8_t(g_door_sprites[i].x);
			ppu.sprites[base_slot + i].y = int8_t(g_door_sprites[i].y);
			ppu.sprites[base_slot + i].index = g_door_sprites[i].index;
			ppu.sprites[base_slot + i].attributes = g_door_sprites[i].attributes;

		}
		// (optional) clear any remaining sprite slots if you previously used them
	}

	// target red character sprite (slot 63)
	{
		const size_t red_char_slot = 63;

		int draw_x = int(s_target_red_pos.x);
		int draw_y = int(s_target_red_pos.y);

		if (g_target_hidden) draw_y = 255; // NES-style 'offscreen'

		ppu.sprites[red_char_slot].x = int8_t(draw_x);
		ppu.sprites[red_char_slot].y = int8_t(draw_y);
		ppu.sprites[red_char_slot].index = s_char_red_idx;
		ppu.sprites[red_char_slot].attributes = s_char_red_palette;
	}


	//--- actually draw ---
	ppu.draw(drawable_size);
}
