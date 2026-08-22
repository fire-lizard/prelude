#!/usr/bin/env python3
r"""Prelude parchment font sheet generator - recolour the glyph sheet.

The game has no runtime text colouring: TEXT_COLOR_T in Source/ZSFontEngine.h
is only an index into ZSFont::lpFontSurf[], which ZSFontEngine.cpp fills in file
order, so TEXT_RED_PARCHMENT is red purely because slot 6 loads fontredparch.bmp.
Adding a colour means appending to that enum, bumping NUM_FONT_COLORS, adding the
lpFontSurf[n] line, and dropping a .bmp next to the exe - this makes the .bmp.

    fontgen.py "C:\Users\...\Desktop\Prelude to Darkness"
    fontgen.py --selftest "C:\...\Prelude to Darkness"    checks, writes nothing

Three traps, all of which produce a sheet that looks right and renders wrong:

  * Transparency is not hardcoded. ZSFontEngine::LoadFont reads the single pixel
    at SurfPtr[SurfWidth*2 - 2] and makes that value the colour key, so a sheet
    declares its own - cream for the parchment sheets, magenta for fontwhite.bmp.
    Every cream pixel here is copied verbatim, which keeps that pixel intact.
  * The glyphs are not one ink over cream. There is a second, hand-picked outline
    anchor, and the anti-aliased pixels carry subpixel colour fringes from the
    original 2005 render. Recolouring by a scalar cream->ink lerp misses the
    shipped sheets by up to 30 per channel. What works, and what this does, is to
    project onto whichever ramp fits, recomposite, and add the residual back
    unchanged - feeding the master's own anchors back in is then byte-exact.
  * fontredparch.bmp and fontgreenparch.bmp share a pixel-identical non-cream
    mask, but the grey sheets differ by ~396 pixels. Only the red/green pair is a
    true recolour of one render, so red is the master.

An ink with a channel at 0 (purple's green) cannot represent the fringes that
want to go below it; those channels clamp. The count is reported - a few dozen
out of ~17600 is invisible at 14px, a few thousand would mean picking a new ink.
"""
import collections
import os
import struct
import sys

MASTER  = 'fontredparch.bmp'
BG      = (0xaf, 0xe4, 0xf9)     # BGR bytes - parchment cream, and the colour key
CORE    = (0x5f, 0x7d, 0xa9)     # master glyph core,    RGB(169,125,95) luma 135
OUTLINE = (0x41, 0x5c, 0x8d)     # master glyph outline, RGB(141, 92,65) luma 104

# Sheets to generate: filename -> (core, outline) as BGR byte tuples.
# The family runs luma ~128 for the core and ~106 for the outline
# (fontlightgreyparch.bmp is literally grey 128 and 106); blue keeps to that.
# Purple's core is a caller-specified RGB(127,0,127), darker than the rest of the
# family at luma 52, with an outline holding the family's ~0.82 core:outline ratio.
SHEETS = {
    'fontblueparch.bmp':   ((235, 135, 75), (205, 112, 58)),
    'fontpurpleparch.bmp': ((127, 0, 127), (104, 0, 104)),
}


def load(path):
    d = bytearray(open(path, 'rb').read())
    off, = struct.unpack_from('<I', d, 10)
    w, h = struct.unpack_from('<ii', d, 18)
    bpp, = struct.unpack_from('<H', d, 28)
    assert bpp == 32, '%s is %d bpp, expected 32' % (path, bpp)
    return d, off, w, h


def project(px, anchor):
    """Least-squares weight of px on the ramp BG -> anchor, and its residual."""
    num = den = 0
    for c in range(3):
        dv = anchor[c] - BG[c]
        num += (px[c] - BG[c]) * dv
        den += dv * dv
    a = num / den
    return a, [px[c] - (BG[c] + a * (anchor[c] - BG[c])) for c in range(3)]


def recolour(master, core, outline):
    """Returns (new file bytes, count of channels that had to clamp)."""
    d, off, w, h = master
    out = bytearray(d)
    clamped = 0
    for p in range(w * h):
        i = off + p * 4
        px = (d[i], d[i + 1], d[i + 2])
        if px == BG:                      # verbatim -> colour key survives
            continue
        a1, r1 = project(px, CORE)
        a2, r2 = project(px, OUTLINE)
        a, res, dst = ((a1, r1, core) if max(map(abs, r1)) <= max(map(abs, r2))
                       else (a2, r2, outline))
        for c in range(3):
            v = round(BG[c] + a * (dst[c] - BG[c]) + res[c])
            if not 0 <= v <= 255:
                clamped += 1
                v = max(0, min(255, v))
            out[i + c] = v
    return out, clamped


def describe(sheet, off, w, h):
    c = collections.Counter(bytes(sheet[off + p * 4:off + p * 4 + 3]).hex()
                            for p in range(w * h))
    for x, n in c.most_common(3):
        b, g, r = (int(x[i:i + 2], 16) for i in (0, 2, 4))
        yield '   %s  RGB(%d,%d,%d) luma %3d  n=%d' % (
            x, r, g, b, round(.299 * r + .587 * g + .114 * b), n)


def main(argv):
    gamedir = argv[0] if argv else '.'
    path = os.path.join(gamedir, MASTER)
    if not os.path.exists(path):
        sys.exit('no %s in %s - point me at the game directory' % (MASTER, gamedir))
    master = load(path)
    d, off, w, h = master
    key = off + (w * 2 - 2) * 4
    nonbg = sum(1 for p in range(w * h)
                if tuple(d[off + p * 4:off + p * 4 + 3]) != BG)

    same, _ = recolour(master, CORE, OUTLINE)
    assert same == d, 'identity recolour is not lossless'
    print('identity check: recolour(master anchors) == %s' % MASTER)

    for name, (core, outline) in SHEETS.items():
        sheet, clamped = recolour(master, core, outline)
        assert sheet[key:key + 3] == d[key:key + 3], 'colour key changed in ' + name
        assert len(sheet) == len(d)
        if selftest:
            print('%s  ok, %d/%d channels would clamp' % (name, clamped, nonbg * 3))
            continue
        open(os.path.join(gamedir, name), 'wb').write(sheet)
        print('wrote %s  %dx%d  %d bytes, key %s kept, %d/%d channels clamped'
              % (name, w, h, len(sheet), d[key:key + 3].hex(), clamped, nonbg * 3))
        for line in describe(sheet, off, w, h):
            print(line)


if __name__ == '__main__':
    argv = [a for a in sys.argv[1:] if a != '--selftest']
    selftest = '--selftest' in sys.argv
    main(argv)
