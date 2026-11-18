# MP3 tags

The [Use mp3 tag info](actions_hr.md#mp3) action reads the tags stored inside the MP3 files and lets you use them in the mask, like any other `%tag%` (see ["Formatting"](format_hr.md)). Two kinds of tags are read:

- **ID3v2** (2.2, 2.3 and 2.4), at the beginning of the file: the modern format, with accents and any alphabet (Latin-1, UTF-16 and UTF-8 texts are understood).
- **ID3v1 and v1.1**, at the end of the file: the old format, limited to 30 characters per field.

When both exist, the v2 value is used and v1 only fills what v2 does not have. A file with no ID3 tag at all is not renamed: the log says so (blue line).

## The tags

| Tag | Meaning | ID3v2 frame | ID3v1 |
|---|---|---|---|
| `%author%` | Author / artist | TPE1 (v2.2: TP1) | Artist |
| `%title%` | Title of the song | TIT2 (TT2) | Title |
| `%album%` | Album name | TALB (TAL) | Album |
| `%year%` | Year (four digits) | TYER, TDRC (TYE) | Year |
| `%genre%` | Genre, as text | TCON (TCO) | Genre number, shown as its name |
| `%comm%` | Comment | COMM (COM) | Comment |
| `%track%` | Track number | TRCK (TRK) | Track (v1.1 only) |

`%name%`, `%ext%`, `%folderN%` and `%%` are also available, as well as the `$if` function.

## Good to know

- **A missing tag gives an empty text**, not a number: `$if(%title%,...)` really tests whether the file has a title.
- **A file with no usable tag is not renamed.** If the mask gives no letter or digit (for example `%author% - %title%%ext%` on a file with neither), the file keeps its name and the log says "No usable tag: the new name would be empty".
- **Track.** "3/12" gives `3`. The option "Two-digit track number" of the action adds a zero in front of a one-digit number ("3" becomes "03"), so that the files sort in the album order. A file without a track number gives an empty text.
- **Genre.** v2 numeric codes such as `(17)` are replaced by the name of the genre ("Rock").
- **Forbidden characters.** A tag can contain any character, so the values are cleaned like every file name: quotation marks (") become apostrophes ('), question marks become a dot, and `\ / : * < > |` and the control characters become a dash.
- **Other formats.** Only MP3 files are read: FLAC, OGG, M4A... are not.

## Examples

| Mask | Result |
|---|---|
| `%author% - %title%%ext%` | `Queen - Bohemian Rhapsody.mp3` |
| `%track% - %title%%ext%` (two-digit option) | `03 - Bohemian Rhapsody.mp3` |
| `%author% - %album% - %track% - %title%%ext%` | `Queen - A Night at the Opera - 11 - Bohemian Rhapsody.mp3` |
| `%author%$if(%title%, - %title%,)%ext%` | `Queen.mp3` when the title is empty, no dangling hyphen |
| `$if(%year%,%year% - ,)%album% - %title%%ext%` | `1975 - A Night at the Opera - Bohemian Rhapsody.mp3` |
