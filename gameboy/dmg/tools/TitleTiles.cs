// Packs the 160x144 title screen into Game Boy background tiles.
//
// A full screen of detailed art has about 360 different 8x8 tiles, more than the 256 a
// background map can address. The way out is the LCDC "tile data" bit: the top of the screen
// uses the 0x8000 addressing (tile n = VRAM slot n) and the bottom the 0x8800 addressing
// (tile n < 128 = slot 256 + n, tile n >= 128 = slot n). Together they reach 384 slots.
//
//   slots   0..127   top of the screen only
//   slots 128..247   either (tiles shared by top and bottom, or overflow of either half)
//   slots 248..254   reserved: the menu cursor sprite
//   slot  255        solid black
//   slots 256..383   bottom of the screen only
//
// Cells the game rewrites at run time ("dynamic" cells) get a slot of their own. If everything
// does not fit, the closest pairs of tiles are merged (a few pixels change) until it does.

using System;
using System.Collections.Generic;
using System.Drawing;

public class Overlay
{
    public string Path;     // png with frames stacked vertically
    public int FrameY;      // first row of the frame in the png
    public int W, H;        // frame size
    public int X, Y;        // where the frame goes on the 160x144 screen
    public Overlay(string path, int frameY, int w, int h, int x, int y)
    { Path = path; FrameY = frameY; W = w; H = h; X = x; Y = y; }
}

public class TitleResult
{
    public byte[] SlotsLo;      // slots 0..255, 16 bytes each (2bpp tile data)
    public byte[] SlotsHi;      // slots 256..383
    public byte[] Map;          // 20x18 tile numbers as the display sees them
    public byte[] DynIndex;     // 20x18, index into DynOrig or 255
    public byte[] DynOrig;      // original tile data of each dynamic cell, 16 bytes each
    public int SplitRow;        // first screen tile row that uses the 0x8800 addressing
    public int Unique;          // distinct tiles before merging
    public int Merged;          // tiles merged away
    public int WorstMerge;      // largest pixel difference of a merge
    public int SlotsUsed;
}

public static class TitleTiles
{
    const int W = 20, H = 18;
    public const int SharedLimit = 248;      // first reserved slot
    public const int CursorSlot = 248;       // 6 slots: 3 sprites of 8x16

    static int Pop(ulong v) { int c = 0; while (v != 0) { v &= v - 1; c++; } return c; }

    static bool White(Color c) { return c.A >= 128 && c.R > 128; }

    static ulong[] LoadCells(string titlePng, string bannerPng, List<Overlay> overlays)
    {
        Bitmap t = new Bitmap(titlePng);
        if (t.Width != 160 || t.Height != 144) throw new Exception("title screen must be 160x144");
        bool[,] black = new bool[160, 144];
        for (int y = 0; y < 144; y++) for (int x = 0; x < 160; x++) black[x, y] = !White(t.GetPixel(x, y));
        t.Dispose();

        if (bannerPng != null)
        {
            Bitmap b = new Bitmap(bannerPng);
            if (b.Width != 160 || b.Height != 8) throw new Exception("banner must be 160x8");
            for (int y = 0; y < 8; y++) for (int x = 0; x < 160; x++) black[x, 136 + y] = !White(b.GetPixel(x, y));
            b.Dispose();
        }

        foreach (Overlay o in overlays)
        {
            Bitmap ob = new Bitmap(o.Path);
            for (int y = 0; y < o.H; y++)
                for (int x = 0; x < o.W; x++)
                {
                    Color c = ob.GetPixel(x, o.FrameY + y);
                    if (c.A < 128) continue;                               // transparent: keep the picture
                    black[o.X + x, o.Y + y] = !White(c);
                }
            ob.Dispose();
        }

        ulong[] cells = new ulong[W * H];
        for (int ty = 0; ty < H; ty++)
            for (int tx = 0; tx < W; tx++)
            {
                ulong v = 0;
                for (int y = 0; y < 8; y++)
                    for (int x = 0; x < 8; x++)
                        if (black[tx * 8 + x, ty * 8 + y]) v |= 1UL << (y * 8 + x);
                cells[ty * W + tx] = v;
            }
        return cells;
    }

    static void TileBytes(ulong v, byte[] dst, int off)
    {
        for (int y = 0; y < 8; y++)
        {
            int row = 0;
            for (int x = 0; x < 8; x++) if ((v >> (y * 8 + x) & 1) != 0) row |= 0x80 >> x;
            dst[off + y * 2] = (byte)row;
            dst[off + y * 2 + 1] = (byte)row;
        }
    }

    // Sprite tiles for a 20x12 cursor: 3 sprites of 8x16, each a top tile then a bottom tile.
    // Index 0 transparent, 1 white (low plane only), 3 black.
    public static byte[] CursorTiles(string png)
    {
        Bitmap b = new Bitmap(png);
        byte[] r = new byte[6 * 16];
        for (int p = 0; p < 3; p++)
            for (int half = 0; half < 2; half++)
                for (int y = 0; y < 8; y++)
                {
                    int lo = 0, hi = 0;
                    for (int x = 0; x < 8; x++)
                    {
                        int px = p * 8 + x - 2, py = half * 8 + y;          // cursor sits 2px in from the left
                        if (px < 0 || px >= b.Width || py >= b.Height) continue;
                        Color c = b.GetPixel(px, py);
                        if (c.A < 128) continue;
                        lo |= 0x80 >> x;
                        if (!White(c)) hi |= 0x80 >> x;
                    }
                    r[(p * 2 + half) * 16 + y * 2] = (byte)lo;
                    r[(p * 2 + half) * 16 + y * 2 + 1] = (byte)hi;
                }
        b.Dispose();
        return r;
    }

    // capacity check for one split row; returns slack (>= 0 fits)
    static int Slack(ulong[] cells, bool[] dyn, int split)
    {
        HashSet<ulong> top = new HashSet<ulong>(), bot = new HashSet<ulong>();
        int dynCount = 0;
        for (int i = 0; i < cells.Length; i++)
        {
            int row = i / W;
            if (dyn[i]) { if (row < split) return -1000; dynCount++; continue; }
            if (row < split) top.Add(cells[i]); else bot.Add(cells[i]);
        }
        int shared = 0;
        foreach (ulong v in top) if (bot.Contains(v)) shared++;
        int topOnly = top.Count - shared, botOnly = bot.Count - shared + dynCount;
        int room = SharedLimit - 128;
        int over = shared + Math.Max(0, topOnly - 128) + Math.Max(0, botOnly - 128);
        if (shared > room) return -1000;
        return room - over;
    }

    static int BestSplit(ulong[] cells, bool[] dyn, out int slack)
    {
        int best = -1; slack = -1000;
        for (int s = 4; s <= 14; s++)
        {
            int k = Slack(cells, dyn, s);
            if (k > slack) { slack = k; best = s; }
        }
        return best;
    }

    public static TitleResult Build(string titlePng, string bannerPng, List<Overlay> overlays, bool[] dyn, byte[] cursor)
    {
        ulong[] cells = LoadCells(titlePng, bannerPng, overlays);
        TitleResult r = new TitleResult();

        ulong[] dynOrig = new ulong[cells.Length];
        for (int i = 0; i < cells.Length; i++) dynOrig[i] = cells[i];

        HashSet<ulong> all = new HashSet<ulong>();
        for (int i = 0; i < cells.Length; i++) if (!dyn[i]) all.Add(cells[i]);
        r.Unique = all.Count;

        int slack, split = BestSplit(cells, dyn, out slack);
        while (slack < 0)
        {
            Dictionary<ulong, int> use = new Dictionary<ulong, int>();
            for (int i = 0; i < cells.Length; i++)
                if (!dyn[i]) { int n; use.TryGetValue(cells[i], out n); use[cells[i]] = n + 1; }
            List<ulong> tiles = new List<ulong>(use.Keys);
            int bi = -1, bj = -1, bd = 99;
            for (int i = 0; i < tiles.Count; i++)
                for (int j = i + 1; j < tiles.Count; j++)
                {
                    int d = Pop(tiles[i] ^ tiles[j]);
                    if (d < bd) { bd = d; bi = i; bj = j; }
                }
            if (bi < 0 || bd > 16) throw new Exception("title screen does not fit in 384 tiles, simplify it (needs " + (-slack) + " more)");
            ulong keep = use[tiles[bi]] >= use[tiles[bj]] ? tiles[bi] : tiles[bj];
            ulong drop = keep == tiles[bi] ? tiles[bj] : tiles[bi];
            for (int i = 0; i < cells.Length; i++) if (!dyn[i] && cells[i] == drop) cells[i] = keep;
            r.Merged++;
            if (bd > r.WorstMerge) r.WorstMerge = bd;
            split = BestSplit(cells, dyn, out slack);
        }
        r.SplitRow = split;

        Dictionary<ulong, int> slotOf = new Dictionary<ulong, int>();
        HashSet<ulong> topSet = new HashSet<ulong>(), botSet = new HashSet<ulong>();
        List<ulong> topOrder = new List<ulong>(), botOrder = new List<ulong>();
        List<int> dynCells = new List<int>();
        for (int i = 0; i < cells.Length; i++)
        {
            int row = i / W;
            if (dyn[i]) { dynCells.Add(i); continue; }
            if (row < split) { if (topSet.Add(cells[i])) topOrder.Add(cells[i]); }
            else { if (botSet.Add(cells[i])) botOrder.Add(cells[i]); }
        }
        List<ulong> shared = new List<ulong>(), topOnly = new List<ulong>(), botOnly = new List<ulong>();
        foreach (ulong v in topOrder) { if (botSet.Contains(v)) shared.Add(v); else topOnly.Add(v); }
        foreach (ulong v in botOrder) if (!topSet.Contains(v)) botOnly.Add(v);

        int next128 = 128;
        foreach (ulong v in shared) slotOf[v] = next128++;
        int lowNext = 0;
        foreach (ulong v in topOnly) slotOf[v] = lowNext < 128 ? lowNext++ : next128++;
        int highNext = 256;
        foreach (ulong v in botOnly) slotOf[v] = highNext < 384 ? highNext++ : next128++;
        int[] dynSlot = new int[cells.Length];
        foreach (int c in dynCells) dynSlot[c] = highNext < 384 ? highNext++ : next128++;
        if (next128 > SharedLimit) throw new Exception("internal: slot overflow");
        r.SlotsUsed = Math.Max(256, highNext);

        r.SlotsLo = new byte[256 * 16];
        r.SlotsHi = new byte[128 * 16];
        foreach (KeyValuePair<ulong, int> kv in slotOf)
        {
            if (kv.Value < 256) TileBytes(kv.Key, r.SlotsLo, kv.Value * 16);
            else TileBytes(kv.Key, r.SlotsHi, (kv.Value - 256) * 16);
        }
        TileBytes(ulong.MaxValue, r.SlotsLo, 255 * 16);                 // slot 255: solid black
        Array.Copy(cursor, 0, r.SlotsLo, CursorSlot * 16, cursor.Length);

        r.Map = new byte[cells.Length];
        r.DynIndex = new byte[cells.Length];
        List<byte> orig = new List<byte>();
        int dynN = 0;
        for (int i = 0; i < cells.Length; i++)
        {
            int row = i / W;
            r.DynIndex[i] = 255;
            int slot;
            if (dyn[i])
            {
                slot = dynSlot[i];
                byte[] t = new byte[16];
                TileBytes(dynOrig[i], t, 0);
                if (slot < 256) TileBytes(dynOrig[i], r.SlotsLo, slot * 16);
                else TileBytes(dynOrig[i], r.SlotsHi, (slot - 256) * 16);
                orig.AddRange(t);
                r.DynIndex[i] = (byte)dynN++;
            }
            else slot = slotOf[cells[i]];
            if (row < split) r.Map[i] = (byte)slot;
            else r.Map[i] = (byte)(slot >= 256 ? slot - 256 : slot);
        }
        r.DynOrig = orig.ToArray();
        return r;
    }
}
