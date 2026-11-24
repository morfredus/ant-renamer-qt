#!/usr/bin/env python3
"""Sample photos for the "Set dates from EXIF" screenshot (used by make-screenshots.sh).

Usage: make-sample-dates.py FOLDER

Five JPEG files with an EXIF shooting date (23 and 24 October 2026), each with a RAW file and an .xmp sidecar, and a text
file with no EXIF date. Every file carries the date it was "copied" (2 November 2026, 18:30), so the dialog shows an old and
a new date for each picture. The pictures hold nothing but the EXIF block (enough for the program).
"""
import os
import struct
import sys
import time


def jpeg(path, when):
    u16 = lambda v: struct.pack('<H', v)
    u32 = lambda v: struct.pack('<I', v)

    def entry(tag, typ, count, val):
        return u16(tag) + u16(typ) + u32(count) + (val + b'\0' * 4)[:4]

    date = when.encode() + b'\0'
    ifd0 = u16(1) + entry(0x8769, 4, 1, u32(26)) + u32(0)
    exif = u16(1) + entry(0x9003, 2, len(date), u32(44)) + u32(0)
    payload = b'Exif\0\0' + b'II*\0' + u32(8) + ifd0 + exif + date
    n = len(payload) + 2
    with open(path, 'wb') as f:
        f.write(b'\xff\xd8\xff\xe1' + bytes([n >> 8, n & 255]) + payload + b'\xff\xda\x00\x02' + b'x')


folder = sys.argv[1]
os.makedirs(folder, exist_ok=True)
copied = time.mktime((2026, 11, 2, 18, 30, 0, 0, 0, -1))
shots = ['2026:10:23 08:17:35', '2026:10:23 08:21:02', '2026:10:23 14:05:40', '2026:10:24 09:12:11', '2026:10:24 16:48:27']
for i, when in enumerate(shots):
    name = 'IMG_%04d' % (4012 + i)
    jpeg(os.path.join(folder, name + '.JPG'), when)
    open(os.path.join(folder, name + '.CR2'), 'wb').close()
    open(os.path.join(folder, name + '.JPG.xmp'), 'wb').close()
    for ext in ('.JPG', '.CR2', '.JPG.xmp'):
        os.utime(os.path.join(folder, name + ext), (copied, copied))
with open(os.path.join(folder, 'notes.txt'), 'w') as f:
    f.write('notes')
os.utime(os.path.join(folder, 'notes.txt'), (copied, copied))
