/*
 * Leanna the Lion - Game Boy (DMG) port
 *
 * Ported from the Arduboy version (arduboy/LEANNA_THE_LION_AB.ino). The gameplay
 * numbers (speeds, odds table, hit boxes, timers) are copied from it; this file
 * only replaces the platform layer.
 *
 * Playfield: the whole 160x144 screen. Compared to the Arduboy layout the lanes are
 * twice as far apart (LANE_SPACING 24 instead of 12), the lion changes lanes twice as
 * fast to compensate, and the two top fences are far enough apart for the truck to
 * drive between them. Entities enter at the right edge of the screen (SPAWN_X).
 *
 * Screen lines:
 *    0..15   HUD
 *   16..31   background fence
 *   26..57   truck lane (truck is 32px tall, bottom at LANE0_BOTTOM)
 *   58..65   foreground rail
 *   79/103/127  bottoms of lanes 1..3 (lion rows are 8px above these)
 *  128..135  foreground rail
 *
 * Everything below the background fence is 8px higher than in the first version.
 */

#include <gb/gb.h>
#include <gb/hardware.h>
#include <rand.h>
#include <stdint.h>

#include "gfx_data.h"
#include "title_data.h"

/* ---- layout ------------------------------------------------------------ */

#define SCREEN_W   160
#define SCREEN_H   144
#define SPAWN_X    160      /* original: 128. Entities spawn at the screen edge */
#define TRUCK_SPAWN_X (SPAWN_X + 64)   /* trucks start further off screen */
#define OAM_COUNT  40

#define LANE_SPACING 24     /* original: 12 */
#define LANE0_BOTTOM 58     /* truck lane, between the two top fences */
#define LANE1_BOTTOM 79     /* bottom of the first lane the lion can run in */
#define ROW_MIN      (LANE1_BOTTOM - 8)               /* lion y for lane 1 (lion is 8px tall) */
#define ROW_MAX      (ROW_MIN + 2 * LANE_SPACING)
#define ROW_START    (ROW_MIN + LANE_SPACING)
#define LANE_CHANGE_SPEED 2 /* px per frame, original: 1 */

#define BG_FENCE_Y   16     /* background fence, 16px tall */
#define FG_RAIL1_Y   58
#define FG_RAIL2_Y   128

/* BG map rows holding the scrolling fence strips (never visible without the LCD effect) */
#define MAP_BGFENCE_ROW 24  /* two rows. Far down the map; the row after the foreground strip must stay blank, because */
#define MAP_FGFENCE_ROW 26  /* the bottom rail band is 16 lines tall and shows it plus the row below */

#define VERSION_TEXT "DMG V0.2"

/* ---- game constants (from the Arduboy source) --------------------------- */

#define STATE_TITLE   0
#define STATE_SCORES  1   /* top scores */
#define STATE_PLAYING 2
#define STATE_OVER    3
#define STATE_INTRO   4
#define STATE_INITIALS 5  /* entering the initials of a new top 5 score */
#define STATE_STORY   6   /* story typed over the game view */
#ifndef BOT_SELECT
#define BOT_SELECT 0
#endif
#ifndef BOT_SCORES_HOLD
#define BOT_SCORES_HOLD 0
#endif
#ifndef BOT_TITLE_DELAY
#define BOT_TITLE_DELAY 20
#endif
#ifndef INTRO_FRAMES
#define INTRO_FRAMES  90
#endif

#define ELDER_ENTITY            1
#define CHILD_ENTITY            2
#define ADULT_ENTITY            3
#define MONKEY_ENTITY           4
#define ELEPHANT_ENTITY         5
#define TRANQ_ZOO_KEEPER_ENTITY 6
#define TRANQ_TRUCK_ENTITY      7
#define CAGE_ENTITY             8
#define WATER_PUDDLE_ENTITY     9
#define HOT_SAUCE_ENTITY        10

#define TRANQ_DART        1
#define ANGLED_TRANQ_DART 2
#define MONKEY_POO        3

#define TOTAL_ENTITIES 4
#define TOTAL_DARTS    4
#define MAX_LEVEL      6

#define TRAMPLED_TEXT    1
#define KNOCKED_OUT_TEXT 2
#define CAGED_TEXT       3

/* odds per entity per 15 sec chunk: count_down_till, entity1..entity5 (4 lanes x 7 levels) */
static const uint8_t odds[] = {
  /* 0-15 secs */
  120, 0, 0, 0, 0, 0,
  120, ELDER_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, CAGE_ENTITY,
  120, ELDER_ENTITY, ELDER_ENTITY, CHILD_ENTITY, ADULT_ENTITY, CHILD_ENTITY,
  120, ELDER_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, CAGE_ENTITY,
  /* 16-30 secs */
  120, 0, 0, 0, 0, 0,
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, CAGE_ENTITY,
  120, WATER_PUDDLE_ENTITY, CHILD_ENTITY, CHILD_ENTITY, ADULT_ENTITY, ELEPHANT_ENTITY,
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, CAGE_ENTITY,
  /* 31-45 secs */
  120, TRANQ_TRUCK_ENTITY, 0, 0, 0, 0,
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY,
  120, WATER_PUDDLE_ENTITY, CHILD_ENTITY, CHILD_ENTITY, ADULT_ENTITY, CAGE_ENTITY,
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY,
  /* 46-60 secs */
  120, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, 0, 0, 0,
  120, MONKEY_ENTITY, ELDER_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY,
  120, WATER_PUDDLE_ENTITY, CHILD_ENTITY, CHILD_ENTITY, ADULT_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,
  120, MONKEY_ENTITY, ELDER_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY,
  /* 61-75 secs */
  120, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, 0, 0,
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,
  120, MONKEY_ENTITY, CHILD_ENTITY, CHILD_ENTITY, ADULT_ENTITY, ELEPHANT_ENTITY,
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,
  /* 76-90 secs */
  120, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, 0,
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,
  120, MONKEY_ENTITY, CHILD_ENTITY, ADULT_ENTITY, ELEPHANT_ENTITY, ELEPHANT_ENTITY,
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,
  /* 91-105 secs */
  120, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY,
  120, MONKEY_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,
  120, WATER_PUDDLE_ENTITY, ADULT_ENTITY, ADULT_ENTITY, CAGE_ENTITY, CAGE_ENTITY,
  120, MONKEY_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,
};

/* per entity type (index = type): sprite, speed (1/8 px per frame), animate every N frames,
 * frame count, score for eating it, width in px */
static const uint8_t type_sprite[]  = { 0, SPR_ELDER, SPR_CHILD, SPR_ADULT, SPR_MONKEY, SPR_ELEPHANT,
                                        SPR_ZOOKEEPER, SPR_TRUCK, SPR_CAGE, SPR_PUDDLE, SPR_HOTSAUCE };
static const uint8_t type_speed[]   = { 0, 12, 8, 6, 7, 17, 16, 4, 16, 16, 16 };
static const uint8_t type_animrate[]= { 0, 15, 6, 4, 4, 15, 0, 0, 0, 0, 0 };
static const uint8_t type_frames[]  = { 1, 2, 4, 4, 4, 2, 1, 1, 1, 1, 1 };
static const uint8_t type_score[]   = { 0, 1, 2, 4, 8, 0, 16, 0, 0, 0, 0 };
static const uint8_t type_width[]   = { 0, 8, 8, 8, 8, 32, 16, 32, 32, 16, 8 };
static const uint8_t type_height[]  = { 0, 16, 16, 16, 8, 32, 16, 32, 16, 8, 8 };
/* hit box relative to the sprite: x, y, w, h */
static const int8_t  type_hit[][4]  = {
  { 0, 0, 0, 0 },
  { 0, 8, 8, 8 },    /* elder */
  { 0, 8, 8, 8 },    /* child */
  { 0, 8, 8, 8 },    /* adult */
  { 0, 0, 8, 8 },    /* monkey */
  { 8, 24, 16, 8 },  /* elephant */
  { 0, 8, 8, 8 },    /* zoo keeper */
  { 0, 0, 0, 0 },    /* truck: no hit box */
  { 0, 8, 12, 8 },   /* cage */
  { 0, 0, 16, 8 },   /* puddle */
  { 0, 0, 8, 8 },    /* hot sauce */
};

/* ---- state ---------------------------------------------------------------- */

typedef struct {
  int16_t  x;            /* 1/8 px */
  int16_t  y;            /* px, playfield space */
  uint8_t  exists;
  uint8_t  type;
  uint8_t  frame;
  uint8_t  row;
  int8_t   creation_timer;
  int8_t   darts;
  uint8_t  slot;         /* first OAM slot, NO_SLOT if none */
  uint8_t  pad[5];       /* 16 bytes: array indexing becomes a shift instead of a multiply call */
} Entity;

typedef struct {
  int16_t x;             /* px */
  int16_t y;
  uint8_t exists;
  uint8_t type;
  uint8_t slot;
  uint8_t pad[1];        /* 8 bytes, see Entity */
} Dart;

typedef struct {
  int16_t  x;
  int16_t  y;
  int16_t  row;
  uint8_t  frame;
  uint8_t  sliding;
  uint8_t  slide_timer;
  uint8_t  jump;
  int16_t  multiplier_timer;
  uint32_t score;
} Player;

static Entity entities[TOTAL_ENTITIES];
static Dart   darts[TOTAL_DARTS];
static Player player;

static uint8_t  game_state;
static uint16_t frame_count;
static uint16_t state_timer;
static uint8_t  level;
static uint16_t level_frames;
static uint16_t hotsauce_timer;
static uint8_t  gameover_type;
static uint8_t  gameover_text_id;
static uint8_t  hud_dirty;

static uint8_t keys, prev_keys, pressed;

/* everyXFrames(n) without the 16-bit modulo (slow on the Game Boy CPU) */
static uint8_t anim_cnt[16], anim_hit[16];

static void anim_update(void) {
  static const uint8_t rates[4] = { 4, 5, 6, 15 };
  uint8_t i, r;
  for (i = 0; i < 4; i++) {
    r = rates[i];
    if (++anim_cnt[r] >= r) { anim_cnt[r] = 0; anim_hit[r] = 1; }
    else                    { anim_hit[r] = 0; }
  }
}

/* ---- sprites (OAM) ---------------------------------------------------------- */

/* Each moving object owns a fixed run of OAM slots for its lifetime. The tiles and y are
 * written once; per frame only x (and y for the lion and darts) changes. SDCC is far too
 * slow to rebuild every sprite every frame (a redraw of the full OAM overran the frame).
 *
 * Lower slots draw on top, so the lion gets the first slots. OAM x/y are screen + 8/16 and
 * wrap as 8 bit values: positions just off the left or top edge land in the hidden range
 * (0 or >= 168 / 160), so nothing needs clipping. */
#define NO_SLOT 0xFF
static uint8_t oam_used[OAM_COUNT];

static uint8_t oam_alloc(uint8_t n) {
  uint8_t s, i;
  for (s = 0; s + n <= OAM_COUNT; s++) {
    for (i = 0; i < n && !oam_used[s + i]; i++) ;
    if (i == n) {
      for (i = 0; i < n; i++) oam_used[s + i] = 1;
      return s;
    }
  }
  return NO_SLOT;
}

static void oam_release(uint8_t s, uint8_t n) {
  if (s == NO_SLOT) return;
  while (n--) {
    oam_used[s] = 0;
    shadow_OAM[s].y = 0;
    s++;
  }
}

static void oam_reset(void) {
  uint8_t i;
  for (i = 0; i < OAM_COUNT; i++) { oam_used[i] = 0; shadow_OAM[i].y = 0; }
}

static uint8_t lion_slot = NO_SLOT, sight_slot = NO_SLOT, lion_shown_frame = 0xFF;

static uint8_t spr_pieces(uint8_t spr) { return sprdefs[spr].stride >> 1; }

static void spr_set_tiles(uint8_t slot, uint8_t spr, uint8_t frame) {
  const SprDef *d = &sprdefs[spr];
  uint8_t tile = d->tile, n = d->stride >> 1;
  OAM_item_t *o = &shadow_OAM[slot];
  while (frame--) tile += d->stride;
  while (n--) { o->tile = tile; tile += 2; o++; }
}

static void spr_x(uint8_t slot, uint8_t spr, uint8_t x0) {
  const SprDef *d = &sprdefs[spr];
  uint8_t r, c, x, cols = d->cols, rows = d->rows;
  OAM_item_t *o = &shadow_OAM[slot];
  for (r = 0; r < rows; r++) {
    x = x0;
    for (c = 0; c < cols; c++, x += 8, o++) o->x = x;
  }
}

static void spr_y(uint8_t slot, uint8_t spr, uint8_t y0) {
  const SprDef *d = &sprdefs[spr];
  uint8_t r, c, cols = d->cols, rows = d->rows;
  OAM_item_t *o = &shadow_OAM[slot];
  for (r = 0; r < rows; r++, y0 += 16) {
    for (c = 0; c < cols; c++, o++) o->y = y0;
  }
}

/* ---- text (BG) -------------------------------------------------------------- */

static void text(uint8_t col, uint8_t row, const char *s) {
  uint8_t buf[20];
  uint8_t n = 0;
  char ch;
  while ((ch = *s++) != 0 && n < 20) {
    if (ch >= 'a' && ch <= 'z') ch -= 32;
    buf[n++] = (ch < 32 || ch > 95) ? BG_FONT : (uint8_t)(BG_FONT + ch - 32);
  }
  if (n) set_bkg_tiles(col, row, n, 1, buf);
}

static void text_center(uint8_t row, const char *s) {
  uint8_t n = 0;
  while (s[n]) n++;
  text((20 - n) >> 1, row, s);
}

static uint8_t u32_str(uint32_t v, char *out) {
  char tmp[11];
  uint8_t n = 0, i;
  if (v < 65536ul) {                 /* 32-bit division is far too slow for the common case */
    static const uint16_t pow10[4] = { 10000, 1000, 100, 10 };
    uint16_t w = (uint16_t)v;
    uint8_t started = 0, d;
    for (i = 0; i < 4; i++) {
      d = 0;
      while (w >= pow10[i]) { w -= pow10[i]; d++; }
      if (d || started) { out[n++] = 0x30 + d; started = 1; }
    }
    out[n++] = 0x30 + (uint8_t)w;
    out[n] = 0;
    return n;
  }
  do {
    tmp[n++] = '0' + (uint8_t)(v % 10);
    v /= 10;
  } while (v);
  for (i = 0; i < n; i++) out[i] = tmp[n - 1 - i];
  out[n] = 0;
  return n;
}

static void number(uint8_t col, uint8_t row, uint32_t v) {
  char buf[11];
  u32_str(v, buf);
  text(col, row, buf);
}

static void clear_screen(void) {
  fill_bkg_rect(0, 0, 20, 18, BG_FONT);   /* glyph 0 is the blank space */
}

/* ---- sound ------------------------------------------------------------------ */

/* Notes are stored as channel 1 period values and lengths in frames, worked out at compile
 * time: the ms and Hz conversions need 16/32-bit divisions that stall a frame on this CPU.
 * Same notes as the Arduboy tone tables (Hz, ms). */
#define NOTE(hz)  ((uint16_t)(2048 - (131072L + (hz) / 2) / (hz)))
#define MS(ms)    ((uint8_t)((ms) * 3 / 50))     /* ms -> 60Hz frames */
#define REST      0

typedef struct { uint16_t period; uint8_t frames; } Tone;   /* period REST = silence, frames 0 = end */

static const Tone snd_intro[] = {
  {NOTE(200),MS(100)},{REST,MS(100)},{NOTE(200),MS(100)},{REST,MS(100)},
  {NOTE(600),MS(100)},{REST,MS(100)},{NOTE(600),MS(100)},{REST,MS(100)},
  {NOTE(400),MS(100)},{REST,MS(100)},{NOTE(300),MS(100)},{REST,MS(100)},
  {NOTE(400),MS(100)},{REST,MS(100)},{0,0}
};
static const Tone snd_eat[]       = { {NOTE(320),MS(200)}, {0,0} };
static const Tone snd_game_over[] = { {NOTE(600),MS(400)}, {REST,MS(100)}, {NOTE(350),MS(100)}, {0,0} };

static const Tone *snd_cur;
static uint8_t snd_left;
static uint8_t sound_on = 1;

static void snd_next(void) {
  uint16_t period;
  if (snd_cur->frames == 0) { snd_cur = 0; NR12_REG = 0x00; return; }
  period = snd_cur->period;
  if (period == REST) {
    NR12_REG = 0x00;
  } else {
    NR10_REG = 0x00;
    NR11_REG = 0x80;         /* 50% duty */
    NR12_REG = 0xF0;         /* full volume, no envelope */
    NR13_REG = (uint8_t)period;
    NR14_REG = 0x80 | (uint8_t)(period >> 8);
  }
  snd_left = snd_cur->frames;
  snd_cur++;
}

static void snd_play(const Tone *t) {
  if (!sound_on) return;
  snd_cur = t;
  snd_next();
}

static void snd_stop(void) { snd_cur = 0; NR12_REG = 0x00; }

static void snd_update(void) {
  if (!snd_cur) return;
  if (snd_left) snd_left--;
  if (snd_left == 0) snd_next();
}

/* ---- save (cart SRAM) ---------------------------------------------------------
 *
 *   0..5   'L' 'N', unused, unused, unused, sound (1 = off, 2 = on)
 *   8..    'T' 'S', then 5 entries of 3 initials + score (16 bit), then a checksum
 */

#define SRAM ((volatile uint8_t *)0xA000)
#define SRAM_SCORES 8

#define NUM_SCORES 8
static char score_ini[NUM_SCORES][3];
static uint16_t score_val[NUM_SCORES];       /* best first. 0 = empty */

static void scores_default(void) {
  uint8_t i, k;
  for (i = 0; i < NUM_SCORES; i++) {
    for (k = 0; k < 3; k++) score_ini[i][k] = 'A';
    score_val[i] = 0;
  }
}

static void settings_load(void) {
  uint8_t i, k, sum, b, count = 0;
  uint16_t o = SRAM_SCORES + 2;
  scores_default();
  ENABLE_RAM;
  if (SRAM[0] == 'L' && SRAM[1] == 'N') {
    if (SRAM[5] == 1) sound_on = 0;       /* 1 = off, 2 = on, anything else = default (on) */
  }
  if (SRAM[SRAM_SCORES] == 'T' && SRAM[SRAM_SCORES + 1] == '8') count = 8;
  else if (SRAM[SRAM_SCORES] == 'T' && SRAM[SRAM_SCORES + 1] == 'S') count = 5;   /* the first version kept 5 */
  if (count) {
    sum = 0xA5;
    for (i = 0; i < count * 5; i++) sum ^= SRAM[o + i];
    if (SRAM[o + count * 5] == sum) {
      for (i = 0; i < count; i++) {
        for (k = 0; k < 3; k++) {
          b = SRAM[o++];
          score_ini[i][k] = (b >= 'A' && b <= 'Z') ? b : 'A';
        }
        score_val[i] = SRAM[o] | ((uint16_t)SRAM[o + 1] << 8);
        o += 2;
      }
    }
  }
  DISABLE_RAM;
}

static void save_settings(void) {
  ENABLE_RAM;
  SRAM[0] = 'L'; SRAM[1] = 'N';
  SRAM[5] = sound_on ? 2 : 1;
  DISABLE_RAM;
}

static void scores_save(void) {
  uint8_t i, k, sum = 0xA5, b;
  uint16_t o = SRAM_SCORES + 2;
  ENABLE_RAM;
  SRAM[SRAM_SCORES] = 'T'; SRAM[SRAM_SCORES + 1] = '8';
  for (i = 0; i < NUM_SCORES; i++) {
    for (k = 0; k < 3; k++) { b = score_ini[i][k]; sum ^= b; SRAM[o++] = b; }
    b = (uint8_t)score_val[i]; sum ^= b; SRAM[o++] = b;
    b = (uint8_t)(score_val[i] >> 8); sum ^= b; SRAM[o++] = b;
  }
  SRAM[o] = sum;
  DISABLE_RAM;
}

/* where a score would land in the top 5: 0..4, or NUM_SCORES if it does not make it */
static uint8_t score_rank(uint32_t s) {
  uint8_t i;
  if (s == 0) return NUM_SCORES;
  for (i = 0; i < NUM_SCORES; i++) if (s > score_val[i]) return i;
  return NUM_SCORES;
}

static void score_insert(uint8_t rank, const char *ini, uint32_t s) {
  uint8_t i, k;
  for (i = NUM_SCORES - 1; i > rank; i--) {
    for (k = 0; k < 3; k++) score_ini[i][k] = score_ini[i - 1][k];
    score_val[i] = score_val[i - 1];
  }
  for (k = 0; k < 3; k++) score_ini[rank][k] = ini[k];
  score_val[rank] = s > 65535ul ? 65535 : (uint16_t)s;
}

/* ---- scrolling fence (LCD scanline effect) ---------------------------------------
 *
 * The two fences scroll at different speeds, so the screen is split in bands with their
 * own SCX/SCY. The fence strips live far down the BG map; SCY brings the right strip to
 * the top of each band. Band changes happen in HBlank of the line before the band.
 */

#define FX_BANDS 5
/* band starts: background fence, blank, rail 1, blank, rail 2. Lines above the first band are the HUD. */
static const uint8_t fx_line[FX_BANDS] = { BG_FENCE_Y, BG_FENCE_Y + 16, FG_RAIL1_Y, FG_RAIL1_Y + 8, FG_RAIL2_Y };
static uint8_t fx_scy[FX_BANDS]  = {     /* the blank bands (1, 3) are changed by the story slide */
  (uint8_t)(MAP_BGFENCE_ROW * 8 - BG_FENCE_Y),
  0,
  (uint8_t)(MAP_FGFENCE_ROW * 8 - FG_RAIL1_Y),
  0,
  (uint8_t)(MAP_FGFENCE_ROW * 8 - FG_RAIL2_Y)
};
static uint8_t fx_hud_scy;         /* SCY of the lines above the first band */
static uint8_t fx_scx[FX_BANDS];   /* 0 for the static bands, scroll offsets for the fences */
static volatile uint8_t fx_idx;
static uint8_t bg_fence_x, fg_fence_x;

#define FXM_OFF   0
#define FXM_GAME  1   /* fence bands */
#define FXM_TITLE 2   /* tile data addressing switches half way down the screen */
static volatile uint8_t fx_mode;

static void fx_lcd_isr(void) {
  uint8_t i = fx_idx;
  while (STAT_REG & STATF_BUSY) ;      /* wait for HBlank */
  if (fx_mode == FXM_TITLE) {
    LCDC_REG &= ~LCDCF_BG8000;         /* bottom of the title: 0x8800 addressing */
    return;
  }
  SCX_REG = fx_scx[i];
  SCY_REG = fx_scy[i];
  i++;
  if (i < FX_BANDS) LYC_REG = fx_line[i] - 1;
  fx_idx = i;
}

#ifdef DEBUG_PERF
static volatile uint16_t vbl_frames;
#endif

static void fx_vbl_isr(void) {
#ifdef DEBUG_PERF
  vbl_frames++;
#endif
  if (fx_mode == FXM_TITLE) {
    LCDC_REG |= LCDCF_BG8000;          /* top of the title: 0x8000 addressing */
    SCX_REG = 0;
    SCY_REG = 0;
    LYC_REG = TITLE_SPLIT_LINE - 1;
    return;
  }
  LCDC_REG &= ~LCDCF_BG8000;           /* everything else uses 0x8800 addressing */
  SCX_REG = 0;
  SCY_REG = fx_hud_scy;
  fx_idx = 0;
  LYC_REG = fx_line[0] - 1;
}

static void fx_set_mode(uint8_t m) {
#ifdef DEBUG_NOFX
  if (m == FXM_GAME) m = FXM_OFF;
#endif
  fx_mode = m;
  if (m == FXM_OFF) {
    STAT_REG &= ~STATF_LYC;
    SCX_REG = 0;
    SCY_REG = 0;
    LCDC_REG &= ~LCDCF_BG8000;
  } else {
    fx_idx = 0;
    LYC_REG = (m == FXM_TITLE ? TITLE_SPLIT_LINE : fx_line[0]) - 1;
    STAT_REG |= STATF_LYC;
  }
}

static void fx_set(uint8_t on) { fx_set_mode(on ? FXM_GAME : FXM_OFF); }

/* ---- gameplay ----------------------------------------------------------------- */

static uint8_t overlap(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
                       int16_t bx, int16_t by, int16_t bw, int16_t bh) {
  return !(bx > ax + aw || bx + bw < ax || by > ay + ah || by + bh < ay);
}

/* Sight line tests. The Arduboy uses a float line/rect routine; the two lines in the
 * game are fixed shapes, so these are division free (a divide costs ~1000 cycles here). */

/* zoo keeper: horizontal line at y from x=0 to x=len, against the rect */
static uint8_t hline_hits(int16_t len, int16_t y, int16_t rx, int16_t ry, int16_t rh) {
  return len > rx && y > ry && y < ry + rh;
}

/* truck: 45 degree line of SIGHT_LEN px from (fx,fy) running down-left, against the rect */
#define SIGHT_LEN 96
static uint8_t diag_hits(int16_t fx, int16_t fy, int16_t rx, int16_t ry, int16_t rw, int16_t rh) {
  int16_t y, x;
  if (fx <= rx || fx - SIGHT_LEN >= rx + rw || fy >= ry + rh || fy + SIGHT_LEN <= ry) return 0;
  y = fy + (fx - rx);                        /* at the left edge */
  if (y > ry && y < ry + rh) return 1;
  y = fy + (fx - rx - rw);                   /* at the right edge */
  if (y > ry && y < ry + rh) return 1;
  x = fx - (ry - fy);                        /* at the top edge */
  if (x > rx && x < rx + rw) return 1;
  x = fx - (ry + rh - fy);                   /* at the bottom edge */
  if (x > rx && x < rx + rw) return 1;
  return 0;
}

static uint8_t dart_spr(uint8_t type) {
  return type == TRANQ_DART ? SPR_DART : type == ANGLED_TRANQ_DART ? SPR_ANGLEDDART : SPR_MONKEYPOO;
}

static void create_dart(uint8_t type, int16_t x, int16_t y) {
  uint8_t i, spr;
  Dart *d;
  for (i = 0; i < TOTAL_DARTS; i++) {
    d = &darts[i];
    if (!d->exists) {
      d->x = x;
      d->y = y;
      d->exists = 1;
      d->type = type;
      spr = dart_spr(type);
      d->slot = oam_alloc(spr_pieces(spr));
      if (d->slot != NO_SLOT) {
        spr_set_tiles(d->slot, spr, 0);
        spr_y(d->slot, spr, (uint8_t)(y + 16));
        spr_x(d->slot, spr, (uint8_t)(x + 8));
      }
      break;
    }
  }
}

static void entity_remove(Entity *e) {
  oam_release(e->slot, spr_pieces(type_sprite[e->type]));
  e->slot = NO_SLOT;
  e->exists = 0;
}

static void create_entity(uint8_t type, uint8_t id) {
  Entity *e = &entities[id];
  int16_t h;
  uint8_t spr;
  if (e->exists) return;
  h = type_height[type];
  e->y = (e->row == 0) ? (LANE0_BOTTOM - h) : (LANE1_BOTTOM + (e->row - 1) * LANE_SPACING - h);
  e->x = (int16_t)(type == TRANQ_TRUCK_ENTITY ? TRUCK_SPAWN_X : SPAWN_X) << 3;
  e->exists = 1;
  e->frame = 0;
  e->type = type;
  e->creation_timer = odds[id * 6 + level * 24];
  e->darts = 1;
  e->slot = NO_SLOT;
  if (type == 0) {
    e->exists = 0;
    return;
  }
  spr = type_sprite[type];
  e->slot = oam_alloc(spr_pieces(spr));
  if (e->slot != NO_SLOT) {
    spr_set_tiles(e->slot, spr, 0);
    spr_y(e->slot, spr, (uint8_t)(e->y + 16));
    spr_x(e->slot, spr, 200);                  /* hidden: off the right edge */
  }
}

static void game_over(uint8_t why) {
#ifdef DEBUG_NODEATH
  return;
#endif
  snd_play(snd_game_over);
  gameover_type = why;
  gameover_text_id = rand() & 3;
  game_state = STATE_OVER;
  state_timer = 0;
}

/* uniform 0..4 without a division */
static uint8_t rand_0_4(void) {
  uint8_t r;
  do { r = rand() & 7; } while (r > 4);
  return r;
}

static void tick_entity(uint8_t i) {
  Entity *e = &entities[i];
  uint8_t t;
  int16_t px;

  if (e->creation_timer > 0) e->creation_timer--;

  if (!e->exists) {
    if (e->creation_timer == 0) {
      if (hotsauce_timer == 0 && i != 0) {
        hotsauce_timer = 3600;
        create_entity(HOT_SAUCE_ENTITY, i);
      } else {
#ifdef DEBUG_FORCE_TYPE
        create_entity(((DEBUG_FORCE_TYPE == TRANQ_TRUCK_ENTITY) == (i == 0)) ? DEBUG_FORCE_TYPE : 0, i);
#else
        create_entity(odds[i * 6 + level * 24 + rand_0_4() + 1], i);
#endif
      }
    }
    return;
  }

  t = e->type;
  px = e->x >> 3;

  if (t == MONKEY_ENTITY) {
    /* if the player is right behind me then spray poo */
    if (px - 24 <= player.x + 20 && overlap(px - 24, e->y + 1, 16, 6, player.x + 4, player.y + 1, 16, 6))
      create_dart(MONKEY_POO, px - 16, e->y);
  } else if (t == TRANQ_TRUCK_ENTITY) {
    /* if the player is in the line of sight, shoot a tranq dart */
    if (e->darts > 0 && px < player.x + 108 &&
        diag_hits(px + 12, e->y + 21, player.x + 4, player.y + 1, 16, 6)) {
      e->darts--;
      create_dart(ANGLED_TRANQ_DART, px + 1, e->y + 21);
    }
  } else if (t == TRANQ_ZOO_KEEPER_ENTITY) {
    /* if the player is on the same row he shoots his single dart */
    if (e->darts > 0 &&
        hline_hits(px, e->y + 9, player.x + 4, player.y, 8)) {
      e->darts--;
      create_dart(TRANQ_DART, px, e->y + 6);
    }
  }

  e->x -= type_speed[t];
  if (e->x < -((int16_t)type_width[t] << 3)) {
    entity_remove(e);
    return;
  }
  if (e->slot == NO_SLOT) return;
  if (type_animrate[t] && anim_hit[type_animrate[t]]) {
    e->frame++;
    if (e->frame >= type_frames[t]) e->frame = 0;
    spr_set_tiles(e->slot, type_sprite[t], e->frame);
  }
  px = (e->x >> 3) + 8;
  spr_x(e->slot, type_sprite[t], px > 200 ? 200 : (uint8_t)px);   /* stay clear of the 8 bit wrap-around */
}

static void tick_darts(void) {
  uint8_t i;
  for (i = 0; i < TOTAL_DARTS; i++) {
    Dart *d = &darts[i];
    if (!d->exists) continue;
    if (d->type == TRANQ_DART)             d->x -= 4;
    else if (d->type == ANGLED_TRANQ_DART) { d->x -= 2; d->y += 2; }
    else                                   d->x -= 16;
    if (d->x < -8 || d->y > SCREEN_H) {
      oam_release(d->slot, spr_pieces(dart_spr(d->type)));
      d->slot = NO_SLOT;
      d->exists = 0;
    } else if (d->slot != NO_SLOT) {
      spr_x(d->slot, dart_spr(d->type), (uint8_t)(d->x + 8));
      if (d->type == ANGLED_TRANQ_DART) spr_y(d->slot, SPR_ANGLEDDART, (uint8_t)(d->y + 16));
    }
  }
}

static void entity_collisions(void) {
  uint8_t i, t;
  int16_t px;
  Entity *e;
  for (i = 0; i < TOTAL_ENTITIES; i++) {
    e = &entities[i];
    if (!e->exists) continue;
    t = e->type;
    if (t == TRANQ_TRUCK_ENTITY) continue;
    /* cheap reject: hit boxes start at the sprite origin and are at most 32px wide and tall */
    px = e->x >> 3;
    if (px > player.x + 20 || px + 32 < player.x + 4 || e->y > player.y + 7 || e->y + 32 < player.y + 1) continue;
    if (!overlap(px + type_hit[t][0], e->y + type_hit[t][1], type_hit[t][2], type_hit[t][3],
                 player.x + 4, player.y + 1, 16, 6)) continue;

    if (type_score[t]) {                       /* edible: elder, child, adult, monkey, zoo keeper */
      if (!player.sliding) {
        entity_remove(e);
        snd_play(snd_eat);
        player.score += player.multiplier_timer > 0 ? type_score[t] * 2 : type_score[t];
        hud_dirty = 1;
        e->creation_timer = 0;
      }
    } else if (t == WATER_PUDDLE_ENTITY) {
      if (player.jump != 1) { player.slide_timer = 45; player.sliding = 1; }
    } else if (t == CAGE_ENTITY) {
      game_over(CAGED_TEXT);
      return;
    } else if (t == ELEPHANT_ENTITY) {
      game_over(TRAMPLED_TEXT);
      return;
    } else if (t == HOT_SAUCE_ENTITY) {
      snd_play(snd_eat);
      player.multiplier_timer = 900;
      entity_remove(e);
    }
  }
}

static void dart_collisions(void) {
  uint8_t i;
  for (i = 0; i < TOTAL_DARTS; i++) {
    Dart *d = &darts[i];
    if (!d->exists || d->x > player.x + 20 || d->x + 8 < player.x + 4) continue;
    /* the Arduboy build shadows the 16x8 poo hit box with the 6x6 dart one, so every
     * projectile uses the small box. Kept as shipped. */
    if (!overlap(d->x + 1, d->y + 1, 6, 6, player.x + 4, player.y + 1, 16, 6)) continue;
    if (d->type == MONKEY_POO) {
      player.slide_timer = 45;
      player.sliding = 1;
    } else {
      game_over(KNOCKED_OUT_TEXT);
      return;
    }
  }
}

static void update_player(void) {
  if (player.sliding) {
    if (player.slide_timer > 0) player.slide_timer--;
    if (player.slide_timer == 0) player.sliding = 0;
    player.frame = 4;
    return;
  }

  if ((pressed & J_UP) && player.jump != 1) {
    player.row -= LANE_SPACING;
    if (player.row < ROW_MIN) player.row = ROW_MIN;
  }
  if ((pressed & J_DOWN) && player.jump != 1) {
    player.row += LANE_SPACING;
    if (player.row > ROW_MAX) player.row = ROW_MAX;
  }
  if ((pressed & (J_A | J_B)) && player.jump == 0) player.jump = 1;

  if (player.jump == 1) {
    if (player.x < 32) player.x += 2;
    if (player.x == 32) player.jump = 2;
    player.frame = 5;
  } else if (anim_hit[5]) {
    player.frame++;
    if (player.frame > 3) player.frame = 0;
  } else if (player.frame > 3) {
    player.frame = 0;
  }
  if (player.jump == 2 && player.x != 0) {
    player.x--;
    if (player.x == 0) player.jump = 0;
  }
  if (player.y < player.row) player.y += LANE_CHANGE_SPEED;
  else if (player.y > player.row) player.y -= LANE_CHANGE_SPEED;
}

/* ---- pixel text -------------------------------------------------------------------
 *
 * The scoreboard and the "new top score" widget are small pictures that text is drawn into pixel by
 * pixel: the picture is kept as a 1 bit per pixel canvas, glyphs of the 8x8 font are blitted onto it
 * (5x7 pixels each) and the canvas is copied into BG tiles.
 */

static void blit_glyph(uint8_t *buf, uint8_t stride, uint8_t x, uint8_t y, char ch) {
  const uint8_t *gp;
  uint8_t r, g, sft;
  uint8_t *p;
  if (ch >= 'a' && ch <= 'z') ch -= 32;
  if (ch < 32 || ch > 95) return;
  gp = &bg_tiles[(uint16_t)(ch - 32) << 4];
  sft = x & 7;
  p = &buf[(uint16_t)y * stride + (x >> 3)];
  for (r = 0; r < 7; r++, p += stride) {          /* no multiplication per row: SDCC's is a slow library call */
    g = gp[r << 1];
    p[0] |= g >> sft;
    if (sft) p[1] |= g << (8 - sft);
  }
}

static void cell_write(uint8_t idx, const uint8_t *buf, uint8_t stride, uint8_t cx, uint8_t cy) {
  uint8_t out[16], y;
  const uint8_t *p = &buf[((uint16_t)cy * stride << 3) + cx];
  for (y = 0; y < 8; y++, p += stride) out[y << 1] = out[(y << 1) + 1] = *p;
  set_bkg_data(idx, 1, out);
}

static uint8_t wg_tile(uint8_t i);            /* defined with the new top score widget */

/* Writing a tile to VRAM is slow (4 frames for the 12 scoreboard tiles), so only the cells that differ from what
 * is already in VRAM (prev) are written. force writes everything. */
/* returns 1 if cells that differ are left (more than limit) */
static uint8_t cells_flush(const uint8_t *buf, uint8_t *prev, uint8_t stride, uint8_t cols, uint8_t rows,
                           uint8_t first_tile, uint8_t widget, uint8_t force, uint8_t limit) {
  uint8_t cx, cy, y, changed, i = 0, written = 0, pending = 0, out[16];
  uint16_t base = 0, o;
  for (cy = 0; cy < rows; cy++) {
    for (cx = 0; cx < cols; cx++, i++) {
      changed = force;
      o = base + cx;
      for (y = 0; y < 8 && !changed; y++, o += stride) changed = buf[o] != prev[o];
      if (!changed) continue;
      if (written >= limit) { pending = 1; continue; }
      o = base + cx;
      for (y = 0; y < 8; y++, o += stride) { out[y << 1] = out[(y << 1) + 1] = buf[o]; prev[o] = buf[o]; }
      set_bkg_data(widget ? wg_tile(i) : first_tile + i, 1, out);
      written++;
    }
    base += (uint16_t)stride << 3;
  }
  return pending;
}

/* The scoreboard: tile columns 7..12 of the two HUD tile rows. Its white inside is 37x9 at canvas
 * x 5..41; digits are 6 px apart and right aligned. */
#define SB_COL  7
#define SB_TILE 101
static uint8_t sb_buf[16 * 6];

static uint8_t sb_prev[16 * 6];
static uint8_t sb_force, sb_dirty;

static void sb_init(void) {
  uint8_t t[12], i;
  sb_force = 1;                            /* the tiles in VRAM are unknown: write them all next time */
  for (i = 0; i < 12; i++) t[i] = SB_TILE + i;
  set_bkg_tiles(SB_COL, 0, 6, 2, t);
}

static void sb_draw(uint32_t score) {
  char num[11], *d = num;
  uint8_t n, i;
  for (i = 0; i < 16 * 6; i += 2) { sb_buf[i] = sb_rows[i]; sb_buf[i + 1] = sb_rows[i + 1]; }
  n = u32_str(score, num);
  if (n > 6) { d += n - 6; n = 6; }
  for (i = 0; i < n; i++) blit_glyph(sb_buf, 6, 42 - 6 * n + 6 * i, 3, d[i]);
  sb_dirty = 1;
}

/* The tile writes are spread over the following frames (1 a frame) instead of stalling the one where the lion ate. */
static void sb_service(uint8_t limit) {
  if (!sb_dirty) return;
  sb_dirty = cells_flush(sb_buf, sb_prev, 6, 6, 2, SB_TILE, 0, sb_force, limit);
  sb_force = 0;
}

static void draw_hud(void) {
  sb_draw(player.score);
  hud_dirty = 0;
}

/* ---- sight lines -----------------------------------------------------------------
 *
 * The Arduboy draws an aim line while a zoo keeper or tranq truck still has its dart.
 * Zoo keeper: a 1px horizontal line from the left edge to him. It sits exactly on the
 * first pixel row of a BG tile row for each lane, so it is drawn with BG tiles.
 * Truck: the line runs diagonally across the lanes, drawn with 8x16 sprites.
 */

#define SIGHT_ROW0     9      /* BG tile row of the lane 1 line (screen y 72), then every 3 rows */
#define BG_HLINE       92     /* 8 tiles: line 1..8 px long, left aligned */
#define BG_SOLID       100    /* all black */
#define SPR_SIGHT_TILE SPR_TILE_COUNT   /* diagonal line piece, 16 rows tall */

static uint8_t hline_len[3];

static void build_runtime_tiles(void) {
  uint8_t t[16];
  uint8_t len, i, r;

  for (len = 1; len <= 8; len++) {
    for (i = 0; i < 16; i++) t[i] = 0;
    t[0] = t[1] = (uint8_t)(0xFF00u >> len);
    set_bkg_data(BG_HLINE + len - 1, 1, t);
  }
  for (i = 0; i < 16; i++) t[i] = 0xFF;
  set_bkg_data(BG_SOLID, 1, t);

  /* 45 degree diagonal, 8 rows: 1 px to the left per row, top row at the right edge (x=7).
   * The second tile of the 8x16 sprite stays empty. */
  for (i = 0; i < 16; i++) t[i] = 0;
  for (r = 0; r < 8; r++) t[r * 2] = t[r * 2 + 1] = (uint8_t)(1u << r);
  set_sprite_data(SPR_SIGHT_TILE, 1, t);
  for (i = 0; i < 16; i++) t[i] = 0;
  set_sprite_data(SPR_SIGHT_TILE + 1, 1, t);
}

static void update_zoo_sights(void) {
  uint8_t want[3];
  uint8_t i, lane, k, full, part;
  uint8_t buf[20];
  int16_t px;

  want[0] = want[1] = want[2] = 0;
  for (i = 0; i < TOTAL_ENTITIES; i++) {
    Entity *e = &entities[i];
    if (e->exists && e->type == TRANQ_ZOO_KEEPER_ENTITY && e->darts > 0 && e->row > 0) {
      px = e->x >> 3;
      want[e->row - 1] = px < 0 ? 0 : px > SCREEN_W ? SCREEN_W : (uint8_t)px;
    }
  }
  for (lane = 0; lane < 3; lane++) {
    uint8_t old = hline_len[lane], lo, hi;
    if (want[lane] == old) continue;
    hline_len[lane] = want[lane];
    /* only the tiles between the old and the new end of the line change */
    lo = (want[lane] < old ? want[lane] : old) >> 3;
    hi = (want[lane] < old ? old : want[lane]) >> 3;
    if (hi > 19) hi = 19;
    full = want[lane] >> 3;
    part = want[lane] & 7;
    for (k = lo; k <= hi; k++) {
      if (k < full)                buf[k - lo] = BG_HLINE + 7;
      else if (k == full && part)  buf[k - lo] = BG_HLINE + part - 1;
      else                         buf[k - lo] = BG_FONT;
    }
    set_bkg_tiles(lo, SIGHT_ROW0 + lane * 3, hi - lo + 1, 1, buf);
  }
}

#define SIGHT_PIECES 12   /* 8 rows each: enough for the lowest lane. Fewer if OAM is crowded */
static uint8_t sight_n;

static void update_truck_sight(void) {
  uint8_t i, k;
  Entity *truck = 0;
  OAM_item_t *o;
  int16_t x, y;

  for (i = 0; i < TOTAL_ENTITIES; i++) {
    if (entities[i].exists && entities[i].type == TRANQ_TRUCK_ENTITY && entities[i].darts > 0) {
      truck = &entities[i];
      break;
    }
  }
  if (!truck) {
    if (sight_slot != NO_SLOT) { oam_release(sight_slot, sight_n); sight_slot = NO_SLOT; }
    return;
  }
  if (sight_slot == NO_SLOT) {
    for (sight_n = SIGHT_PIECES; sight_n >= 3; sight_n--) {
      sight_slot = oam_alloc(sight_n);
      if (sight_slot != NO_SLOT) break;
    }
    if (sight_slot == NO_SLOT) return;
    for (k = 0; k < sight_n; k++) shadow_OAM[sight_slot + k].tile = SPR_SIGHT_TILE;
  }
  x = (truck->x >> 3) + 1 - 7 + 8;    /* OAM x of the first piece: the line starts at px + 1 */
  y = truck->y + 21 + 16;
  o = &shadow_OAM[sight_slot];
  for (k = 0; k < sight_n; k++, o++) {
    o->x = (x > 0 && x < 168) ? (uint8_t)x : 0;     /* 0 = hidden */
    o->y = (uint8_t)y;
    x -= 8;
    y += 8;
  }
}

#ifdef DEBUG_PERF
/* DIV ticks every 256 cycles; per-phase totals are written to cart RAM */
static uint8_t perf_t;
static uint16_t perf_acc[8];
static uint8_t perf_peak[8];
static uint8_t perf_cur[8];
static uint16_t over_acc[8], over_n;
static uint16_t perf_v0;
static uint8_t perf_it0, perf_w0;
static uint16_t it_acc, wait_acc;
static uint16_t line_bins[4];
static uint16_t lines_total;
#define PERF_START() (perf_t = DIV_REG)
#define PERF_REAL(n) do { uint8_t t_ = DIV_REG; uint8_t d_ = (uint8_t)(t_ - perf_t); perf_acc[n] += d_; perf_cur[n] += d_; if (d_ > perf_peak[n]) perf_peak[n] = d_; perf_t = DIV_REG; } while (0)
#ifdef DEBUG_PERF_DRAW
#define PERF_MARK(n)
#define PERF_MARKD(n) PERF_REAL(n)
#else
#define PERF_MARK(n) PERF_REAL(n)
#define PERF_MARKD(n)
#endif
#else
#define PERF_START()
#define PERF_MARK(n)
#define PERF_MARKD(n)
#endif

/* the lion changes every frame; everything else updates its own OAM entries when it moves */
static void update_sprites(void) {
  if (lion_slot != NO_SLOT) {
    if (player.frame != lion_shown_frame) {
      lion_shown_frame = player.frame;
      spr_set_tiles(lion_slot, SPR_LION, player.frame);
    }
    spr_x(lion_slot, SPR_LION, (uint8_t)(player.x + 8));
    spr_y(lion_slot, SPR_LION, (uint8_t)(player.y + 16));
  }
  update_truck_sight();
}


static void game_setup(void) {
  uint8_t i;
  level = 0;
  level_frames = 0;
#ifdef DEBUG_LEVEL
  level = DEBUG_LEVEL;
#endif
  for (i = 0; i < TOTAL_ENTITIES; i++) {
    entities[i].creation_timer = odds[i * 6];
    entities[i].row = i;
    entities[i].type = 0;
    entities[i].exists = 0;
    entities[i].slot = NO_SLOT;
  }
  for (i = 0; i < TOTAL_DARTS; i++) {
    darts[i].exists = 0;
    darts[i].slot = NO_SLOT;
  }
  oam_reset();
  sight_slot = NO_SLOT;
  lion_slot = oam_alloc(spr_pieces(SPR_LION));
  lion_shown_frame = 0xFF;
  player.score = 0;
#ifdef DEBUG_SCORE_START
  player.score = DEBUG_SCORE_START;
#endif
  player.jump = 0;
  player.x = 0;
  player.y = ROW_START;
  player.row = ROW_START;
  player.frame = 0;
  player.sliding = 0;
  player.slide_timer = 0;
  player.multiplier_timer = 0;
  hotsauce_timer = 3600;
  hud_dirty = 1;
  bg_fence_x = fg_fence_x = 0;
  hline_len[0] = hline_len[1] = hline_len[2] = 0;

  clear_screen();
  sb_init();
  draw_hud();
  sb_service(255);                          /* the first time everything is written at once */
  game_state = STATE_PLAYING;
  fx_set(1);
}

static void update_playing(void) {
  uint8_t i;
  PERF_START();

  if (hotsauce_timer > 0) hotsauce_timer--;
  if (++level_frames == 900) {
    level_frames = 0;
    if (level < MAX_LEVEL) level++;
  }

  /* fences: background 1px/frame (16px period), foreground 2px/frame (32px period) */
  bg_fence_x = (bg_fence_x + 1) & 15;
  fg_fence_x = (fg_fence_x + 2) & 31;
  fx_scx[0] = bg_fence_x;
  fx_scx[2] = fg_fence_x;
  fx_scx[4] = fg_fence_x;

  PERF_MARK(0);
  anim_update();
  update_player();
  PERF_MARK(1);
  for (i = 0; i < TOTAL_ENTITIES; i++) tick_entity(i);
  PERF_MARK(2);
  tick_darts();
  update_zoo_sights();
  PERF_MARK(3);
  entity_collisions();
  if (game_state == STATE_PLAYING) dart_collisions();
  PERF_MARK(4);

  if (player.multiplier_timer > 0) {
    player.multiplier_timer--;
    text(14, 0, "2X");
    if (player.multiplier_timer == 0) fill_bkg_rect(14, 0, 2, 1, BG_FONT);
  }
  if (hud_dirty) draw_hud();
  sb_service(1);
  PERF_MARK(5);
  update_sprites();
  PERF_MARK(6);
#ifdef DEBUG_PERF
#endif
}

/* ---- screens ---------------------------------------------------------------------- */

/* ---- cursor ---------------------------------------------------------------------
 *
 * The arrow (20x12) is three 8x16 sprites. On the title its tiles are part of the picture's tile
 * data (TITLE_CURSOR_SLOT); everywhere else they are loaded at CURSOR_GAME_TILE. It bobs 3 pixels
 * down and back up.
 */

#define CURSOR_GAME_TILE 140
static uint8_t cursor_slot = NO_SLOT;
static uint8_t bob_t, bob_i;
static const uint8_t bob_tab[6] = { 0, 1, 2, 3, 2, 1 };

static void hide_all_sprites(void) {
  oam_reset();
  lion_slot = NO_SLOT;
  sight_slot = NO_SLOT;
  cursor_slot = NO_SLOT;
}

static void cursor_show(uint8_t tile0) {
  uint8_t k;
  cursor_slot = oam_alloc(3);
  if (cursor_slot == NO_SLOT) return;
  for (k = 0; k < 3; k++) shadow_OAM[cursor_slot + k].tile = tile0 + (k << 1);
}

/* x, y: screen position of the 24x16 sprite; the arrow is 20x12 in its top left, 2 px in */
static void cursor_move(uint8_t x, uint8_t y) {
  uint8_t k;
  if (cursor_slot == NO_SLOT) return;
  for (k = 0; k < 3; k++) {
    shadow_OAM[cursor_slot + k].x = x + 8 + (k << 3);
    shadow_OAM[cursor_slot + k].y = y + 16;
  }
}

static uint8_t cursor_bob(void) {
  if (++bob_t >= 5) {
    bob_t = 0;
    if (++bob_i >= 6) bob_i = 0;
  }
  return bob_tab[bob_i];
}

/* ---- tiles ------------------------------------------------------------------------ */

static void load_game_tiles(void) {
  fx_set_mode(FXM_OFF);                    /* 0x8800 addressing */
  set_bkg_data(0, BG_TILE_COUNT, bg_tiles);
  build_runtime_tiles();
  set_sprite_data(0, SPR_TILE_COUNT, spr_tiles);
  set_sprite_data(CURSOR_GAME_TILE, 6, cursor_tiles);
}

/* white on black text, for the black area of the panels */
static void text_inv(uint8_t col, uint8_t row, const char *s) {
  uint8_t buf[20], n = 0;
  char ch;
  while ((ch = *s++) != 0 && n < 20) {
    if (ch >= 'a' && ch <= 'z') ch -= 32;
    buf[n++] = (ch < 32 || ch > 95) ? SCORES_FONT : (uint8_t)(SCORES_FONT + ch - 32);
  }
  if (n) set_bkg_tiles(col, row, n, 1, buf);
}

static void text_inv_center(uint8_t row, const char *s) {
  uint8_t n = 0;
  while (s[n]) n++;
  text_inv(1 + ((18 - n) >> 1), row, s);
}

static void enter_intro(void) {
  fx_set(0);
  hide_all_sprites();
  set_bkg_tiles(0, 0, 20, 18, logo_map);   /* full screen logo */
  game_state = STATE_INTRO;
  state_timer = 0;
}

/* ---- title screen -------------------------------------------------------------------
 *
 * The title is a full screen picture (see tools/TitleTiles.cs) with three buttons: PLAY, SCORES
 * and SOUND, which shows its on or off picture. The cursor bobs above the selected one.
 */

/* set_bkg_data() decides between 0x8000 and 0x9000 for tiles below 128 from the current LCDC bit, and
 * that bit is toggled by the title interrupts, so tile data is written to VRAM directly here. */
static void tile_write(uint8_t idx, const uint8_t *src) {
  volatile uint8_t *dst = (volatile uint8_t *)((idx >= 128 ? 0x8000u : 0x9000u) + ((uint16_t)idx << 4));
  uint8_t i;
  for (i = 0; i < 16; i++) {
    while (STAT_REG & STATF_BUSY) ;
    *dst++ = *src++;
  }
}

/* The sound button: its black and white pixels over the picture, transparent pixels show the picture. */
static void title_sound(void) {
  uint8_t cx, cy, y, out[16];
  uint16_t cell;
  const uint8_t *icon, *orig;

  SWITCH_ROM(TITLE_BANK);
  for (cy = 0; cy < 2; cy++) {
    for (cx = 0; cx < 4; cx++) {
      cell = (uint16_t)(TITLE_ICON_ROW + cy) * 20 + TITLE_ICON_COL + cx;
      orig = &title_dyn_orig[(uint16_t)title_dyn_index[cell] << 4];
      icon = &title_icon[(((uint16_t)(sound_on ? 0 : 1) << 3) + (cy << 2) + cx) << 4];
      for (y = 0; y < 8; y++)
        out[y << 1] = out[(y << 1) + 1] = (orig[y << 1] & ~icon[y]) | icon[8 + y];
      tile_write(title_map[cell], out);
    }
  }
}

static const uint8_t menu_x[3] = { MENU_BTN_X0, MENU_BTN_X1, MENU_BTN_X2 };
static uint8_t menu_sel;                   /* 0 PLAY, 1 SCORES, 2 SOUND */

static void menu_cursor_update(void) {
  cursor_move(menu_x[menu_sel] + 4, MENU_BTN_Y - 16 + cursor_bob());
}

/* black while tile data is replaced, like the splash to title */
static void screen_black(void) { BGP_REG = 0xFF; }
static void screen_show(void) {
  wait_vbl_done();
  wait_vbl_done();
  BGP_REG = 0xE4;
}

static void load_title(void) {
  screen_black();
  fx_set_mode(FXM_OFF);
  hide_all_sprites();
  SWITCH_ROM(TITLE_BANK);
  set_sprite_data(0, 128, title_lo);       /* VRAM slots 0..255 through the sprite addressing */
  set_sprite_data(128, 128, title_lo + 128 * 16);
  set_bkg_data(0, 128, title_hi);          /* slots 256..383 */
  set_bkg_tiles(0, 0, 20, 18, title_map);
  fx_set_mode(FXM_TITLE);
  title_sound();
  cursor_show(TITLE_CURSOR_SLOT);
  menu_cursor_update();
  screen_show();
}

static void enter_title(uint8_t music) {
  menu_sel = 0;
  load_title();
  game_state = STATE_TITLE;
  state_timer = 0;
  if (music) snd_play(snd_intro);
}

/* ---- the flashing "press A button" -------------------------------------------------------
 *
 * 8x3 tiles at tile column 6, row 14, in the lower white area of the panels. The button's tiles are loaded
 * once; flashing switches the map between those tiles and the panel's plain white tile.
 */
#define PB_COL  6
#define PB_ROW  14
#define PB_TILE 101                        /* 24 tiles, the widget's range: the widget is not on these screens */
static uint8_t pb_blank, pb_on, pb_timer;

static void pb_set(uint8_t on) {
  uint8_t m[24], i;
  for (i = 0; i < 24; i++) m[i] = on ? PB_TILE + i : pb_blank;
  set_bkg_tiles(PB_COL, PB_ROW, 8, 3, m);
}

static void pb_load(uint8_t blank_tile) {
  uint8_t cx, cy;
  for (cy = 0; cy < 3; cy++)
    for (cx = 0; cx < 8; cx++) cell_write(PB_TILE + cy * 8 + cx, press_rows, 8, cx, cy);
  pb_blank = blank_tile;
  pb_on = 1;
  pb_timer = 0;
  pb_set(1);
}

static void pb_update(void) {
  if (++pb_timer >= 30) {
    pb_timer = 0;
    pb_on ^= 1;
    pb_set(pb_on);
  }
}

/* ---- top scores screen ------------------------------------------------------------------- */

#define SC_ROW         4                   /* text row of the first entry: 8 rows, centred in the black area */

static uint8_t scores_hl;                  /* entry to blink, NUM_SCORES = none */
static uint8_t scores_blink;

static void draw_score_row(uint8_t i, uint8_t visible) {
  char buf[12];
  uint8_t n, k, p = 0;
  for (k = 0; k < 11; k++) buf[k] = ' ';
  buf[11] = 0;
  if (visible) {
    buf[0] = '1' + i;
    for (k = 0; k < 3; k++) buf[2 + k] = score_val[i] ? score_ini[i][k] : '-';
    {
      char num[6];
      n = u32_str(score_val[i], num);
      p = 11 - n;
      for (k = 0; k < n; k++) buf[p + k] = num[k];
    }
  }
  text_inv(4, SC_ROW + i, buf);
}

static void enter_scores(uint8_t hl) {
  uint8_t i;
  screen_black();
  fx_set_mode(FXM_OFF);
  hide_all_sprites();
  load_game_tiles();
  clear_screen();
  set_bkg_data(SCORES_BASE, SCORES_TILES, scores_tiles);
  set_bkg_data(SCORES_FONT, 64, font_inv);
  set_bkg_tiles(0, 0, 20, 18, scores_map);

  for (i = 0; i < NUM_SCORES; i++) draw_score_row(i, 1);

  pb_load(scores_map[15 * 20 + 10]);
  scores_hl = hl;
  scores_blink = 0;
  game_state = STATE_SCORES;
  state_timer = 0;
  screen_show();
}

static void update_scores(void) {
  if (scores_hl < NUM_SCORES && ++scores_blink >= 20) {
    scores_blink = 0;
    state_timer ^= 0x100;                  /* bit 8 is the blink phase, below it counts frames */
    draw_score_row(scores_hl, !(state_timer & 0x100));
  }
  if ((state_timer & 0xFF) < 255) state_timer++;
  pb_update();
  if ((state_timer & 0xFF) > 20 && (pressed & (J_A | J_B | J_START))) enter_title(0);
}

/* ---- starting a game: black out, story over the scrolling view, lion runs in --------------------- */

static const char *const story_lines[7] = {
  "LEANNA MAY BE A",
  "LION, BUT SHE KNOWS",
  "WHAT SHE WANTS: SHE",
  "WANTS FRESH MEAT",
  "FROM HER FAVORITE",
  "RESTAURANT. THE ZOO",
  "HAD TO CLOSE.",
};
#define STORY_ROW      9      /* the text sits in the blank band between the two rails */
#define STORY_LINES    7
#define STORY_HOLD     60     /* frames to read it before it disappears */
#define LION_RUN_IN    1      /* px per frame the lion runs in from the left edge after the story */

static uint8_t story_line, story_col, story_wait, story_phase;   /* phase 0 typing, 1 hold, 2 lion runs in */
static uint8_t story_timer;

static void scroll_fences(void) {
  bg_fence_x = (bg_fence_x + 1) & 15;
  fg_fence_x = (fg_fence_x + 2) & 31;
  fx_scx[0] = bg_fence_x;
  fx_scx[2] = fg_fence_x;
  fx_scx[4] = fg_fence_x;
}

/* the lion runs on the spot (off the left edge until the story is done) */
static void story_lion(void) {
  anim_update();
  if (anim_hit[5]) {
    player.frame++;
    if (player.frame > 3) player.frame = 0;
  }
  update_sprites();
}

static void begin_story_scene(void) {
  uint8_t i;
  load_game_tiles();

  for (i = 0; i < TOTAL_ENTITIES; i++) { entities[i].exists = 0; entities[i].slot = NO_SLOT; }
  for (i = 0; i < TOTAL_DARTS; i++) { darts[i].exists = 0; darts[i].slot = NO_SLOT; }
  player.x = -24;                          /* off the left edge: no lion while the story is typed */
  player.y = ROW_START;
  player.row = ROW_START;
  player.frame = 0;
  player.sliding = 0;
  player.jump = 0;
  hide_all_sprites();
  lion_slot = oam_alloc(spr_pieces(SPR_LION));
  lion_shown_frame = 0xFF;

  clear_screen();
  bg_fence_x = fg_fence_x = 0;
  scroll_fences();
  fx_scx[1] = fx_scx[3] = 255;                /* the story text sits 1 px to the right (the blank bands scroll by -1) */
  fx_hud_scy = 0;
  fx_set_mode(FXM_GAME);

  story_line = story_col = story_wait = story_phase = 0;
  story_timer = 0;
  game_state = STATE_STORY;
}

static void go_play(void) {
  initrand(((uint16_t)DIV_REG << 8) ^ frame_count);
  screen_black();
  begin_story_scene();
  screen_show();
}

static void story_type_char(void) {
  const char *s = story_lines[story_line];
  char ch = s[story_col];
  set_bkg_tile_xy(story_col, STORY_ROW + story_line, ch >= 32 ? BG_FONT + (ch - 32) : BG_FONT);
  story_col++;
  if (s[story_col] == 0) {
    story_line++;
    story_col = 0;
    if (story_line == STORY_LINES) { story_phase = 1; story_timer = 0; }
  }
}

static void story_finish(void) {
  fx_scx[1] = fx_scx[3] = 0;
  fill_bkg_rect(0, STORY_ROW, 20, STORY_LINES, BG_FONT);
  game_setup();
}

static void update_story(void) {
  scroll_fences();
  story_lion();

  if (story_phase == 0) {
    if (pressed & (J_A | J_B | J_START)) {
      while (story_phase == 0) story_type_char();       /* show the rest at once */
    } else if (story_wait) {
      story_wait--;
    } else {
      story_type_char();
      story_wait = 1;                                     /* 1 character every 2 frames = 30 a second */
    }
  } else if (story_phase == 1) {
    if ((pressed & (J_A | J_B | J_START)) || ++story_timer >= STORY_HOLD) {
      story_phase = 2;                                  /* the text disappears, the lion runs in */
      fill_bkg_rect(0, STORY_ROW, 20, STORY_LINES, BG_FONT);
    }
  } else {
    player.x += LION_RUN_IN;
    if (player.x >= 0 || (pressed & (J_A | J_B | J_START))) story_finish();   /* the lion is home: play */
  }
}

/* ---- title input -------------------------------------------------------------------------------- */

static void update_title(void) {
  if (pressed & J_RIGHT) menu_sel = menu_sel >= 2 ? 0 : menu_sel + 1;
  if (pressed & J_LEFT)  menu_sel = menu_sel == 0 ? 2 : menu_sel - 1;

  if (pressed & J_START) {
    go_play();
    return;
  }
  if (pressed & J_A) {
    if (menu_sel == 0) {
      go_play();
      return;
    }
    if (menu_sel == 1) {
      enter_scores(NUM_SCORES);
      return;
    }
    if (sound_on) {
      snd_stop();
      sound_on = 0;
    } else {
      sound_on = 1;
      snd_play(snd_intro);
    }
    save_settings();
    title_sound();
  }
  menu_cursor_update();
}

/* ---- game over: panel, and the initials of a new top 5 score -------------------------------------
 *
 * The panel is a full screen picture: the quip and the score go in its black area (white text), and
 * the lower white area holds either PRESS A or the "new top score" widget with the initials.
 */

static uint8_t over_rank;                  /* NUM_SCORES = not a top score */
static uint8_t ini_pos, ini_char[3];       /* ini_char: 0..25 = A..Z */

#define WG_COL 3                           /* the widget canvas is 14x3 tiles at tile column 3, row 14 */
#define WG_ROW 14
static uint8_t wg_buf[24 * 14], wg_prev[24 * 14];
static uint8_t wg_force;
static const uint8_t ini_px[3] = { 72, 83, 94 };    /* x of each letter in the widget canvas */
#define INI_PY 6                           /* y of the letters in the canvas */

/* tile numbers for the 42 widget cells: 101..139, then 246.. (free while the game over screen shows) */
static uint8_t wg_tile(uint8_t i) { return i < 39 ? 101 + i : 246 + (i - 39); }

static void wg_draw(void) {
  uint8_t i;
  for (i = 0; i < 24 * 14 / 2; i++) { wg_buf[i << 1] = widget_rows[i << 1]; wg_buf[(i << 1) + 1] = widget_rows[(i << 1) + 1]; }
  for (i = 0; i < 3; i++) blit_glyph(wg_buf, 14, ini_px[i], INI_PY, 'A' + ini_char[i]);
  cells_flush(wg_buf, wg_prev, 14, 14, 3, 0, 1, wg_force, 255);
  wg_force = 0;
}

static void wg_show(void) {
  uint8_t i, m[42];
  for (i = 0; i < 42; i++) m[i] = wg_tile(i);
  set_bkg_tiles(WG_COL, WG_ROW, 14, 3, m);
}

static void ini_cursor(void) {
  /* the arrow is centered over the letter: letter x on screen = canvas x + 24, the arrow is 20 wide */
  cursor_move((uint8_t)(WG_COL * 8 + ini_px[ini_pos] - 10), 102 + cursor_bob());
}

static void enter_over(void) {
  char buf[12];
  uint8_t n;
  screen_black();
  fx_set_mode(FXM_OFF);
  hide_all_sprites();
  clear_screen();

  set_bkg_data(SCORES_BASE, OVER_TILES, over_tiles);
  set_bkg_data(SCORES_FONT, 64, font_inv);
  set_bkg_tiles(0, 0, 20, 18, over_map);

  if (gameover_type == TRAMPLED_TEXT)         text_inv_center(3, "TRAMPLED");
  else if (gameover_type == KNOCKED_OUT_TEXT) text_inv_center(3, "KNOCKED OUT");
  else                                        text_inv_center(3, "CAGED");

  /* the quip, at most 4 lines of 18 characters */
  switch (gameover_text_id) {
    case 0:
      text_inv_center(6, "WHAT, NO MINT?");
      break;
    case 1:
      text_inv_center(5, "ALL THAT FAST FOOD");
      text_inv_center(6, "WAS WORTH IT.");
      break;
    case 2:
      text_inv_center(5, "BONES CAN BE");
      text_inv_center(6, "TOOTHPICKS,");
      text_inv_center(7, "I SUPPOSE.");
      break;
    default:
      text_inv_center(5, "I DEMAND TO SEE");
      text_inv_center(6, "YOUR MANAGER.");
      text_inv_center(7, "I'M SURE THEY'LL");
      text_inv_center(8, "BE DELICIOUS.");
      break;
  }

  text_inv(2, 10, "SCORE");
  n = u32_str(player.score, buf);
  text_inv(18 - n, 10, buf);
  text_inv(2, 11, "BEST");
  n = u32_str(score_val[0] > player.score ? score_val[0] : player.score, buf);
  text_inv(18 - n, 11, buf);

  over_rank = score_rank(player.score);
  if (over_rank < NUM_SCORES) {
    ini_pos = 0;
    ini_char[0] = ini_char[1] = ini_char[2] = 0;
    wg_show();
    wg_force = 1;
    wg_draw();
    cursor_show(CURSOR_GAME_TILE);
    ini_cursor();
    game_state = STATE_INITIALS;
  } else {
    pb_load(over_map[15 * 20 + 10]);
    game_state = STATE_OVER;
  }
  state_timer = 0;
  screen_show();
}

static void update_initials(void) {
  char ini[3];
  uint8_t k;
  if (state_timer < 255) state_timer++;
  if (state_timer > 20) {
    if (pressed & J_UP)   { ini_char[ini_pos] = ini_char[ini_pos] >= 25 ? 0 : ini_char[ini_pos] + 1; wg_draw(); }
    if (pressed & J_DOWN) { ini_char[ini_pos] = ini_char[ini_pos] == 0 ? 25 : ini_char[ini_pos] - 1; wg_draw(); }
    if ((pressed & J_LEFT) && ini_pos > 0) ini_pos--;
    if ((pressed & J_RIGHT) && ini_pos < 2) ini_pos++;
    if (pressed & J_B) { if (ini_pos > 0) ini_pos--; }
    if (pressed & (J_A | J_START)) {
      if (ini_pos < 2 && !(pressed & J_START)) {
        ini_pos++;
      } else {                                   /* last letter: done */
        for (k = 0; k < 3; k++) ini[k] = 'A' + ini_char[k];
        score_insert(over_rank, ini, player.score);
        scores_save();
        enter_scores(over_rank);
        return;
      }
    }
  }
  ini_cursor();
}

/* ---- main -------------------------------------------------------------------------- */

void main(void) {
  DISPLAY_OFF;
  SWITCH_ROM(TITLE_BANK);          /* all tile data lives in ROM bank 1 */
  NR52_REG = 0x80;
  NR51_REG = 0xFF;
  NR50_REG = 0x77;

  BGP_REG  = 0xE4;
  OBP0_REG = 0xD0;           /* 0 transparent, 1 white, 2 light grey, 3 black */

  set_bkg_data(0, BG_TILE_COUNT, bg_tiles);
  set_sprite_data(0, SPR_TILE_COUNT, spr_tiles);
  SPRITES_8x16;

  /* fence strips on the BG map (outside of the visible 20x18 area) */
  {
    uint8_t x, row[32], r2[32], fg[32];
    for (x = 0; x < 32; x++) {
      row[x] = BG_BGFENCE + (x & 1);
      r2[x]  = BG_BGFENCE + 2 + (x & 1);
      fg[x]  = BG_FGFENCE + (x & 3);
    }
    set_bkg_tiles(0, MAP_BGFENCE_ROW,     32, 1, row);
    set_bkg_tiles(0, MAP_BGFENCE_ROW + 1, 32, 1, r2);
    set_bkg_tiles(0, MAP_FGFENCE_ROW,     32, 1, fg);
  }

  build_runtime_tiles();
  set_bkg_data(BG_LOGO, LOGO_TILES, logo_tiles);
  settings_load();
#ifdef DEBUG_SEED_SCORES
  { /* sample table for screenshots */
    static const char s[8][3] = { {'L','E','A'}, {'S','H','D'}, {'K','U','R'}, {'C','R','S'}, {'N','R','G'}, {'Z','O','O'}, {'M','O','M'}, {'D','A','D'} };
    static const uint16_t v[8] = { 486, 412, 355, 298, 241, 187, 134, 96 };
    uint8_t q, k;
    for (q = 0; q < 8; q++) { for (k = 0; k < 3; k++) score_ini[q][k] = s[q][k]; score_val[q] = v[q]; }
  }
#endif

  add_VBL(fx_vbl_isr);
  add_LCD(fx_lcd_isr);
  set_interrupts(VBL_IFLAG | LCD_IFLAG);

  SHOW_BKG;
  SHOW_SPRITES;
  DISPLAY_ON;

  enter_intro();

  while (1) {
#ifdef DEBUG_PERF
    perf_w0 = DIV_REG;
#endif
    wait_vbl_done();
#ifdef DEBUG_PERF
    wait_acc += (uint8_t)(DIV_REG - perf_w0);
    perf_it0 = DIV_REG;
#endif
#ifdef DEBUG_PERF
    if ((frame_count & 63) == 0)
    { /* stats go to cart RAM so the HUD does not disturb the timing */
      uint16_t v = vbl_frames;
      ENABLE_RAM;
      SRAM[16] = (uint8_t)frame_count; SRAM[17] = (uint8_t)(frame_count >> 8);
      SRAM[18] = (uint8_t)v;           SRAM[19] = (uint8_t)(v >> 8);
      { uint8_t k; for (k = 0; k < 8; k++) { SRAM[20 + k * 2] = (uint8_t)perf_acc[k]; SRAM[21 + k * 2] = (uint8_t)(perf_acc[k] >> 8); SRAM[40 + k] = perf_peak[k]; SRAM[58 + k * 2] = (uint8_t)over_acc[k]; SRAM[59 + k * 2] = (uint8_t)(over_acc[k] >> 8); } SRAM[56] = (uint8_t)over_n; SRAM[57] = (uint8_t)(over_n >> 8); }
      SRAM[84] = (uint8_t)line_bins[0]; SRAM[85] = (uint8_t)(line_bins[0] >> 8); SRAM[86] = (uint8_t)line_bins[1]; SRAM[87] = (uint8_t)(line_bins[1] >> 8); SRAM[88] = (uint8_t)line_bins[2]; SRAM[89] = (uint8_t)(line_bins[2] >> 8); SRAM[90] = (uint8_t)line_bins[3]; SRAM[91] = (uint8_t)(line_bins[3] >> 8);
      SRAM[80] = (uint8_t)it_acc; SRAM[81] = (uint8_t)(it_acc >> 8); SRAM[82] = (uint8_t)wait_acc; SRAM[83] = (uint8_t)(wait_acc >> 8);
      { uint8_t k; const uint8_t *src = (const uint8_t *)shadow_OAM; for (k = 0; k < 160; k++) SRAM[128 + k] = src[k]; }
      DISABLE_RAM;
    }
#endif
#ifdef DEBUG_PERF
    { uint8_t k; perf_v0 = vbl_frames; for (k = 0; k < 8; k++) perf_cur[k] = 0; }
#endif
    frame_count++;
    prev_keys = keys;
    keys = joypad();
    pressed = keys & ~prev_keys;
#ifdef DEBUG_BOT
    if (game_state == STATE_PLAYING) {
      static uint8_t c37, c71;
      if (++c37 == 37) { c37 = 0; pressed |= (rand() & 1) ? J_UP : J_DOWN; }
      if (++c71 == 71) { c71 = 0; pressed |= J_A; }
    } else if (game_state == STATE_TITLE && frame_count > BOT_TITLE_DELAY) {
      if (menu_sel < BOT_SELECT) pressed |= J_RIGHT; else pressed |= J_A;
    } else if (game_state == STATE_OVER) pressed |= J_A;
    else if (game_state == STATE_SCORES && state_timer > BOT_SCORES_HOLD) pressed |= J_A;
    else if (game_state == STATE_INITIALS) {
      static uint8_t ci;
      if (++ci >= 30) { ci = 0; pressed |= (rand() & 1) ? J_UP : J_A; }
    }
#endif

#ifdef DEBUG_DUMP_STATE
    { /* copies VRAM, the BG map and OAM into cart RAM once, to be read back from the .sav */
      static uint8_t dumped;
      if (!dumped && game_state == DEBUG_DUMP_STATE && DEBUG_DUMP_T == 40) {
        const volatile uint8_t *v = (const volatile uint8_t *)0x8000;
        uint16_t q;
        dumped = 1;
        ENABLE_RAM;
        for (q = 0; q < 6144; q++) { while (STAT_REG & STATF_BUSY) ; SRAM[0x100 + q] = v[q]; }
        v = (const volatile uint8_t *)0x9800;
        for (q = 0; q < 1024; q++) { while (STAT_REG & STATF_BUSY) ; SRAM[0x1900 + q] = v[q]; }
        { const uint8_t *o = (const uint8_t *)shadow_OAM; for (q = 0; q < 160; q++) SRAM[0x1D00 + q] = o[q]; }
        SRAM[0x1DF0] = 0x5A;
        DISABLE_RAM;
      }
    }
#endif
    snd_update();

    switch (game_state) {
      case STATE_INTRO:
        if (++state_timer > INTRO_FRAMES) enter_title(1);
        break;

      case STATE_TITLE:
        update_title();
        break;

      case STATE_SCORES:
        update_scores();
        break;

      case STATE_STORY:
        update_story();
        break;

      case STATE_INITIALS:
        update_initials();
        break;

      case STATE_PLAYING:
        update_playing();
        if (game_state == STATE_OVER) enter_over();
        break;

      case STATE_OVER:
        pb_update();
        if (++state_timer > 30 && (pressed & (J_A | J_B | J_START))) enter_title(1);
        break;
    }
#ifdef DEBUG_PERF
    it_acc += (uint8_t)(DIV_REG - perf_it0);
    { uint8_t ly = LY_REG; uint16_t used = (uint16_t)(vbl_frames - perf_v0) * 154u + (ly >= 144 ? ly - 144 : ly + 10);
      lines_total += used; line_bins[used <= 77 ? 0 : used <= 154 ? 1 : used <= 231 ? 2 : 3]++; }
    if (vbl_frames != perf_v0) { uint8_t k; over_n++; for (k = 0; k < 8; k++) over_acc[k] += perf_cur[k]; }
#endif
  }
}
