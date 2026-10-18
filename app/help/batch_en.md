# Predefined list (batch)

Ant Renamer can store and retrieve predefined lists of actions, called here "batches". They can be managed from the [Actions page](actions_en.md).

In addition to the Files toolbar which is always visible, a toolbar to manage these batch files is displayed:  

![Batch toolbar](img/batchtoolbar_en.png)

Here are the commands:

- ![Add to batch](img/batchadd.png) "Add to batch" button (Ctrl+F2) will add currently selected action and its parameters to the list.
- ![Remove from batch](img/batchdel.png) "Remove selected actions" button (Shift+F6) will remove from the list the items that are selected.
- ![Clear](img/clear.png) "Clear batch" button (Shift+F7) will remove from the list all the predefined actions.
- ![Open](img/open.png) "Open" button (Ctrl+O) will display a standard file selection window, so you can select a batch file to load in the list.
- ![Save](img/save.png) "Save" button (Ctrl+S) will display a standard file selection window, so you can select where to save the contents of the list for a future use.

These batch files can be used for combining multiple actions in one renaming process. They can be saved for a future use, or to use them from [command-line](params_en.md).

They are stored as XML files, but they have a ".arb" extension (meaning "Ant Renamer Batch"). The files of Ant Renamer 2 can be opened by Ant Renamer Qt and the other way round.

If the batch is empty when you click "Go" (or refresh the preview), the action that is displayed on the Actions page is the one that is applied, once, with its current parameters.

The list of current planned actions is displayed in the bottom panel, called "Batch contents". If it is not visible, you can show it by clicking on the checkbox at the left of its title.

You can move the items up and down in the list by drag & drop or using the four additional icons that appear in the toolbar when the batch panel is visible.

In the classic layout, after having been added, an item cannot be modified: you have to remove it, redefine the parameters and add it again. In the modern layout of Ant Renamer Qt it can be modified: double-click it to load its parameters in the editor, then click "Validate the modification" or "Modify", see [editing an action of the batch](modernui_en.md#edit).
