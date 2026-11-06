# Manual Test Notes

> Automated tests cover the logic. These checks cover what they cannot: the real dialogs, the real window and the files as other programs see them. Run the relevant section before merging a branch that touches it. Record failures as issues.

Build and start the app from the repository root:

```powershell
cmake --build --preset debug
.\build\debug\src\app\scribe.exe
```

## Encodings and line endings

Create the test files in Notepad (Save As, choose the encoding) or with PowerShell. Open each in Scribe, press `Ctrl+S` without editing, and compare the file bytes before and after (`Get-FileHash` must match).

| File | Expected |
|---|---|
| UTF-8, no BOM, with `é` and `€` | Text displays correctly. Hash unchanged after save. |
| UTF-8 with BOM | Same, and the BOM is still there after saving. |
| UTF-16 LE and UTF-16 BE (Notepad "Unicode" / "Unicode big endian") | Same. |
| ANSI with `é` (Notepad "ANSI") | Same. |
| LF-only and CR-only files | Open as multiple lines. Hash unchanged after save. |

## Tabs

| # | Steps | Expected |
|---|---|---|
| 1 | `Ctrl+N` three times | Three new tabs appear to the right and the last one is active. Each tab has a close button. |
| 2 | Type in one tab | Its label gets ` *`. The window title shows the `*` only while that tab is active. |
| 3 | `Ctrl+Tab` and `Ctrl+Shift+Tab` while typing in the editor | Switches tabs and wraps around. No tab character is inserted. |
| 4 | `Ctrl+O` a file, then `Ctrl+O` the same file again | The second open switches to the existing tab and keeps its edits. |
| 5 | Open a file while the current tab is an empty, unmodified Untitled | The file opens in that tab (no extra tab). |
| 6 | `Ctrl+W`, middle-click on a tab, and click a tab's close button | Each closes the tab. Closing the last tab leaves one empty Untitled tab. |
| 7 | File > Close Other Tabs / Close All Tabs | Only the active tab stays / one empty Untitled remains. |
| 8 | Hover a tab with a saved file | The tooltip shows the full path. |
| 9 | Open a file named `a&b.txt` | The tab reads `a&b.txt`, not `ab.txt` with an underline. |
| 10 | Open 15 or more files | The tab bar stays usable (scroll arrows). The full overflow design comes later. |
| 11 | Drag a tab to reorder | The tab moves. Save and the title still act on the active tab. |

## Unsaved changes

Use a saved file `a.txt` and an untitled tab with some typed text. A tab is "modified" when its label ends with ` *`.

| # | Steps | Expected |
|---|---|---|
| 1 | Type in a tab, then close it with `Ctrl+W` | A dialog asks `Do you want to save changes to "<name>"?` with **Save** (the default button), **Don't Save** and **Cancel**. The tab being asked about is active. |
| 2 | In that dialog press `Esc`, then click the window's close button | Both act as **Cancel**: the tab stays, text and `*` unchanged. |
| 3 | Choose **Don't Save** | The tab closes. The file on disk is unchanged. |
| 4 | Choose **Save** for a saved file | The file is written, then the tab closes. |
| 5 | Choose **Save** for an untitled tab | A Save As dialog opens. Cancel it: the tab stays open and modified. Pick a name: the file is saved and the tab closes. |
| 6 | Choose **Save** for a file that cannot be written (for example a read-only file) | An error explains the save failed. The tab stays open and modified. |
| 7 | Click a tab's close button, and middle-click a modified tab | Both ask, exactly like `Ctrl+W`. A tab without changes closes without asking. |
| 8 | With three modified tabs: File > Close Other Tabs | Asks once for each of the other two tabs, never for the active one. |
| 9 | With three modified tabs: File > Close All Tabs, answer **Don't Save**, then **Cancel** | Asks left to right. The first tab is closed; after Cancel the remaining tabs stay and the refused tab is the active one. |
| 10 | With modified tabs: File > Exit, then the window's X button, then `Alt+F4` | Each asks for every modified tab. **Cancel** keeps the window open. Answering for all tabs closes the window. |
| 11 | `Ctrl+N` and `Ctrl+O` while the current tab is modified | No question: they add a tab and never touch the modified one. |
| 12 | Close the window when every tab is unmodified | It closes at once, without a dialog. |

## Edit menu

Use two tabs with some text. Check the menu with the mouse and the shortcuts with the keyboard.

| # | Steps | Expected |
|---|---|---|
| 1 | Open the Edit menu in a fresh empty tab | Undo, Redo, Cut, Copy, Delete are greyed out. Select All and Insert Date/Time are available. Paste is available only if the clipboard has text. |
| 2 | Type a word, open the Edit menu | Undo is available. Cut, Copy and Delete are still greyed out (no selection). |
| 3 | Select the word, open the Edit menu | Cut, Copy and Delete are now available. |
| 4 | Click Copy, move the caret, click Paste | The word is pasted. Paste works from text copied in another program too. |
| 5 | `Ctrl+Z`, then `Ctrl+Y` | The typing is undone, then redone. The Edit menu entries follow (Redo is available only after an undo). |
| 6 | Put the caret before a letter and press `Delete` | The next letter is removed. With a selection, the selection is removed. Edit > Delete with no selection is greyed out. |
| 7 | `Ctrl+A`, `Ctrl+X`, `Ctrl+V` | Everything is selected, cut, and pasted back. |
| 8 | Press `F5` | The time and short date are inserted at the caret, in the format of the Windows region settings (for example `3:45 PM 11/6/2026`). With text selected, the selection is replaced. |
| 9 | Type `at: `, press `F5`, then `Ctrl+Z` once | Only the date and time disappear; `at: ` stays. |
| 10 | Switch tabs and use Edit commands | They act on the active tab only. The menu state changes with the tab (for example Undo is greyed out in a tab with no edits). |
| 11 | Right-click in the editor | Qt's built-in context menu appears and works. (It is not the styled menu from the design yet.) |

## Error handling

| # | Steps | Expected |
|---|---|---|
| 4 | Open a file locked by another program (for example open in Excel) | An error dialog names the file. The editor keeps its current content. |
| 5 | Open a UTF-16 file and remove its last byte with a hex editor, then open it | The file is refused with an explanation, not opened with changed text. |

## Known gaps (do not report)

- All new tabs are named "Untitled", and Save As onto a file that is open in another tab gives two tabs for one file.
