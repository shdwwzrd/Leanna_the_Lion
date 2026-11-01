# Title screen, menu buttons, cursor and top scores panel. Dot-sourced by convert_assets.ps1
# (it uses that script's $gfx, $out, Load-Bitmap, Encode-Tile and Format-Bytes).
#
# The title is packed into more tiles than a single tile data bank holds, see tools/TitleTiles.cs.
# The three menu buttons (Menu-32x32.png: frame 0 sound on, 1 sound off, 2 PLAY, 3 SCORES) are 32x16.
# PLAY and SCORES are part of the picture; the sound button is a run time cell that shows frame 0 or 1.

$btnX = @(8, 64, 120)          # PLAY, SCORES, SOUND (8 pixel columns 1, 8, 15)
$btnY = 112                    # tile row 14
$iconRow = $btnY / 8
$iconCol = $btnX[2] / 8

$dynCells = New-Object 'bool[]' 360
for ($cy = 0; $cy -lt 2; $cy++) { for ($cx = 0; $cx -lt 4; $cx++) { $dynCells[($iconRow + $cy) * 20 + $iconCol + $cx] = $true } }

Add-Type -Path "$PSScriptRoot\TitleTiles.cs" -ReferencedAssemblies System.Drawing
$ov = New-Object 'System.Collections.Generic.List[Overlay]'
$ov.Add((New-Object Overlay("$gfx\Menu-32x32.png", 32, 32, 16, $btnX[0], $btnY)))
$ov.Add((New-Object Overlay("$gfx\Menu-32x32.png", 48, 32, 16, $btnX[1], $btnY)))
$cursor = [TitleTiles]::CursorTiles("$gfx\cursor_20x12.png")
$tr = [TitleTiles]::Build("$gfx\leanna_the_lion_title_screen_160x144.png", "$gfx\creators_160x8.png", $ov, $dynCells, $cursor)
$dynN = [int]($tr.DynOrig.Length / 16)

# sound button cells: per frame (on, off) and cell, 8 bytes of "opaque" rows then 8 bytes of "black" rows
$ic = Load-Bitmap "$gfx\Menu-32x32.png"
$iconBytes = @()
for ($f = 0; $f -lt 2; $f++) { for ($cy = 0; $cy -lt 2; $cy++) { for ($cx = 0; $cx -lt 4; $cx++) {
  $op = @(); $bk = @()
  for ($y = 0; $y -lt 8; $y++) {
    $o = 0; $k = 0
    for ($x = 0; $x -lt 8; $x++) {
      $p = $ic.GetPixel($cx * 8 + $x, $f * 16 + $cy * 8 + $y)
      if ($p.A -ge 128) { $o = $o -bor (0x80 -shr $x); if ($p.R -le 128) { $k = $k -bor (0x80 -shr $x) } }
    }
    $op += $o; $bk += $k
  }
  $iconBytes += $op; $iconBytes += $bk
} } }
$ic.Dispose()

# ---- panels (112x88) for the top scores and game over screens, and a white-on-black copy of the font ----
# for the black area. Both panels load at the same BG tile numbers (they are never on screen together).
$scoresBase = 150
# The shared full screen panel with a banner drawn in its top white section, centred.
function Convert-Panel($bannerFile, $bannerY) {
  $wTiles = 20; $hTiles = 18
  $panel = Load-Bitmap (Join-Path $gfx 'FullScreenPanel_160x144.png')
  if ($panel.Width -ne 160 -or $panel.Height -ne 144) { throw "FullScreenPanel_160x144.png must be 160x144" }
  $banner = Load-Bitmap (Join-Path $gfx $bannerFile)
  $bmp = New-Object System.Drawing.Bitmap 160, 144, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
  $gr = [System.Drawing.Graphics]::FromImage($bmp)
  $gr.InterpolationMode = 'NearestNeighbor'; $gr.PixelOffsetMode = 'Half'
  # explicit sizes: DrawImageUnscaled would scale by the png's dpi
  $gr.DrawImage($panel, 0, 0, 160, 144)
  $gr.DrawImage($banner, [int]((160 - $banner.Width) / 2), $bannerY, $banner.Width, $banner.Height)
  $gr.Dispose(); $panel.Dispose(); $banner.Dispose()
  $seen = @{}; $bytes = @(); $map = @()
  for ($ty = 0; $ty -lt $hTiles; $ty++) {
    for ($tx = 0; $tx -lt $wTiles; $tx++) {
      $t = Encode-Tile $bmp ($tx * 8) ($ty * 8) 'bg' 0 $bmp.Height
      $key = $t -join ','
      if (-not $seen.ContainsKey($key)) { $seen[$key] = $seen.Count; $bytes += $t }
      $map += ($seen[$key] + $scoresBase)
    }
  }
  $bmp.Dispose()
  if ($seen.Count -gt 42) { throw "$file needs $($seen.Count) tiles, at most 42 fit" }
  return @{ Tiles = $seen.Count; Bytes = $bytes; Map = $map }
}
$sc = Convert-Panel 'TopScoresBanner.png' 7        # banner y: the white section is y 2..23
$ov2 = Convert-Panel 'GameOverBanner.png' 5
$scoreTiles = [Math]::Max($sc.Tiles, $ov2.Tiles)

# ---- canvases drawn on at run time: the scoreboard (score digits are blitted into it) and the new top
# score widget (the initials are). 1 bit per pixel, a row of bytes per pixel row, bit 7 = leftmost, 1 = black.
function Convert-Canvas($file, $canvasW, $canvasH, $x, $y) {
  $black = New-Object 'bool[,]' $canvasW, $canvasH
  $img = Load-Bitmap (Join-Path $gfx $file)
  for ($py = 0; $py -lt $img.Height; $py++) { for ($px = 0; $px -lt $img.Width; $px++) {
    $p = $img.GetPixel($px, $py)
    if ($p.A -ge 128 -and $p.R -le 128) { $black[($x + $px), ($y + $py)] = $true }
  } }
  $img.Dispose()
  $bytes = @()
  for ($cy = 0; $cy -lt $canvasH; $cy++) { for ($cx = 0; $cx -lt $canvasW / 8; $cx++) {
    $b = 0
    for ($bit = 0; $bit -lt 8; $bit++) { if ($black[($cx * 8 + $bit), $cy]) { $b = $b -bor (0x80 -shr $bit) } }
    $bytes += $b
  } }
  return $bytes
}
$sbRows = Convert-Canvas 'scoreboard_37x9.png' 48 16 4 1          # 6x2 tiles at tile column SB_COL, scoreboard at x 4, y 1
$wgRows = Convert-Canvas 'NewTopScore_widget_112x19.png' 112 24 2 0   # 14x3 tiles, widget at x 2
$pressRows = Convert-Canvas 'PressAButton-52x16.png' 64 24 6 4      # 8x3 tiles, button at x 6, y 4: centred in the panel's lower white area

$fontInv = @()
$fnt = Load-Bitmap "$gfx\font_8x8x1.png"
for ($g = 0; $g -lt 64; $g++) {
  $t = Encode-Tile $fnt 0 ($g * 8) 'ink' 0 $fnt.Height
  $fontInv += @($t | ForEach-Object { $_ -bxor 0xFF })
}
$fnt.Dispose()

# ---- write -------------------------------------------------------------------------------------
$th = @"
/* Generated by tools/convert_assets.ps1 - do not edit. */
#ifndef TITLE_DATA_H
#define TITLE_DATA_H

#include <stdint.h>

/* All tile data lives in ROM bank TITLE_BANK: main() switches to it once and leaves it there. */
#define TITLE_BANK        1
#define TITLE_SPLIT_LINE  $($tr.SplitRow * 8)   /* screen line where the tile data addressing changes */
#define TITLE_BLACK_TILE  255                   /* solid black, valid in both addressing modes */
#define TITLE_DYN_COUNT   $dynN
#define TITLE_CURSOR_SLOT $([TitleTiles]::CursorSlot)   /* cursor sprite tiles inside the title picture's tile data */

/* menu buttons: pixel x of PLAY, SCORES and SOUND, and the y of all three. 32x16 each */
#define MENU_BTN_Y        $btnY
#define MENU_BTN_X0       $($btnX[0])
#define MENU_BTN_X1       $($btnX[1])
#define MENU_BTN_X2       $($btnX[2])
#define TITLE_ICON_ROW    $iconRow     /* the sound button: 4x2 tiles */
#define TITLE_ICON_COL    $iconCol

/* top scores panel: BG tiles SCORES_BASE.., then the white on black font */
#define SCORES_BASE       $scoresBase
#define SCORES_TILES      $($sc.Tiles)
#define OVER_TILES        $($ov2.Tiles)
#define SCORES_FONT       $($scoresBase + $scoreTiles)   /* after the larger of the two panels */

extern const unsigned char title_lo[256 * 16];     /* VRAM slots 0..255 */
extern const unsigned char title_hi[128 * 16];     /* VRAM slots 256..383 (0x8800 tiles 0..127) */
extern const unsigned char title_map[360];         /* 20x18 */
extern const unsigned char title_dyn_index[360];   /* per cell: index into title_dyn_orig, 255 = none */
extern const unsigned char title_dyn_orig[TITLE_DYN_COUNT * 16];
extern const unsigned char title_icon[2 * 8 * 16];  /* sound button: [frame][cell][8 opaque rows, 8 black rows] */
extern const unsigned char cursor_tiles[6 * 16];    /* 3 sprites of 8x16: the 20x12 cursor, 2px in from the left */
extern const unsigned char scores_tiles[SCORES_TILES * 16];
extern const unsigned char scores_map[20 * 18];     /* the shared panel with the top scores banner; tile numbers include SCORES_BASE */
extern const unsigned char over_tiles[OVER_TILES * 16];
extern const unsigned char over_map[20 * 18];       /* the shared panel with the game over banner */
extern const unsigned char sb_rows[16 * 6];         /* scoreboard canvas: 48x16 pixels, 1 bit each, 1 = black */
extern const unsigned char widget_rows[24 * 14];    /* new top score widget canvas: 112x24 pixels */
extern const unsigned char press_rows[24 * 8];     /* "press A button" canvas: 64x24 pixels */
extern const unsigned char font_inv[64 * 16];       /* ASCII 32..95, white on black */

#endif
"@
$tc = @"
/* Generated by tools/convert_assets.ps1 - do not edit. */
#include "gfx_data.h"
#include "title_data.h"

/* The tile data is here, in ROM bank 1, because bank 0 is full. main() switches to bank 1 once and
 * leaves it there. */
const unsigned char spr_tiles[] = {$(Format-Bytes $sprBytes)
};

const unsigned char bg_tiles[] = {$(Format-Bytes $bgBytes)
};

const unsigned char logo_tiles[] = {$(Format-Bytes $logoBytes)
};

const unsigned char logo_map[] = {$(Format-Bytes $logoMapC)
};

const unsigned char title_lo[256 * 16] = {$(Format-Bytes $tr.SlotsLo)
};

const unsigned char title_hi[128 * 16] = {$(Format-Bytes $tr.SlotsHi)
};

const unsigned char title_map[360] = {$(Format-Bytes $tr.Map)
};

const unsigned char title_dyn_index[360] = {$(Format-Bytes $tr.DynIndex)
};

const unsigned char title_dyn_orig[TITLE_DYN_COUNT * 16] = {$(Format-Bytes $tr.DynOrig)
};

const unsigned char title_icon[2 * 8 * 16] = {$(Format-Bytes $iconBytes)
};

const unsigned char cursor_tiles[6 * 16] = {$(Format-Bytes $cursor)
};

const unsigned char scores_tiles[SCORES_TILES * 16] = {$(Format-Bytes $sc.Bytes)
};

const unsigned char scores_map[20 * 18] = {$(Format-Bytes $sc.Map)
};

const unsigned char over_tiles[OVER_TILES * 16] = {$(Format-Bytes $ov2.Bytes)
};

const unsigned char over_map[20 * 18] = {$(Format-Bytes $ov2.Map)
};

const unsigned char sb_rows[16 * 6] = {$(Format-Bytes $sbRows)
};

const unsigned char widget_rows[24 * 14] = {$(Format-Bytes $wgRows)
};

const unsigned char press_rows[24 * 8] = {$(Format-Bytes $pressRows)
};

const unsigned char font_inv[64 * 16] = {$(Format-Bytes $fontInv)
};
"@
Set-Content -Path "$out\title_data.h" -Value $th -Encoding ascii
Set-Content -Path "$out\title_data.c" -Value $tc -Encoding ascii
"title: $($tr.Unique) tiles, merged $($tr.Merged) (worst $($tr.WorstMerge) px), split at tile row $($tr.SplitRow), $($tr.SlotsUsed) slots; panels $($sc.Tiles) and $($ov2.Tiles) tiles"
