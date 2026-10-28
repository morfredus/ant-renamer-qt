#!/usr/bin/env python3
"""Generates a test folder for Ant Renamer Qt.

Usage: make_test_folder.py [DESTINATION]   (default: ../ant-renamer-test-data)

Content: pictures with EXIF (JPEG/TIFF/PNG/WebP), audio with ID3 tags, documents,
files without extension, accented names and spaces, hidden files and folders,
nested subfolders, and a folder of 250 files.
Names and texts of the test files are in French on purpose (accents).
Requires Pillow and exiftool.
"""
import math
import random
import shutil
import struct
import subprocess
import sys
import wave
import zipfile
from pathlib import Path

from PIL import Image, ImageDraw

dest = Path(sys.argv[1] if len(sys.argv) > 1 else
            Path(__file__).resolve().parents[3] / "ant-renamer-test-data")
if dest.exists():
    shutil.rmtree(dest)
dest.mkdir(parents=True)
random.seed(42)


def put(rel, data=b""):
    p = dest / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    if isinstance(data, str):
        p.write_text(data, encoding="utf-8")
    else:
        p.write_bytes(data)
    return p


def image(rel, size, color, fmt, label=""):
    p = dest / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    img = Image.new("RGB", size, color)
    d = ImageDraw.Draw(img)
    d.rectangle([10, 10, size[0] - 10, size[1] - 10], outline="white", width=4)
    d.text((24, 24), label or p.name, fill="white")
    img.save(p, fmt)
    return p


def exif(path, **tags):
    args = ["exiftool", "-overwrite_original", "-q"]
    args += [f"-{k}={v}" for k, v in tags.items()]
    subprocess.run(args + [str(path)], check=True)


CAMERAS = [
    ("Canon", "Canon EOS R6", "RF24-105mm F4 L IS USM"),
    ("NIKON CORPORATION", "NIKON Z 6_2", "NIKKOR Z 50mm f/1.8 S"),
    ("SONY", "ILCE-7M3", "FE 35mm F1.8"),
    ("Apple", "iPhone 14 Pro", "iPhone 14 Pro back camera 6.86mm f/1.78"),
    ("FUJIFILM", "X-T4", "XF23mmF2 R WR"),
]


def photo(rel, n, fmt="JPEG"):
    make, model, lens = CAMERAS[n % len(CAMERAS)]
    hue = (n * 47) % 255
    p = image(rel, (640, 480), (hue, 90 + n * 13 % 120, 255 - hue), fmt)
    exif(p,
         Make=make, Model=model, LensModel=lens,
         DateTimeOriginal=f"2024:0{1 + n % 9}:{10 + n:02d} 1{n % 10}:3{n % 6}:15",
         CreateDate=f"2024:0{1 + n % 9}:{10 + n:02d} 1{n % 10}:3{n % 6}:15",
         ISO=100 * (1 + n % 8), FNumber=[1.8, 2.8, 4, 5.6, 8][n % 5],
         ExposureTime=f"1/{125 * (1 + n % 4)}", FocalLength=[24, 35, 50, 85][n % 4],
         Artist="Frédéric Test", Copyright="(c) 2024 Test",
         ImageDescription=f"Photo de test numéro {n}",
         GPSLatitude=48.8566 + n * 0.01, GPSLatitudeRef="N",
         GPSLongitude=2.3522 + n * 0.01, GPSLongitudeRef="E")
    return p


# ---------- 01 Photos (EXIF) ----------
for i in range(1, 9):
    photo(f"01_photos_exif/IMG_{1000 + i}.jpg", i)
photo("01_photos_exif/DSC_0042.JPG", 3)                  # uppercase extension
photo("01_photos_exif/Vacances été 2024 (1).jpeg", 4)    # accents, spaces, parentheses
for i, ext in enumerate(["tif", "png", "webp"], 20):
    fmt = {"tif": "TIFF", "png": "PNG", "webp": "WEBP"}[ext]
    p = image(f"01_photos_exif/photo_{ext}.{ext}", (320, 240), (30 * i % 255, 120, 200), fmt)
    exif(p, Make="Canon", Model="Canon EOS 5D", DateTimeOriginal="2023:07:14 09:05:00")
# JPEG without EXIF
image("01_photos_exif/sans_exif.jpg", (320, 240), (60, 60, 60), "JPEG")
Image.new("RGB", (64, 64), "red").save(dest / "01_photos_exif/icone.gif")
Image.new("RGB", (64, 64), "blue").save(dest / "01_photos_exif/fond.bmp")

# ---------- 02 Audio (ID3 / metadata) ----------
def wav(rel, secs=1, freq=440):
    p = dest / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(p), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(8000)
        w.writeframes(b"".join(
            struct.pack("<h", int(8000 * math.sin(2 * math.pi * freq * t / 8000)))
            for t in range(8000 * secs)))
    return p


def synsyncsafe(n):
    return bytes([(n >> 21) & 0x7F, (n >> 14) & 0x7F, (n >> 7) & 0x7F, n & 0x7F])


def mp3(rel, title, artist, album, track, year):
    def frame(fid, text):
        body = b"\x03" + text.encode("utf-8")  # UTF-8
        return fid.encode() + struct.pack(">I", len(body)) + b"\x00\x00" + body
    frames = (frame("TIT2", title) + frame("TPE1", artist) + frame("TALB", album)
              + frame("TRCK", str(track)) + frame("TYER", str(year)) + frame("TCON", "Rock"))
    tag = b"ID3\x04\x00\x00" + synsyncsafe(len(frames)) + frames
    # MPEG1 L3 frame, 128 kbps 44.1 kHz (417 bytes), repeated
    mpeg = (b"\xff\xfb\x90\x00" + b"\x00" * 413) * 10
    put(rel, tag + mpeg)


mp3("02_audio/01 - Intro.mp3", "Intro", "Les Testeurs", "Album de Test", 1, 2021)
mp3("02_audio/02 - Été indien.mp3", "Été indien", "Les Testeurs", "Album de Test", 2, 2021)
mp3("02_audio/03 - Ça va.mp3", "Ça va ?", "Les Testeurs", "Album de Test", 3, 2021)
mp3("02_audio/piste_sans_nom.mp3", "Inconnu", "Artiste Inconnu", "Single", 1, 2019)
put("02_audio/sans_tag.mp3", (b"\xff\xfb\x90\x00" + b"\x00" * 413) * 10)
wav("02_audio/bip_440.wav", 1, 440)
wav("02_audio/bip_880.wav", 1, 880)

# ---------- 03 Documents ----------
put("03_documents/lisez-moi.txt", "Fichier texte de test.\nDeuxième ligne.\n")
put("03_documents/notes.md", "# Notes\n\n- un\n- deux\n")
put("03_documents/données.csv", "id;nom;valeur\n1;alpha;10\n2;bêta;20\n")
put("03_documents/config.json", '{"nom": "test", "valeurs": [1, 2, 3]}\n')
put("03_documents/page.html", "<!doctype html><title>Test</title><p>Bonjour</p>\n")
put("03_documents/style.css", "body { margin: 0; }\n")
put("03_documents/script.py", "print('bonjour')\n")
put("03_documents/data.xml", '<?xml version="1.0"?><racine><a>1</a></racine>\n')
put("03_documents/LISEZMOI.TXT", "Extension en majuscules.\n")
put("03_documents/Rapport Final v2 (copie).txt", "Nom avec espaces et parenthèses.\n")
put("03_documents/fichier.tar.gz", b"\x1f\x8b\x08\x00" + b"\x00" * 20)   # double extension
put("03_documents/archive.backup.old.txt", "Plusieurs points dans le nom.\n")
pdf = image("03_documents/rapport.pdf", (400, 300), (200, 200, 200), "PDF", "Rapport")
exif(pdf, Title="Rapport de test", Author="Frédéric Test", Subject="Essai")
with zipfile.ZipFile(dest / "03_documents/archive.zip", "w") as z:
    z.writestr("a.txt", "a")
    z.writestr("dossier/b.txt", "b")
# fake docx / odt (zip with metadata)
for name, mime in [("lettre.docx", "docx"), ("tableau.xlsx", "xlsx"), ("texte.odt", "odt")]:
    with zipfile.ZipFile(dest / "03_documents" / name, "w") as z:
        z.writestr("[Content_Types].xml", "<Types/>")
        z.writestr("docProps/core.xml", "<cp:coreProperties/>")

# ---------- 04 Edge cases in names ----------
put("04_noms_speciaux/sans_extension", "pas d'extension\n")
put("04_noms_speciaux/Makefile", "all:\n\t@echo ok\n")
put("04_noms_speciaux/fichier avec  deux  espaces.txt", "x")
put("04_noms_speciaux/ espace_au_debut.txt", "x")
put("04_noms_speciaux/MAJUSCULES.TXT", "x")
put("04_noms_speciaux/minuscules.txt", "x")
put("04_noms_speciaux/MixTe_CaSe.Txt", "x")
put("04_noms_speciaux/àéèêëïôùûç ÀÉÈ.txt", "accents")
put("04_noms_speciaux/日本語ファイル.txt", "unicode")
put("04_noms_speciaux/emoji_😀.txt", "emoji")
put("04_noms_speciaux/a&b #1 [test] {x} ~tilde $dollar %pct.txt", "symboles")
put("04_noms_speciaux/" + "n" * 200 + ".txt", "nom long")
put("04_noms_speciaux/doublon.txt", "x")
put("04_noms_speciaux/Doublon.txt", "x")                  # case collision (Linux)
put("04_noms_speciaux/fichier vide.txt", b"")
put("04_noms_speciaux/001.txt", "x")
put("04_noms_speciaux/002.txt", "x")
put("04_noms_speciaux/010.txt", "x")
put("04_noms_speciaux/2.txt", "x")
put("04_noms_speciaux/10.txt", "x")
put("04_noms_speciaux/1.txt", "x")                         # natural vs alphabetical sort

# ---------- 05 Hidden ----------
put("05_caches/.fichier_cache.txt", "caché\n")
put("05_caches/.gitignore", "*.tmp\n")
put("05_caches/.hidden_photo.jpg", b"")
shutil.copy(dest / "01_photos_exif/sans_exif.jpg", dest / "05_caches/.photo_cachee.jpg")
exif(dest / "05_caches/.photo_cachee.jpg", Make="Sony", Model="ILCE-7M3",
     DateTimeOriginal="2022:12:25 18:00:00")
put("05_caches/visible.txt", "visible\n")
put("05_caches/.dossier_cache/secret1.txt", "dans un dossier caché\n")
put("05_caches/.dossier_cache/secret2.txt", "dans un dossier caché\n")
put("05_caches/.dossier_cache/.encore_cache.txt", "caché dans caché\n")
put("05_caches/.dossier_cache/sous/profond.txt", "profond\n")
put("05_caches/dossier_visible/.cache_dedans.txt", "caché dans visible\n")
put("05_caches/dossier_visible/normal.txt", "normal\n")
put("05_caches/.config/app/settings.ini", "[a]\nb=1\n")
put(".racine_cachee.txt", "caché à la racine\n")
put(".dossier_racine_cache/fichier.txt", "x\n")
put("Thumbs.db", b"\x00" * 16)
put(".DS_Store", b"\x00" * 16)

# ---------- 06 Nested tree ----------
for path in ["a/b/c/d", "a/b/autre", "a/x", "vide_mais_present"]:
    (dest / "06_arborescence" / path).mkdir(parents=True, exist_ok=True)
put("06_arborescence/racine.txt", "x")
put("06_arborescence/a/niveau1.txt", "x")
put("06_arborescence/a/b/niveau2.txt", "x")
put("06_arborescence/a/b/c/niveau3.txt", "x")
put("06_arborescence/a/b/c/d/niveau4.txt", "x")
photo("06_arborescence/a/b/autre/photo_profonde.jpg", 5)
put("06_arborescence/a/x/note.txt", "x")
put("06_arborescence/Dossier Avec Espaces/fichier un.txt", "x")
put("06_arborescence/Dossier Avec Espaces/fichier deux.txt", "x")
put("06_arborescence/Été 2024/photo.txt", "x")

# ---------- 07 Many files (250) ----------
exts = ["txt", "log", "dat", "jpg", "png", "mp3", "csv", "md"]
(dest / "07_beaucoup").mkdir(parents=True)
for i in range(1, 251):
    ext = exts[i % len(exts)]
    if ext in ("jpg", "png"):
        Image.new("RGB", (16, 16), (i % 256, (i * 3) % 256, (i * 7) % 256)).save(
            dest / "07_beaucoup" / f"fichier_{i:03d}.{ext}",
            "JPEG" if ext == "jpg" else "PNG")
    else:
        put(f"07_beaucoup/fichier_{i:03d}.{ext}", f"contenu {i}\n")
for i in range(1, 11):
    put(f"07_beaucoup/.cache_{i:02d}.tmp", "caché\n")      # 10 more hidden files
for i in range(1, 6):
    put(f"07_beaucoup/sous_dossier_{i}/element_{i:02d}.txt", "x")

# ---------- Files at the root ----------
put("readme_racine.txt", "Dossier de test pour Ant Renamer Qt.\n")
photo("photo_racine.jpg", 2)

# ---------- Varied modification dates ----------
import os
import time
base = time.mktime((2020, 1, 1, 12, 0, 0, 0, 0, -1))
for n, f in enumerate(sorted(dest.rglob("*"))):
    if f.is_file():
        t = base + n * 86400 * 3
        os.utime(f, (t, t))

files = sum(1 for f in dest.rglob("*") if f.is_file())
dirs = sum(1 for f in dest.rglob("*") if f.is_dir())
print(f"OK : {dest}  ({files} fichiers, {dirs} dossiers)")
