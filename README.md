Get to My Friend

Author: Cheyu Tu

Design:
A grey little creature wants make itself red so that it can make friends with the red little creature. It discovers that the doors can change its color...

Screen Shot:

![Game Screenshot](dist/assets/screenshot_2.png)

How Your Asset Pipeline Works:

Credit: Used ChatGPT for assistance
1. Author PNGs (GIMP):
Draw pixel art on an 8×8 grid. Each sheet uses ≤4 colors (index 0 = transparent). Current palettes:
• Background: #301e21, #463537, #110a18
• Characters & doors use their own 4-color palettes. Save as PNG. 

2. Load & pack at runtime:
PNGs are loaded with load_png(...) and converted to PPU tiles using Sprites::pack_png_tileset(...) for sheets and Sprites::pack_png_single_tile(...) for individual 8×8 tiles. The packer maps RGBA → 2-bit palette indices. 

3. Upload tiles & palettes to the PPU:
Packed tiles are copied into fixed slots in ppu.tile_table (e.g., background at base 1, characters at 10 and 20, doors at 30), and their 4-color palettes are written to ppu.palette_table (BG in slot 0; characters in 7 and 5; doors in 6). Tile 0 is a blank/transparent tile. 

4. Build the background map:
Clear ppu.background to tile 0 so the solid background_color shows, then place wall tiles along the screen borders and on specific rows (counted from the top) by writing indices into the background tilemap. 

5. Place sprites for player and props (doors):
The player uses a character tile and is drawn from a live player_at position each frame. Doors are loaded as single 8×8 tiles, assigned a shared palette, and placed as sprites at specified (row, col) locations (1-based, from top-left). Door overlap triggers row±3 movement; solid-tile collision comes from the background map (non-zero tiles are solid). 

Source files: they are in dist/assets

How To Play:

Control: use the arrow keys to control the little creature.

Goal: let the little creature reach the red creature at the bottom, and make sure that it becomes red when it reach it.

This game was built with [NEST](NEST.md).

Base code for Game 1: http://graphics.cs.cmu.edu/courses/15-466-f25/game1.html

