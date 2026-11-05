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

## Error handling

| # | Steps | Expected |
|---|---|---|
| 4 | Open a file locked by another program (for example open in Excel) | An error dialog names the file. The editor keeps its current content. |
| 5 | Open a UTF-16 file and remove its last byte with a hex editor, then open it | The file is refused with an explanation, not opened with changed text. |

## Known gaps (do not report)

- Closing a tab, Close Other Tabs, Close All Tabs and closing the window discard unsaved edits without asking. This is fixed by the unsaved-changes prompt.
- All new tabs are named "Untitled", and Save As onto a file that is open in another tab gives two tabs for one file.
