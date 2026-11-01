# Converts the PNGs in gfx/ into Game Boy (DMG) 2bpp tile data: src/gfx_data.c and
# src/gfx_data.h. build.bat runs this before every compile, so edit the PNGs in gfx/ and rebuild.
#
# Sprite sheets are vertical strips of frames. Each frame is cut into 8x16 pieces
# (GB sprites run in 8x16 mode), laid out row by row, left to right. Every piece is
# two tiles: the top 8x8 followed by the bottom 8x8. Sprites shorter than 16px get
# a blank bottom tile.
#
# Sprite palette indexes: 0 = transparent, 1 = white (the mask), 3 = black.

Add-Type -AssemblyName System.Drawing
$gfx  = (Resolve-Path "$PSScriptRoot\..\gfx").Path
$ab   = $gfx
$out  = (Resolve-Path "$PSScriptRoot\..\src").Path

# name, file, frame width, frame height
$sprites = @(
  @('LION',       'lion_24x8',            24,  8),
  @('ELDER',      'elder_8x16',            8, 16),
  @('CHILD',      'child_8x16',            8, 16),
  @('ADULT',      'adult_8x16',            8, 16),
  @('MONKEY',     'monkey_8x8',            8,  8),
  @('MONKEYPOO',  'monkeypoo_16x8',       16,  8),
  @('ELEPHANT',   'elephant_32x32',       32, 32),
  @('PUDDLE',     'waterpuddle_16x8',     16,  8),
  @('TRUCK',      'truck_32x32',          32, 32),
  @('ZOOKEEPER',  'zookeeper_16x16',      16, 16),
  @('CAGE',       'cage_32x16',           32, 16),
  @('ANGLEDDART', 'angledtranqdart_8x8',   8,  8),
  @('DART',       'tranqdart_8x8',         8,  8),
  @('HOTSAUCE',   'hotsauce_8x8',          8,  8)
)

function Load-Bitmap($path) { New-Object System.Drawing.Bitmap($path) }

# mode: 'obj'  transparent 0, white 1, black 3
#       'bg'   transparent/white 0, black 3
#       'ink'  white = ink (3), everything else 0   (font)
function Get-Index($bmp, $x, $y, $mode, $yMin, $yMax) {
  if ($x -lt 0 -or $x -ge $bmp.Width -or $y -lt $yMin -or $y -ge $yMax -or $y -ge $bmp.Height) { return 0 }
  $p = $bmp.GetPixel($x, $y)
  $white = ($p.R -gt 128)
  switch ($mode) {
    'obj' { if ($p.A -lt 128) { return 0 } elseif ($white) { return 1 } else { return 3 } }
    'bg'  { if ($p.A -lt 128 -or $white) { return 0 } else { return 3 } }
    'ink' { if ($p.A -ge 128 -and $white) { return 3 } else { return 0 } }
  }
}

function Encode-Tile($bmp, $x0, $y0, $mode, $yMin, $yMax) {
  $bytes = @()
  for ($row = 0; $row -lt 8; $row++) {
    $lo = 0; $hi = 0
    for ($col = 0; $col -lt 8; $col++) {
      $i = Get-Index $bmp ($x0 + $col) ($y0 + $row) $mode $yMin $yMax
      $lo = $lo -bor (($i -band 1) -shl (7 - $col))
      $hi = $hi -bor ((($i -shr 1) -band 1) -shl (7 - $col))
    }
    $bytes += $lo; $bytes += $hi
  }
  return $bytes
}

function Encode-LogoTile($bmp, $x0, $y0) {
  $bytes = @()
  for ($row = 0; $row -lt 8; $row++) {
    $lo = 0
    for ($col = 0; $col -lt 8; $col++) {
      $p = $bmp.GetPixel($x0 + $col, $y0 + $row)
      if ($p.A -ge 128 -and $p.R -gt 128) { continue }   # white pixel: index 0
      $lo = $lo -bor (1 -shl (7 - $col))
    }
    $bytes += $lo; $bytes += $lo                         # both planes: index 3 (black)
  }
  return $bytes
}

function Format-Bytes($bytes) {
  $sb = New-Object System.Text.StringBuilder
  for ($i = 0; $i -lt $bytes.Count; $i++) {
    if ($i % 16 -eq 0) { [void]$sb.Append("`n  ") }
    [void]$sb.AppendFormat('0x{0:X2},', $bytes[$i])
  }
  return $sb.ToString()
}

# ---- sprites ---------------------------------------------------------------
$sprBytes = @()
$defs = New-Object System.Text.StringBuilder
$enum = New-Object System.Text.StringBuilder
$n = 0
foreach ($s in $sprites) {
  $name, $file, $w, $h = $s
  $bmp = Load-Bitmap "$ab\$file.png"
  $frames = [int]($bmp.Height / $h)
  $cols = [int]($w / 8)
  $rows = [int][math]::Ceiling($h / 16)
  $tile = [int]($sprBytes.Count / 16)
  if ($tile % 2 -ne 0) { throw "sprite $name not aligned to an even tile" }
  for ($f = 0; $f -lt $frames; $f++) {
    $yMin = $f * $h; $yMax = $yMin + $h
    for ($r = 0; $r -lt $rows; $r++) {
      for ($c = 0; $c -lt $cols; $c++) {
        $sprBytes += Encode-Tile $bmp ($c * 8) ($yMin + $r * 16)     'obj' $yMin $yMax
        $sprBytes += Encode-Tile $bmp ($c * 8) ($yMin + $r * 16 + 8) 'obj' $yMin $yMax
      }
    }
  }
  [void]$defs.AppendFormat("  {{ {0}, {1}, {2}, {3}, {4} }}, /* {5} {6}x{7} */`n", $tile, $cols, $rows, $frames, ($cols * $rows * 2), $name, $w, $h)
  [void]$enum.AppendFormat("  SPR_{0},`n", $name)
  $bmp.Dispose()
  $n++
}

# ---- background tiles ------------------------------------------------------
$bgBytes = @()

# 64 glyphs, ASCII 32..95, 8x8 each, vertical strip
$font = Load-Bitmap "$gfx\font_8x8x1.png"
for ($g = 0; $g -lt 64; $g++) { $bgBytes += Encode-Tile $font 0 ($g * 8) 'ink' 0 $font.Height }
$font.Dispose()

# creators banner 160x8 = 20 tiles (the full screen width)
$cr = Load-Bitmap "$ab\creators_160x8.png"
for ($c = 0; $c -lt 20; $c++) { $bgBytes += Encode-Tile $cr ($c * 8) 0 'bg' 0 8 }
$cr.Dispose()

# background fence 16x16: TL TR BL BR
$bf = Load-Bitmap "$ab\backgroundfence_16x16.png"
foreach ($t in @(@(0,0), @(8,0), @(0,8), @(8,8))) { $bgBytes += Encode-Tile $bf $t[0] $t[1] 'bg' 0 16 }
$bf.Dispose()

# foreground fence 32x8 = 4 tiles
$ff = Load-Bitmap "$ab\foregroundfence_32x8.png"
for ($c = 0; $c -lt 4; $c++) { $bgBytes += Encode-Tile $ff ($c * 8) 0 'bg' 0 8 }
$ff.Dispose()

# Intro logo, a full screen 160x144 image shown white on black: white pixels are index 0 on a
# black screen, everything else index 3. Identical tiles are stored once and the 20x18 map points
# at them, so a typical logo needs far fewer tiles than the 360 on screen.
$lg = Load-Bitmap "$gfx\NewNeoRetroWithURL-160x144.png"
if ($lg.Width -ne 160 -or $lg.Height -ne 144) { throw "logo must be 160x144" }
$logoBytes = @()
$logoSeen = @{}
$logoMap = @()
for ($ty = 0; $ty -lt 18; $ty++) {
  for ($tx = 0; $tx -lt 20; $tx++) {
    $tile = Encode-LogoTile $lg ($tx * 8) ($ty * 8)
    $key = $tile -join ','
    if (-not $logoSeen.ContainsKey($key)) {
      $logoSeen[$key] = $logoSeen.Count
      $logoBytes += $tile
    }
    $logoMap += $logoSeen[$key]
  }
}
$lg.Dispose()
$logoTiles = $logoSeen.Count
if ($logoTiles -gt 115) { throw "logo needs $logoTiles tiles, at most 115 fit (tiles 140..255). Simplify the image." }
$logoMapC = @($logoMap | ForEach-Object { $_ + 140 })    # absolute BG tile numbers (BG_LOGO = 140)

# ---- write files -----------------------------------------------------------
$sprTiles = [int]($sprBytes.Count / 16)
$bgTiles  = [int]($bgBytes.Count / 16)

$h = @"
/* Generated by tools/convert_assets.ps1 - do not edit. */
#ifndef GFX_DATA_H
#define GFX_DATA_H

#include <stdint.h>

typedef struct {
  uint8_t tile;    /* first tile of frame 0 */
  uint8_t cols;    /* 8px pieces across */
  uint8_t rows;    /* 8x16 pieces down */
  uint8_t frames;
  uint8_t stride;  /* tiles per frame */
  uint8_t pad[3];  /* 8 bytes: indexing sprdefs is a shift */
} SprDef;

enum {
$($enum.ToString())  SPR_COUNT
};

#define SPR_TILE_COUNT $sprTiles
#define BG_TILE_COUNT  $bgTiles

/* BG tile indexes. Font glyph = ASCII - 32 (space..'_', upper case only). */
#define BG_FONT     0
#define BG_CREATORS 64   /* 20 tiles */
#define BG_BGFENCE  84
#define BG_FGFENCE  88
#define BG_LOGO     140   /* above the sprite tiles, which end at 139 */
#define LOGO_TILES  $logoTiles

extern const SprDef sprdefs[SPR_COUNT];
extern const unsigned char spr_tiles[];
extern const unsigned char bg_tiles[];
extern const unsigned char logo_tiles[];   /* LOGO_TILES unique tiles */
extern const unsigned char logo_map[];     /* 20x18 BG map, tile numbers already include BG_LOGO */

#endif
"@

$c = @"
/* Generated by tools/convert_assets.ps1 - do not edit. */
#include "gfx_data.h"

const SprDef sprdefs[SPR_COUNT] = {
$($defs.ToString())};
"@

Set-Content -Path "$out\gfx_data.h" -Value $h -Encoding ascii
Set-Content -Path "$out\gfx_data.c" -Value $c -Encoding ascii
"sprite tiles: $sprTiles  bg tiles: $bgTiles"


# ---- title screen, menu, top scores panel ----------------------------------------
. "$PSScriptRoot\convert_title.ps1"
