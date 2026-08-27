#!/usr/bin/env python3
"""Generate 16x16 1bpp glyphs for the Board C status screen."""

from __future__ import annotations

import ctypes
from ctypes import wintypes
from pathlib import Path

CHARS = "未连接中已失败网络 0123456789.:-"

ROWS = 16
COLS = 16


def render_glyph(ch: str) -> list[int]:
    gdi32 = ctypes.windll.gdi32
    user32 = ctypes.windll.user32

    hwnd_desktop = user32.GetDesktopWindow()
    hdc_screen = user32.GetDC(hwnd_desktop)
    hdc = gdi32.CreateCompatibleDC(hdc_screen)
    bmp = gdi32.CreateCompatibleBitmap(hdc_screen, COLS, ROWS)
    gdi32.SelectObject(hdc, bmp)
    gdi32.PatBlt(hdc, 0, 0, COLS, ROWS, 0x00000042)  # BLACKNESS

    font = gdi32.CreateFontW(
        16,
        0,
        0,
        0,
        700,
        0,
        0,
        0,
        1,  # DEFAULT_CHARSET
        0,
        0,
        5,  # CLEARTYPE_QUALITY
        0,
        "Microsoft YaHei",
    )
    gdi32.SelectObject(hdc, font)
    gdi32.SetBkMode(hdc, 1)  # TRANSPARENT
    gdi32.SetTextColor(hdc, 0x00FFFFFF)

    class RECT(ctypes.Structure):
        _fields_ = [
            ("left", wintypes.LONG),
            ("top", wintypes.LONG),
            ("right", wintypes.LONG),
            ("bottom", wintypes.LONG),
        ]

    rect = RECT(0, 0, COLS, ROWS)
    user32.DrawTextW(hdc, ch, 1, ctypes.byref(rect), 0x00000025)  # CENTER|VCENTER|SINGLELINE

    bits: list[int] = []
    for y in range(ROWS):
        row = 0
        for x in range(COLS):
            color = gdi32.GetPixel(hdc, x, y)
            if color != 0:
                row |= 1 << (15 - x)
        bits.append(row)

    gdi32.DeleteObject(font)
    gdi32.DeleteObject(bmp)
    gdi32.DeleteDC(hdc)
    user32.ReleaseDC(hwnd_desktop, hdc_screen)
    return bits


def main() -> None:
    dest = Path(__file__).resolve().parents[1] / "main" / "display" / "font16.c"
    header = Path(__file__).resolve().parents[1] / "main" / "display" / "font16.h"
    glyphs: list[tuple[int, list[int]]] = []
    for ch in CHARS:
        glyphs.append((ord(ch), render_glyph(ch)))

    header.write_text(
        "#pragma once\n\n"
        "#include <stdbool.h>\n"
        "#include <stdint.h>\n\n"
        "enum { VENTURED_FONT16_ROWS = 16, VENTURED_FONT16_COLS = 16 };\n\n"
        "bool ventured_font16_bits(uint32_t codepoint, const uint16_t **rows);\n",
        encoding="utf-8",
    )

    lines = [
        '#include "font16.h"',
        "",
        "#include <stddef.h>",
        "",
        "typedef struct {",
        "    uint32_t codepoint;",
        "    uint16_t rows[VENTURED_FONT16_ROWS];",
        "} ventured_font16_glyph_t;",
        "",
        "static const ventured_font16_glyph_t s_glyphs[] = {",
    ]
    for codepoint, rows in glyphs:
        packed = ", ".join(f"0x{row:04X}" for row in rows)
        lines.append(f"    {{ 0x{codepoint:04X}, {{ {packed} }} }},")
    lines.extend(
        [
            "};",
            "",
            "bool ventured_font16_bits(uint32_t codepoint, const uint16_t **rows) {",
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
