#!/usr/bin/env python3
"""Generate 32x32 1bpp glyphs for the 1.3 inch 240x240 status screen."""

from __future__ import annotations

from pathlib import Path

CHARS = "未连接中已失败网络可录音完成转写 0123456789.:-"
ROWS = 32
COLS = 32
FONT_CANDIDATES = [
    Path(r"C:\Windows\Fonts\msyhbd.ttc"),
    Path(r"C:\Windows\Fonts\msyh.ttc"),
    Path(r"C:\Windows\Fonts\simhei.ttf"),
    Path(r"C:\Windows\Fonts\simsun.ttc"),
]


def render_with_pil(ch: str) -> list[int]:
    from PIL import Image, ImageDraw, ImageFont

    font_path = next(path for path in FONT_CANDIDATES if path.exists())
    font = ImageFont.truetype(str(font_path), 28, index=0)
    image = Image.new("L", (COLS, ROWS), 0)
    draw = ImageDraw.Draw(image)
    draw.text((COLS // 2, ROWS // 2 - 1), ch, font=font, fill=255, anchor="mm")
    bits: list[int] = []
    for y in range(ROWS):
        row = 0
        for x in range(COLS):
            if image.getpixel((x, y)) >= 96:
                row |= 1 << (31 - x)
        bits.append(row)
    return bits


def render_with_gdi(ch: str) -> list[int]:
    import ctypes
    from ctypes import wintypes

    gdi32 = ctypes.windll.gdi32
    user32 = ctypes.windll.user32
    hwnd_desktop = user32.GetDesktopWindow()
    hdc_screen = user32.GetDC(hwnd_desktop)
    hdc = gdi32.CreateCompatibleDC(hdc_screen)
    bmp = gdi32.CreateCompatibleBitmap(hdc_screen, COLS, ROWS)
    gdi32.SelectObject(hdc, bmp)
    gdi32.PatBlt(hdc, 0, 0, COLS, ROWS, 0x00000042)
    face = next((path.stem for path in FONT_CANDIDATES if path.exists()), "Microsoft YaHei")
    if face.lower().startswith("msyhbd"):
        face = "Microsoft YaHei"
    elif face.lower().startswith("msyh"):
        face = "Microsoft YaHei"
    elif face.lower().startswith("simhei"):
        face = "SimHei"
    elif face.lower().startswith("simsun"):
        face = "SimSun"
    font = gdi32.CreateFontW(28, 0, 0, 0, 700, 0, 0, 0, 1, 0, 0, 3, 0, face)
    gdi32.SelectObject(hdc, font)
    gdi32.SetBkMode(hdc, 1)
    gdi32.SetTextColor(hdc, 0x00FFFFFF)

    class RECT(ctypes.Structure):
        _fields_ = [
            ("left", wintypes.LONG),
            ("top", wintypes.LONG),
            ("right", wintypes.LONG),
            ("bottom", wintypes.LONG),
        ]

    rect = RECT(0, 0, COLS, ROWS)
    user32.DrawTextW(hdc, ch, 1, ctypes.byref(rect), 0x00000025)
    bits: list[int] = []
    for y in range(ROWS):
        row = 0
        for x in range(COLS):
            if gdi32.GetPixel(hdc, x, y) != 0:
                row |= 1 << (31 - x)
        bits.append(row)
    gdi32.DeleteObject(font)
    gdi32.DeleteObject(bmp)
    gdi32.DeleteDC(hdc)
    user32.ReleaseDC(hwnd_desktop, hdc_screen)
    return bits


def render_glyph(ch: str) -> list[int]:
    try:
        return render_with_pil(ch)
    except Exception:
        return render_with_gdi(ch)


def main() -> None:
    dest = Path(__file__).resolve().parents[1] / "main" / "display" / "font.c"
    header = Path(__file__).resolve().parents[1] / "main" / "display" / "font.h"
    glyphs = [(ord(ch), render_glyph(ch)) for ch in CHARS]
    if any(sum(rows) == 0 and ch != " " for ch, rows in zip(CHARS, (g[1] for g in glyphs))):
        raise SystemExit("generated an empty non-space glyph")

    header.write_text(
        "#pragma once\n\n"
        "#include <stdbool.h>\n"
        "#include <stdint.h>\n\n"
        "enum { VENTURED_FONT_ROWS = 32, VENTURED_FONT_COLS = 32 };\n\n"
        "bool ventured_font_bits(uint32_t codepoint, const uint32_t **rows);\n",
        encoding="utf-8",
    )

    lines = [
        '#include "font.h"',
        "",
        "#include <stddef.h>",
        "",
        "typedef struct {",
        "    uint32_t codepoint;",
        "    uint32_t rows[VENTURED_FONT_ROWS];",
        "} ventured_font_glyph_t;",
        "",
        "static const ventured_font_glyph_t s_glyphs[] = {",
    ]
    for codepoint, rows in glyphs:
        packed = ", ".join(f"0x{row:08X}" for row in rows)
        lines.append(f"    {{ 0x{codepoint:04X}, {{ {packed} }} }},")
    lines.extend(
        [
            "};",
            "",
            "bool ventured_font_bits(uint32_t codepoint, const uint32_t **rows) {",
            "    if (rows == NULL) return false;",
            "    for (size_t i = 0; i < sizeof(s_glyphs) / sizeof(s_glyphs[0]); ++i) {",
            "        if (s_glyphs[i].codepoint == codepoint) {",
            "            *rows = s_glyphs[i].rows;",
            "            return true;",
            "        }",
            "    }",
            "    return false;",
            "}",
            "",
        ]
    )
    dest.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {dest} ({len(glyphs)} glyphs)")


if __name__ == "__main__":
    main()
