# Translating Ant Renamer Qt: the `.lng` file process

*How to add a language to the program, or fix an existing translation.*
French version: [TRANSLATING_fr.md](TRANSLATING_fr.md). Overview of the code: [ARCHITECTURE.md](ARCHITECTURE.md), contribution rules: [CONTRIBUTING.md](../CONTRIBUTING.md).

## 1. How it works, in short

Ant Renamer Qt does not use Qt `.ts` files. It reads the language files of the original program: `app/Languages/*.lng` (XML text files, one per language). The texts of the program are written in English in the source code; at run time each English text is looked up in the translation.

- `English.lng` is the **reference**: it holds every English text, each under a *key* (the path of the XML element, for example `MainForm/ActionAddFiles/Caption`).
- A translation file has **the same structure**: same elements, same keys, texts in another language.
- The program pairs the English text and its translation key by key. If a key is missing, or its text is empty or identical to the English one, the English text is displayed: nothing breaks, the text is just not translated.
- Same English text in several places (the caption and the hint of a button): they get the same translation, the first one found in alphabetical order of the keys (the caption, in practice).

## 2. Create a new language

1. **Copy `app/Languages/English.lng`** to `app/Languages/<Language name>.lng`. The file name is the name of the language in English, as it will be shown in the program and matched with the language of the system: `Italian.lng`, `Portuguese (Brazil).lng`, `Spanish (Spain).lng`. When a language has several variants, the name is "Language (variant)".
2. **Choose the encoding** and declare it on the first line, `<?xml version="1.0" encoding="..."?>`. The program reads the encoding written there: `UTF-8` (recommended for a new file), but also `windows-1250`, `windows-1251`, `windows-1252`, `windows-1253`, `shift_jis`, `gb2312`, `big5`... Without a declaration it assumes `windows-1252`. Save the file in the encoding you declared (a text editor that shows it, such as VS Code or Notepad++, avoids mistakes). A UTF-8 byte order mark added by the editor is accepted; a file declared UTF-8 that is not valid UTF-8 is read as windows-1252 and the program says so on its error output.
3. **Fill the header**, the comment `<!-- ... -->` at the top. The program shows these fields in *Options > Language*:

   | Field | Content |
   |---|---|
   | `Name` | Name of the language in English |
   | `LocalName` | Name of the language in itself (`Français`, `Deutsch`...), shown in the list |
   | `Version` / `VersionFormat` | Version of the file, e.g. `3.0` (`VersionFormat` stays as in the model) |
   | `Comments` | Free text; `|` starts a new line |
   | `Authors` | Name(s) of the translator(s); `|` separates several values |
   | `Icon` | Optional 16 x 16 BMP flag in base64: leave the model's or delete the line |

4. **Translate the texts**: in every element, replace the value of `Caption`, `Hint`, `Text`... by the translation (rules in section 4). Do not change element names, `Name=` / `Index=` attributes or the structure.
5. **Check that the file is valid XML** (a single misplaced quote makes the rest of the file unusable: the program prints `Language file <name>: XML error at line N: ...` on its error output, see section 5, and only translates what it read before the error): `xmllint --noout "Italian.lng"` on Linux, or open the file in a browser or VS Code.
6. **Try it** (section 5), then credit yourself (section 6) and send the file (section 7).

## 3. Fix or complete an existing translation

Edit the text of the key in the existing `app/Languages/<Language>.lng`, keeping its encoding, then test (section 5). The new texts of a version of the program (new buttons, options) are added to `English.lng` and to French by the project; for the other languages they stay in English until a translator adds the key. To find them: run the program with `ANTREN_MISSING=1` (section 5), or compare the keys of `English.lng` with those of your file with a comparison tool (`diff`, WinMerge, VS Code *Compare*): every line of `English.lng` that has no equivalent in your file is a text to translate.

## 4. Rules for the texts

- **Placeholders**: keep `%s`, `%d`, `%1`, `%2`... exactly as they are, you may only move them. `%s`/`%d` (original program) and `%1`...`%9` (Qt) are both understood; the order of several `%s` is the order in the English text.
- **Hints**: a `Hint` has the form `Short text|Long text` (title of the tooltip, then the explanation, also shown in the status bar). Keep the `|`, even when the first part is empty (`Hint="|Clear the string table"`).
- **Notes under the actions** (`lbl...Notes` elements): the text contains small HTML codes (`&lt;BR&gt;` for a line break, `&lt;LINK&gt;...&lt;/LINK&gt;` for a link). Keep them, in the form used in the file. **Important**: the text between `LINK` codes that names another action (for example "String replacement") must be **exactly the translated name of that action in the actions list**, otherwise a click on the link does nothing. The "Help" link (`<LINK>Help</LINK>`) opens the help page.
- **XML characters**: write `&amp;` for `&`, `&lt;` for `<`, `&gt;` for `>`, `&quot;` for `"` inside an attribute. In the model, `&#38;&#38;` in a title is the way to show a single `&`.
- **Tags** such as `%name%`, `%ext%`, `%folderN%`, `%num%`, `%author%` or the letters of the date mask (`yyyy`, `mm`...) are **not translated**: they are what the program recognises.
- **Spaces at the ends**: some texts are glued together by the program (" at position %1", " using creation date"). The English text decides: if it starts or ends with a space, so does the translation, whatever you typed.
- **Same length**: toolbar buttons have little room, prefer short captions to long ones.
- **Texts added by Ant Renamer Qt** (portable mode, theme, *About* window, sidecar files...) are in the last section of `English.lng`, `<QtPort>`, with their French translation in `French.lng`. A translator adds the same `<Item Name="..." Value="..."/>` lines to their file; until then these texts stay in English.

## 5. Test

1. Put the `.lng` file in the `Languages` folder **next to the program** (`build-.../app/Languages` for a build, which is refreshed at each build from `app/Languages`; `<prefix>/share/antrenamer-qt/Languages` for an installed Linux package).
2. Start the program, *Options > Language*, pick the language, click OK (the language applies at once, no restart needed). To avoid touching your own settings, a file `antrenamer-qt.ini` containing `language=Italian` (the file name without `.lng`) next to the program does the same in portable mode.
3. **List the texts without translation**: start the program with the environment variable `ANTREN_MISSING=1` and read what it writes on the error output, lines `MISSING: <English text>` (this also opens every dialog of the program so that all their texts are looked up). On Windows the program has no console, redirect the output: `set ANTREN_MISSING=1` then `antrenamer-qt.exe 2> missing.txt`. On Linux: `ANTREN_MISSING=1 antrenamer-qt 2>&1 | grep MISSING`.
4. Look at every page (Files, Actions with each of the 14 actions, Log), the *Options* window with its five pages, the dialogs for adding folders and for drag & drop, and the message boxes. Check that nothing is cut off. Screenshots can be taken by the program itself, see the developer helpers in [BUILDING.md](BUILDING.md).
5. Check also that the **help** shows correctly (see section 8).

## 6. Credit the translator

- The translator's name goes in the header of the file (`Authors`), shown in *Options > Language*.
- The "Translations" list of the *About* window (Credits tab) is written in `app/dialogs.cpp` (function of the `AboutDialog`), and the list at the bottom of the help page *About the author, credits & links* (`app/help/about_<code>.md`, every language): add your line, in alphabetical order of the language.

## 7. Send the translation

On GitHub, [github.com/morfredus/ant-renamer-qt](https://github.com/morfredus/ant-renamer-qt): open a *pull request* with the new or modified `.lng` file (and the credit lines), or, if you are not used to it, an *issue* with the file attached. Say which version of the program you tested with. A line in `CHANGELOG.md` (section *Unreleased*) and in `docs/WHATS_NEW.md` / `WHATS_NEW_fr.md` mentions the new language.

The translators of the original Ant Renamer can send their file as it is: the format is the same, only the texts added by Ant Renamer Qt will be missing (they stay in English).

## 8. The help

The help is separate from the `.lng` file: Markdown pages in `app/help/`, **English, French, Croatian and Tamil** for now (`<page>_en.md`, `_fr.md`, `_hr.md`, `_ta.md`, with the table of contents `toc_<code>.md`). For the other languages of the interface the help is displayed in English.

To add a language to the help: copy the English pages and `toc_en.md` to `<page>_<code>.md` and `toc_<code>.md`, translate them (keep the file names of the links and the `{#id}` after the headings: they are the targets of the links between pages), then add the language to `helpLanguageCode()` in `app/lngtranslator.cpp` (language name of the `.lng` file -> code). The help is long (18 pages): a translation of the interface alone is already very useful, the help can come later.

## 9. Quick checklist

- [ ] `app/Languages/<Language>.lng` created from `English.lng`, encoding declared and respected
- [ ] header filled (`Name`, `LocalName`, `Version`, `Authors`)
- [ ] valid XML (`xmllint --noout`)
- [ ] placeholders (`%s`, `%1`...), `|` of the hints and `<LINK>` texts checked
- [ ] tested with `ANTREN_MISSING=1`, no unexpected `MISSING` line, every page looked at
- [ ] translator credited (file header, About window, help credits)
- [ ] pull request (or issue) sent
