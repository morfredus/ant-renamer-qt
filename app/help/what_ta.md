# What the program does...

This program is designed to rename files and folders. So it will only touch their names, it will not modify the contents of the files.

It was made for renaming, but in the [Options](options_ta.md#processing) you can ask it to copy the files instead of renaming them.

It can [store a predefined list of actions](batch_ta.md) to reuse them later, and run it from the [command line](params_ta.md).

It supports unicode, which means that you can rename files that have characters in Greek, Japanese, Chinese, Arab or any other language. Since most of the programs have problems with such names, it can be useful to be able to rename them to Latin (basic ASCII) characters for example.

It reads the information stored inside some files to build the new names:

- the ID3 tags of the MP3 files (ID3v1, ID3v1.1 and ID3v2.2, 2.3 and 2.4);
- the EXIF, IPTC and XMP data of the pictures (JPEG, and TIFF-based files such as most RAW files; XMP also from PNG, WebP... and from `.xmp` files);
- the date and time of the files.

When a picture is renamed, its sidecar files (`.xmp`, DxO `.dop`) can be renamed with it, see the [options](options_ta.md#processing).

Ant Renamer Qt runs on Windows and on Linux.

# ... and what it does not

As I said, it won't modify the contents of the files.

It will not either modify files' date & time.

It will not modify the ID3 (tag) contained in MP3s or EXIF from JPEG; it only reads this info for building a new file name.

It does not read any info from files other than the MP3s and the pictures described above.

It cannot be fully controlled from command line, though it supports some [parameters](params_ta.md).

Note that the contents of this list is not fixed. Except modifying the contents of the files, all the "does not" things may appear in future versions.

## Known limitations

- The Undo button has only one level: it reverts the last renaming only.
- EXIF: about forty tags are read, and the MakerNote (private data of the camera makers) is not decoded. IPTC is read from JPEG files only. See [EXIF, IPTC and XMP tags](metadata_tags_ta.md).
- The audio tags are the ID3 tags of MP3 files; other formats (FLAC, OGG, M4A...) are not read.
- The creation date of a file is not stored by every file system (it is often missing on Linux): the modification date is then used.
- The help is available in English, French, Croatian and Tamil; the interface itself exists in 27 languages. The pages written for Ant Renamer Qt (modern layout, tags, sidecar files...) may exist only in English and French, the other languages then show the English text.
- Windows 11: the entry added to the Explorer context menu is in the "Show more options" menu (or Shift+F10), the short menu is not supported.
