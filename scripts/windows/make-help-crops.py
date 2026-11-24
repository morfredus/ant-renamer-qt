#!/usr/bin/env python3
"""Cuts and saves the help pictures from the window shots (used by make-help-screenshots.sh).

Usage: make-help-crops.py SHOTS_FOLDER OUTPUT_FOLDER   (needs Pillow and numpy)

- The four toolbar strips are cut out of the wide classic window shots (strip_*) at the place of the toolbars (x = 3, 25 or 29
  pixels high): tabs_* from strip_files_* (first row), filestoolbar_* from strip_files_*, batchtoolbar_* from strip_actions_*,
  logtoolbar_* from strip_log_* (second row). Each strip ends right after its last button.
- Pictures that were 256-colour PNG files (small ones, the plain pages) stay so; the others stay in full colour.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image

shots, out = Path(sys.argv[1]), Path(sys.argv[2])
PALETTE = {'files_page', 'log_page', 'modern_tabs', 'options_files', 'options_processing', 'options_events'}


def save(img, name):
    img = img.convert('RGB')
    stem = name.rsplit('_', 1)[0]
    if stem in PALETTE or name.split('_')[0] in ('tabs', 'filestoolbar', 'batchtoolbar', 'logtoolbar'):
        img = img.quantize(256, dither=Image.Dither.NONE)
    img.save(out / (name + '.png'), optimize=True)


def strip(src, y0, h, name):
    """The row of toolbars [y0, y0 + h): from x = 3 to the right edge of the last button (the background is the colour of the
    right edge of the window)."""
    a = np.asarray(src.convert('RGB')).astype(int)
    band = a[y0:y0 + h]
    bg = a[y0 + h // 2, a.shape[1] - 1]
    cols = np.where((np.abs(band - bg).sum(axis=2) > 30).any(axis=0))[0]
    save(src.crop((3, y0, int(cols.max()) + 1, y0 + h)), name)


for lang in ('en', 'fr'):
    for name in ('files_page', 'actions_page', 'log_page', 'modern_tabs', 'modern_bands', 'modern_columns', 'options_display',
                 'options_files', 'options_processing', 'options_events', 'options_language', 'filter_dialog', 'setdates_dialog'):
        save(Image.open(shots / f'{name}_{lang}.png'), f'{name}_{lang}')
    files = Image.open(shots / f'strip_files_{lang}.png')
    strip(files, 1, 29, f'tabs_{lang}')
    strip(files, 34, 25, f'filestoolbar_{lang}')
    strip(Image.open(shots / f'strip_actions_{lang}.png'), 34, 25, f'batchtoolbar_{lang}')
    strip(Image.open(shots / f'strip_log_{lang}.png'), 34, 25, f'logtoolbar_{lang}')
print('crops done')
