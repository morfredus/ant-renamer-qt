# Command-line parameters

The program does not (yet) support lots of parameters. It is not really possible to rename files by specifying all the rename parameters from the command line, but they can be given through a [saved batch file](batch_en.md).

Here is the list of supported parameters:

- `-b FileName.arb` : Loads the batch file called FileName.arb
- `-g` : Launches the renaming process after files have been added and batch file loaded
- `-x` : Quits the program after the renaming process
- `-q` : Silent mode, for scripts: no window and no dialog, the renaming starts and the program quits at the end (like `-g -x`, without the window). The exit code tells how it went: 0 everything done, 1 at least one file was not renamed because of an error, 2 nothing done (no file, no batch, batch file unreadable, or hidden files in the list). The log file of the options is still written
- `--set-dates-from-exif` : Instead of renaming, gives the files the shooting date written in their EXIF data (or XMP sidecar) as modification date, see [Set dates from EXIF](files_en.md#setdates). With `-g` or `-q` it runs at once (the parameter is the consent: no question is asked; a file with no EXIF date is left alone); alone, the window shows the table of the dates and asks first. The sidecar files follow their picture and the history can undo it.
- `--shell Path` : (Explorer context menu) adds the path to the window that is already open, or starts the program if it is not running
- Only one window at a time: a second launch shows a message and quits. Paths given alone (a file dropped on the program or its shortcut, "Open with") are added to the window that is already open. With `-g` or `-x`, the message goes to the error output and the exit code is 3.
- `--install-shell` / `--remove-shell` : adds / removes the Explorer context menu entry, then quits
- `-a` : Change the file add mode, based on the letters following the "-a" parameter:
  - `f` : Add files from filenames/paths that follow
  - `F` : Add folders that follow, or contained in folders, see examples
  - `r` : Recursive addition of folders' contents

The filenames and paths can be relative or absolute, and they can contain wildcards (\* and ?).

**Ant Renamer Qt**: the program is called `antrenamer-qt.exe` (Windows) or `antrenamer-qt` (Linux); the examples below use the Windows name; on Linux, type `antrenamer-qt`.

Here are some examples:

- `antrenamer-qt.exe -b d:\test.arb -af c:\data\*.* -g -x`  

    This will load the batch file specified, add all files contained in c:\\data folder, start renaming and then exit
- `antrenamer-qt.exe -af c:\data`  

    This will add the file called c:\\data
- `antrenamer-qt.exe -afr c:\data`  

    This will add the file called c:\\data. If it is a folder, it will add the files that it contains (recursive search)
- `antrenamer-qt.exe -aF c:\data`  

    This will add the "data" folder
- `antrenamer-qt.exe -aF c:\data\*.*`  

    This will add the folders containted in the "data" folder, but not the files
- `antrenamer-qt.exe -af c:\data\*.* c:\images\*.jpg c:\test.txt -afr c:\mp3\*.*`  

    You can combine several paths/filenames, use masks, and use different "-a" parameters in one command line

If a name is given without any "-a" parameter, it is analyzed simply: if a filename ending by ".arb" is given, it is loaded as batch file; if a folder is given, its contents is added recursively (files only, does not support wildcards, requires a full folder name).

On Windows, it is possible to add Ant Renamer Qt in the "Send to" list (in the menu that appears when you click with right mouse button on a folder in Windows Explorer). So this adds the possibility to send a folder to Ant Renamer Qt from its contextual menu, to load its content in the files list. To do that, press Windows+R, type `shell:sendto` and press Enter. A folder window should open; place there a shortcut to `antrenamer-qt.exe`.
