# Formatting strings

In some actions, you can use special tags that are delimited by "%" characters. They are available in the actions that use a mask: [String insertion](actions_hr.md#stringinsert), [Enumeration](actions_hr.md#enum), [Use mp3 tag info](actions_hr.md#mp3), [Random names](actions_hr.md#random) and [Use EXIF info](actions_hr.md#exif). (The [date & time](actions_hr.md#datetime) action has its own system of letters.)

Since the "%" is then used as a special character, to have a simple "%" in the file name you have to enter "%%".

In each action that support special tags, the available tags are listed in the "Notes" text under the parameters of this action.

For example, the [Use mp3](actions_hr.md#mp3) action supports ID3-related tags, like %author% and %title% in addition of %name% and %ext%. So a typical mask for mp3 files would be something like `%author% - %title%%ext%` (%ext% includes the dot, so we do not need to put the dot in the mask used here).

## The tags

| Tag | Meaning | Available in |
|---|---|---|
| `%name%` | Original name of the file, without extension | all |
| `%ext%` | Original extension, including the dot (empty for a folder) | all |
| `%folderN%` | Name of the N-th parent folder, see [below](#folder) | all |
| `%count%` and similar | Number of items in a folder, see [below](#count) | all (renamed folders) |
| `%%` | A "%" character | all |
| `%num%` | The counter | Enumeration, EXIF |
| `%author%`, `%title%`, `%album%`, `%year%`, `%genre%`, `%comm%`, `%track%` | ID3 tags | mp3 |
| `%random%` | The random part | Random names |
| `%DateTimeOriginal%`, `%Make%`, `%IptcKeywords%`, `%XmpTitle%`... | EXIF, IPTC and XMP data, see [the list](metadata_tags_hr.md) | EXIF |
| `$if(...)` | Conditional text, see [below](#if) | mp3, EXIF |

Tags must be written exactly as in the table (in lowercase: `%name%`, not `%Name%`); only the EXIF, IPTC and XMP tags are recognised whatever their case. A tag that is not known (for example because of a typing error) is left as it is in the name, "%" characters included: this is how you notice it in the preview.

## The folder tag {#folder}

A %folderN% tag is usually available. This tag allows you to use the name of the parent folders in the filename itself. This is a little difficult to explain, so I'll give you some examples.

If you have a file in "c:\\data\\images\\movies\\poster.jpg" and that you use the [String insertion](actions_hr.md#stringinsert) action:

- With mask = `%folder1% - ` the file will be renamed to "movies - poster.jpg"
- With mask = `%folder2% - ` the file will be renamed to "images - poster.jpg"
- etc.

You can also use a relative path, e.g. if you have several folders, each one having lots of files in it, and all of these files are not especially unique between two different folders. You may want to insert folder name and then move all the files to the common parent folder. Here is an example; you have the following files:

- c:\\data\\images\\movies\\poster1.jpg
- c:\\data\\images\\games\\poster1.jpg
- c:\\data\\images\\games\\poster2.jpg

If you rename them with String insertion action and a string like `../%folder1% ` inserted at position 0 from beginning, you will get the following in the "images" folder:

- movies poster1.jpg
- games poster1.jpg
- games poster2.jpg

Now you can delete the empty "movies" & "games" folders.

If you enter a negative number instead, the count will be done from the root rather than from the current folder. So still using "c:\\data\\images\\movies\\" as folder:

- `%folder-1%` = "c"
- `%folder-2%` = "data"
- `%folder-3%` = "images"
- etc.

Using the %name% tag, you can also want to have the files placed in a folder that takes the file's name. For that, the ["Force directories" option](options_hr.md#processing) has to be enabled. If you have the following files:

- cars.doc
- cars.xls
- cars.jpg
- houses.doc
- houses.xls
- houses.jpg

Still with the String insertion action, using a string like `%name%\` inserted at position 0 from beginning, you will get the three "cars" files in a "cars" folder, and the three "houses" files in a "houses" folder.

(On Linux, write "/" instead of "\\" as the folder delimiter, for example `%name%/`: a backslash is an ordinary character in a Linux file name. The "/" works on Windows too.)

## The count tags {#count}

When renaming folders, `%count%` and a few derivated tags are available:

- `%count%` = number of files and subfolders contained in renamed folder
- `%countr%` = same but also counts what is in subfolders (recursive)
- `%countfiles%` = counts only files
- `%countfilesr%` = same but includes files of subfolders (recursive)
- `%countfolders%` = counts only sub-folders
- `%countfoldersr%` = same but includes subfolders of subfolders (recursive)

## The $if function {#if}

For the [Use mp3 tags](actions_hr.md#mp3) and [Use EXIF info](actions_hr.md#exif) actions, there is also a special tag: $if. It allows you to make conditional formatting. For example, when a field is empty you may want to not display the " - " that follows it. Or to put a special value.

The syntax is the following: $if(condition,value-if-true,value-if-false). If "condition" is not an empty string, value-if-true will be used. In the other case, value-if-false will be used. Since you may need to use commas and parenthesis inside the $if function, you can put the values inside quotation marks ("...") to prevent them from being analyzed. But be careful with them: they also prevent the %tag% from being analyzed. To get a quotation mark itself, type it twice (""). A "$" that is not followed by "if(" is kept as a character; "$$" gives a "$".

Here are some examples that will explain that better. In the files list there are two mp3 files that has a bad name (1.mp3 and 2.mp3) but that has the following valid ID3 information:

- 1.mp3: Author = Blur, Title = Song 2, Album is empty, Track is empty
- 2.mp3: Author = Vivaldi, Title is empty, Album = Le Quattro Stagioni, Track = 1

Here is the result using various masks:

`%author% - %title%%ext%`  

Simply take the author and title, separated by a dash. Here we are not using the if function, and the result is not really nice when there are empty fields.

- Blur - Song 2.mp3
- Vivaldi - .mp3

`%author%$if(%title%, - %title%,)%ext%`  

Take the author and, if the title is not empty, add a dash and the title.

- Blur - Song 2.mp3
- Vivaldi.mp3

`%author%$if(%album%," ("%album%")",)$if(%title%,", "%title%,)%ext%`  

Take the author, if it exists add the album name between parenthesis, then if it exists add the title separated by a comma. Here you see that parenthesis and comma have to be put between quotes to not be taken as "end of if" or value separator.

- Blur, Song 2.mp3
- Vivaldi (Le Quattro Stagioni).mp3

`%author%$if(%album%," ("%album%")",)$if(%title%,", "%title%,$if(%track%, - %track%,))%ext%`  

Same as previous one, but inside the title's if we add another if to add the track number only in the case that the title is empty.

- Blur, Song 2.mp3
- Vivaldi (Le Quattro Stagioni) - 1.mp3
