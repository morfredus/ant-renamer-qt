# Command-line parameters

The program does not (yet) support lots of parameters. It is not really possible to rename files by specifying all the rename parameters from the command line, but they can be given through a [saved batch file](batch_en.md).

Here is the list of supported parameters:

- `-b FileName.arb` : Loads the batch file called FileName.arb
- `-g` : Launches the renaming process after files have been added and batch file loaded
- `-x` : Quits the program after the renaming process
- `--shell Path` : (Explorer context menu) adds the path to the window that is already open, or starts the program if it is not running
- `--install-shell` / `--remove-shell` : adds / removes the Explorer context menu entry, then quits
- `-a` : Change the file add mode, based on the letters following the "-a" parameter:
  - `f` : Add files from filenames/paths that follow
  - `F` : Add folders that follow, or contained in folders, see examples
  - `r` : Recursive addition of folders' contents

The filenames and paths can be relative or absolute, and they can contain wildcards (\* and ?).

**Ant Renamer Qt**: the program is called `antrenamer-qt.exe` (Windows) or `antrenamer-qt` (Linux); in the examples below, replace `Renamer.exe` with this name.

Here are some examples:

- `Renamer.exe -b d:\test.arb -af c:\data\*.* -g -x`  

    This will load the batch file specified, add all files contained in c:\\data folder, start renaming and then exit
- `Renamer.exe -af c:\data`  

    This will add the file called c:\\data
- `Renamer.exe -afr c:\data`  

    This will add the file called c:\\data. If it is a folder, it will add the files that it contains (recursive search)
- `Renamer.exe -aF c:\data`  

    This will add the "data" folder
- `Renamer.exe -aF c:\data\*.*`  

    This will add the folders containted in the "data" folder, but not the files
- `Renamer.exe -af c:\data\*.* c:\images\*.jpg c:\test.txt -afr c:\mp3\*.*`  

    You can combine several paths/filenames, use masks, and use different "-a" parameters in one command line

If a name is given without any "-a" parameter, it will be analyzed like in Ant Renamer 2.07 (and older) : if a filename ending by ".arb" is given, it is loaded as batch file; if a folder is given, its contents is added recursively (files only, does not support wildcards, requires a full folder name).

On Windows, it is possible to add Ant Renamer in the "Send to" list (in the menu that appears when you click with right mouse button on a folder in Windows Explorer). So this adds the possibility to send a folder to Ant Renamer from its contextual menu, to load its content in the files list. To do that, go to "Start" -\> "Run" and type `sendto`. A folder window should open; place there a shortcut to Ant Renamer.
