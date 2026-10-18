# Adding files & folders

There are several ways to add files to the list:

- ["Add files" button](#addfiles) in main window
- ["Add folders" button](#addfolders) in main window
- [Drag & drop files](#dragdrop) from the file manager (Windows Explorer, Nautilus, Dolphin...)
- [Context menu of the file manager](#explorer) (Windows)
- [Command line](#cmdline)

## "Add files" button {#addfiles}

When you click the ![Add files](img/addfiles.png) "Add files" button (F2) a standard file selection window appears. This window is controlled by the system, so it works like in other programs; you can select several files with Ctrl and Shift. One thing to know is that the order of the files that will be returned by this window is not especially the order that you see. If the order matters (for example for an enumeration), sort the list afterwards by clicking on a column header, see the [Files page](files_en.md).

There may also be some speed problems when trying to add thousands of files. The window may take a long time to send the filenames to the program. If this is the case, use the ["Add folders"](#addfolders) function instead.

## "Add folders" button {#addfolders}

When you click the ![Add folders](img/addfolders.png) "Add folders" button (F3) a window appears, showing the drives and folders of the computer as a tree and allowing you to select one or several folders (Ctrl and Shift to select several). This window was not only designed to add folders to the list, but also their contents.

A few options at the bottom of the window allow to specify what should be added to the list:  
![Options for adding files](img/addfilesoptions.png)

The first one ("Add files of selected folders") specifies if the files contained in selected folder(s) should be added or not. The second one ("Add selected folders") specifies if the selected folder(s) should be added or not. The third one ("Apply these two options also to subfolders") specifies if the sub-folders of selected folder(s) have to be included too (this applies to all sub-levels, not only the first one). The three choices are remembered for the next time.

To add the files of a folder located on the network, the [drag & drop](#dragdrop) or the [command line](#cmdline) may be easier than this window.

## Drag & drop files {#dragdrop}

You can drag & drop files and folders from the file manager (Windows Explorer and other programs that are compatible with it, like Total Commander, or the file manager of your Linux desktop) directly to the main window of Ant Renamer. This is usually a much easier way to add files than selecting them in the "Add files" window.

By default, when you drop the files a window appears. It allows you to choose how you want to add the files. These options are the same as those available in the ["Add folders" window](#addfolders). An option labelled "Do not show this window again" allows to keep the select options for future drag & drop, so the window is not displayed again. If you check this option and want to discard it (or change the options), you can do that in the "Files & Folders" page of the [Options window](options_en.md#files).

The same limitations as for the "Add files" window apply (files order and speed problems with lots of files).

## Context menu of the file manager (Windows) {#explorer}

On Windows, the program can add an "Add to Ant Renamer Qt" entry to the context menu of the files and folders of Windows Explorer. It is enabled in the [Options](options_en.md#files), "Files & Folders" page. When several items are selected, they all arrive together in the window of Ant Renamer that is already open (the program is started if it is not running). On Windows 11 the entry is in the "Show more options" menu (or Shift+F10).

## Command line {#cmdline}

Files can also be added when the program is started from the command line, see the [parameters](params_en.md). Wildcards (`*` and `?`) are accepted there.
