# Manual Test Notes

> Automated tests cover the logic. These checks cover what they cannot: the real dialogs, the real window and the files as other programs see them. Run the relevant section before merging a branch that touches it. Record failures as issues.

Build and start the app from the repository root:

```powershell
cmake --build --preset debug
.\build\debug\src\app\scribe.exe
```

## File menu (Phase 1: New, Open, Save, Save As, Exit)

| # | Steps | Expected |
|---|---|---|
| 1 | Start the app | Title is `Untitled - Scribe`. Empty editor. |
| 2 | Type `hello`, look at the title | Title is `Untitled* - Scribe` (a `*` after the name). |
| 3 | `Ctrl+S` | A Save As dialog opens. Cancel it: nothing changes, the `*` stays. |
| 4 | `Ctrl+S`, choose `a.txt` on the Desktop | Title is `a.txt - Scribe`, no `*`. |
| 5 | Type more text, `Ctrl+S` | No dialog. The `*` disappears. Reopen `a.txt` in Notepad: it has the new text. |
| 6 | `Ctrl+Shift+S`, save as `b.txt` | Title is `b.txt - Scribe`. `a.txt` is unchanged. |
| 7 | `Ctrl+O`, pick `a.txt` | Content and title switch to `a.txt`. No `*`. |
| 8 | `Ctrl+O`, then Cancel | Nothing changes. |
| 9 | `Ctrl+N` | Empty `Untitled` document. |
| 10 | File > Exit | The window closes. |
| 11 | Open the File menu with the mouse and with `Alt+F` | Entries show their shortcuts. The mnemonics (`N`, `O`, `S`, `A`, `x`) work. |

## Encodings and line endings

Create the test files in Notepad (Save As, choose the encoding) or with PowerShell. Open each in Scribe, press `Ctrl+S` without editing, and compare the file bytes before and after (`Get-FileHash` must match).

| File | Expected |
|---|---|
| UTF-8, no BOM, with `é` and `€` | Text displays correctly. Hash unchanged after save. |
| UTF-8 with BOM | Same, and the BOM is still there after saving. |
| UTF-16 LE and UTF-16 BE (Notepad "Unicode" / "Unicode big endian") | Same. |
| ANSI with `é` (Notepad "ANSI") | Same. |
| LF-only and CR-only files | Open as multiple lines. Hash unchanged after save. |

## Error handling

| # | Steps | Expected |
|---|---|---|
| 1 | Open a file, delete it in Explorer, press `Ctrl+S` in Scribe | The file is written again, no error. |
| 2 | Open a read-only file (File properties > Read-only), edit, `Ctrl+S` | An error says the file could not be saved and the file on disk was not changed. The `*` stays. |
| 3 | Save As into a folder that needs administrator rights (`C:\Windows`) | Same error. Original content is still in the editor. |
| 4 | Open a file locked by another program (for example open in Excel) | An error dialog names the file. The editor keeps its current content. |
| 5 | Open a UTF-16 file and remove its last byte with a hex editor, then open it | The file is refused with an explanation, not opened with changed text. |

## Results log

| Date | Branch | Section | Result |
|---|---|---|---|
| 2026-11-05 | `feature/file-new-open-save` | File menu (1-11) | All pass |
| 2026-11-05 | `feature/file-new-open-save` | Error handling 1-3 | Pass |
| 2026-11-05 | `feature/file-new-open-save` | Error handling 4-5 | Not tested yet |
| 2026-11-05 | `feature/file-new-open-save` | Encodings and line endings | Not tested yet |

## Known gaps (do not report)

- Closing the window, New and Open discard unsaved edits without asking. This is fixed by the unsaved-changes prompt.
