# EPUB tags

The [Use EPUB info](actions_hr.md#epub) action reads the metadata stored inside the EPUB files (e-books) and lets you use it in the mask, like any other `%tag%` (see ["Formatting"](format_hr.md)). An EPUB is a ZIP archive: the program opens it, finds the OPF file named in `META-INF/container.xml`, and reads its Dublin Core fields (`dc:title`, `dc:creator`...) and the series information. Nothing is extracted to your disk and the file is never modified.

A file that is not a valid EPUB (not a ZIP, no `container.xml`, no OPF file) is not renamed: the log says so (blue line).

## The tags

| Tag | Meaning | Where it comes from |
|---|---|---|
| `%author%` | Author(s) | `dc:creator`, without a role or with the role `aut`. Several authors are joined with " & " |
| `%title%` | Title | The first `dc:title` |
| `%year%` | Year of publication | The first four digits of the first `dc:date` |
| `%publisher%` | Publisher | `dc:publisher` |
| `%lang%` | Language code (`fr`, `en`...) | `dc:language` |
| `%isbn%` | ISBN | The `dc:identifier` marked as ISBN (`opf:scheme="ISBN"` or `urn:isbn:...`) |
| `%series%` | Name of the series | Calibre (`calibre:series`) or EPUB 3 (`belongs-to-collection`) |
| `%seriesnum%` | Number in the series | Calibre (`calibre:series_index`) or EPUB 3 (`group-position`) |

`%name%`, `%ext%`, `%folderN%` and `%%` are also available, as well as the `$if` function.

## Good to know

- **The quality of the names is the quality of the metadata.** E-books found on the Internet are uneven: the title may already contain the series or the edition ("(French Edition)"), the series and its number are often missing, the author may be wrong. Check the preview before renaming.
- **A missing tag gives an empty text.** Use `$if` to avoid a dangling separator when it is empty.
- **A file with no usable tag is not renamed.** If the mask gives no letter or digit (for example `%author% - %title%%ext%` on a file with neither), the file keeps its name and the log says "No usable tag: the new name would be empty".
- **Series number.** Calibre writes "1.0": it is shown as `1`. The option "Two-digit series number" of the action adds a zero in front of a one-digit number ("1" becomes "01"), so that the volumes sort in the right order.
- **Authors.** Creators with another role (illustrator, translator, editor...) are ignored.
- **Year.** The first `dc:date` is used. Some e-books give the date of the conversion instead of the date of the first publication.
- **Spaces.** Line breaks and repeated spaces in a value are replaced by a single space.
- **Forbidden characters.** The values are cleaned like every file name: `/ \ : * ? " < > |` are replaced.

## Examples

| Mask | Result |
|---|---|
| `%author% - %title%%ext%` | `Antoine de Saint-Exupéry - Le Petit Prince.epub` |
| `%author% - %title% (%year%)%ext%` | `Antoine de Saint-Exupéry - Le Petit Prince (1943).epub` |
| `%author% - %title%$if(%seriesnum%, - %seriesnum%,)%ext%` | no dangling hyphen when the e-book has no series number |
| `$if(%series%,%series% %seriesnum% - ,)%author% - %title%%ext%` (two-digit option) | `Contes 03 - Antoine de Saint-Exupéry - Le Petit Prince.epub`, nothing added without a series |
