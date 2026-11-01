# Leanna the Lion - Game Boy (DMG)

Port of the Arduboy version, using the Arduboy 1-bit PNGs.

- `gfx/` holds every image the game uses. Edit those PNGs and run `build.bat`: it converts them to tile data
  (`tools/convert_assets.ps1` -> `src/gfx_data.*`) and then compiles `build/leanna.gb` with GBDK-2020
  (set `GBDK_HOME` if it is not at the default path).
- Debug builds (compile flags): `-DDEBUG_BOT` auto-plays, `-DDEBUG_NODEATH`, `-DDEBUG_LEVEL=n` starts at level n,
  `-DDEBUG_FORCE_TYPE=n` spawns one entity type, `-DDEBUG_PERF` writes frame timing to cart RAM
  (dropped frames = vblanks - loop frames, read from the .sav), `-DINTRO_FRAMES=n`.

## Layout (screen lines)

| lines | what |
|---|---|
| 0-15 | HUD |
| 16-31 | background fence |
| 26-57 | truck lane (truck drives between the two top fences) |
| 58-65 | foreground rail |
| 79 / 103 / 127 | bottoms of lanes 1-3 (24px apart, original 12px) |
| 128-135 | foreground rail |

The lion changes lanes at 2px/frame (original 1px) so a lane change takes the same time as before.

## Differences from the Arduboy version

- Full 160px width; entities spawn at the screen edge (`SPAWN_X`, was 128).
- Trucks spawn 64px further off screen (`TRUCK_SPAWN_X`); their sight line (45 degrees, 96px) shows while the truck is still off screen.
- Sprites always draw in front of the fences. No "save high score?" prompt (the high score always saves to cart RAM).
- The monkey poo uses the small 6x6 hit box, as in the shipped Arduboy build.

## Performance notes (SDCC on the Game Boy CPU)

- Each moving object keeps a fixed run of OAM slots; per frame only x (and y) is written.
  Rebuilding every sprite each frame took longer than a frame once a few elephants were on screen.
- No 16-bit `%`, `/` or non-power-of-two array element sizes in per-frame code (they call slow library routines).
  `Entity`, `Dart` and `SprDef` are padded to 16/8/8 bytes for that reason.
- Sound periods and note lengths are computed at compile time.

## Art in `gfx/`

Sprite sheets are vertical strips of frames (frame size is in the file name, e.g. `lion_24x8.png` = 6 frames of 24x8).
Sprites: transparent = see-through, white = mask/fill, black = ink. Fences, `creators_160x8.png` and the font are
black on white (transparent counts as white). `NewNeoRetroWithURL-160x144.png` (the intro, full screen, at most 115 different 8x8 tiles) is shown white on black. `font_8x8x1.png` is
64 glyphs, ASCII 32-95. Keep the sizes: the frame sizes are listed in `tools/convert_assets.ps1`, and a new sprite
needs an entry there and in `src/main.c`.

## Menu, scores, game over

- **Title:** `leanna_the_lion_title_screen_160x144.png` with three 32x16 buttons from `Menu-32x32.png`
  (frames, top to bottom: sound on, sound off, PLAY, SCORES). LEFT/RIGHT moves the cursor
  (`cursor_20x12.png`, a sprite that bobs 3 px), A selects, START always plays. The sound button shows its
  on or off picture and A toggles it. The button positions are set at the top of `tools/convert_title.ps1`.
- **Packing:** a full screen of detailed art has more different 8x8 tiles than a background map can address, so
  the converter (`tools/TitleTiles.cs`) packs it into all 384 tile slots and the screen switches the tile data
  addressing half way down. If a changed picture needs more than fits, the closest pairs of tiles are merged
  (a few pixels change) and the build prints how many; it stops with an error if that is not enough.
- **Starting a game:** A on PLAY blacks the screen out (like the splash to title), the game view appears with the
  fences scrolling and no lion, the story is typed over it (30 characters a second, A skips ahead), disappears,
  and the lion runs in from the left edge.
- **Top scores:** the best 8 scores with 3 letter initials are saved in cart RAM (an old 5 entry table is still read). `FullScreenPanel_160x144.png` with `TopScoresBanner.png` in its top white section, text in its black area (white font), and `PressAButton-52x16.png` flashing in the lower area.
  is the frame; its black area is written with a white on black copy of the font.
- **Scoreboard:** `scoreboard_37x9.png` sits at the top centre of the game screen and the score is drawn into it
  pixel by pixel (6 digits, right aligned); there is no "SCORE" text. While the 2x multiplier runs, "2X" shows beside it.
- **Game over:** `FullScreenPanel_160x144.png` (shared with the top scores) with `GameOverBanner.png` in its top white section. The cause, the quip and the scores go in its black area
  (white text, at most 18 characters a line); without a new top score the flashing `PressAButton-52x16.png` shows in the lower area. A score that makes the top 8 shows `NewTopScore_widget_112x19.png`
  in the lower area with the initials drawn over its underscores: UP/DOWN changes the letter, LEFT/RIGHT (or B) moves
  between the letters, A on the last letter confirms. Then the top scores screen shows the new entry blinking.
- All tile data lives in ROM bank 1, which is about 89% full: a third bank will be needed for more large pictures.
