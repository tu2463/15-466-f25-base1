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
    // 1) Define the 4-color palette your PNG actually uses:
    //    Order matters: indices 0..3 become PPU color slots 0..3.
    std::array<glm::u8vec4,4> my_palette = {
        glm::u8vec4(0,   0,   0,   0x00), // index 0: transparent (alpha=0) or solid bg if you want
        glm::u8vec4(0x7f,0x7f,0x7f,0xff), // index 1: mid gray
        glm::u8vec4(0xff,0xff,0xff,0xff), // index 2: white
        glm::u8vec4(0x00,0xa0,0xff,0xff), // index 3: blue
    };

    // 2) Pack PNG -> PPU tiles:
    auto packed = Sprites::pack_png_tileset(
        data_path("assets/tileset.png"), // lives under dist/assets/
        my_palette
    );

    // 3) Upload tiles into the fixed tile table (starting at 0).
    //    Should truncate for Game1 if there are. >256 tiles.

	// calculates the number of tiles to copy into tile_table, ensuring it does not exceed the size of tile_table
    const size_t tiles_copy_count = std::min(packed.tiles.size(), size_t(ppu.tile_table.size()));
    for (size_t i = 0; i < tiles_copy_count; ++i) {
        ppu.tile_table[i] = packed.tiles[i];
    }

    // 4) Upload palette to a slot you’ll use (e.g., slot 0 for BG, 7 for player):
	// NOTE that packed.palette is exactly the same as my_palette, refer to the pack_png_tileset function - is it correct?
	/* from PPU466.hpp:
	typedef std::array< glm::u8vec4, 4 > Palette;
	std::array< Palette, 8 > palette_table; */
	auto &bg_palette = ppu.palette_table[0];
	bg_palette[0] = packed.palette[0];
	bg_palette[1] = packed.palette[1];
	bg_palette[2] = packed.palette[2];
	bg_palette[3] = packed.palette[3];

    ppu.palette_table[7] = ppu.palette_table[0]; // placeholder: player uses the same for now

    // 5) Make a static background using your first tiles.
    //    Replace this with a CSV map later if you like.
    for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
        for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
            // simple checker using the first few tiles so you can see something:
            uint8_t tile_idx = uint8_t( ( (x/2) + (y/2) ) % std::max<uint32_t>(1, packed.tiles_w) ); //?? how does this math work
            ppu.background[x + PPU466::BackgroundWidth * y] = tile_idx; // tile index
        }
    }

    // 6) Put the player sprite on screen using some tile index you own (e.g., 0).
    ppu.sprites[0].index = 32;     // choose a visible tile from your sheet
    ppu.sprites[0].attributes = 7; // palette slot 7 (copied from 0 above)

    // Optionally set background color if you want:
    ppu.background_color = glm::u8vec4(0,0,0,0xff);
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
	ppu.background_color = glm::u8vec4(
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 0.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 1.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 2.0f / 3.0f) ) ) ))),
		0xff
	);

	// //tilemap gets recomputed every frame as some weird plasma thing:
	// //NOTE: don't do this in your game! actually make a map or something :-)
	// for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
	// 	for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
	// 		//TODO: make weird plasma thing
	// 		ppu.background[x+PPU466::BackgroundWidth*y] = ((x+y)%16);
	// 	}
	// }

	//background scroll:
	ppu.background_position.x = int32_t(-0.5f * player_at.x);
	ppu.background_position.y = int32_t(-0.5f * player_at.y);

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
		ppu.sprites[i].index = 32;
		ppu.sprites[i].attributes = 6;
		if (i % 2) ppu.sprites[i].attributes |= 0x80; //'behind' bit
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
