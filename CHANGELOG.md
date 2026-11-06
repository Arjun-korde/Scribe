# Changelog

All notable user-visible changes to Scribe are listed here, newest first. The format follows
[Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added
- File menu with New, Open, Save, Save As and Exit (`Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`).
- Tabs: New and Open add a tab (opening a file that is already open switches to it), `Ctrl+W` closes
  a tab, middle-click or the tab button also close it, and Close Other Tabs / Close All Tabs are in
  the File menu. `Ctrl+Tab` and `Ctrl+Shift+Tab` switch tabs. A tab shows `*` while it has unsaved
  changes, and the window title follows the active tab.
- Unsaved-changes prompt (Save / Don't Save / Cancel) when closing a modified tab, Close Other Tabs,
  Close All Tabs, or the window (Exit, the X button, `Alt+F4`). Cancel, a cancelled Save As or a failed
  save keeps everything open, so nothing is lost. `Ctrl+N` and `Ctrl+O` never ask.
- Edit menu: Undo, Redo, Cut, Copy, Paste, Delete, Select All, and Insert Date/Time (`F5`, time then
  short date, like Notepad). Entries are greyed out when they cannot run, for example Copy without a
  selection or Undo with nothing to undo, and they act on the active tab.
- Drag and drop: drop one or more files from Explorer onto the tab strip or any editor to open them,
  one tab each, with the last one active. A file that is already open is switched to, and one that
  fails shows its own error. Folders are ignored, and dragging or dropping text still works.
- The window title shows the file name, with a `*` while there are unsaved changes.
- Text encoding detection for UTF-8 (with and without BOM), UTF-16 LE/BE and ANSI. Files are saved
  back in the encoding they were opened in, including the BOM.
- Line-ending detection (CRLF, LF, CR). Files are saved back with the same line ending.
- Saving never replaces a file with partly written data, and a file that could not be read back
  byte for byte is refused instead of being altered.
- Clear error messages when a file cannot be opened or saved, or when the text contains characters
  the file's encoding cannot store.

### Known limitations
- Dropping files on the menu bar does not open them (only the tab strip and the editors do).
- Binary files (an `.exe`, an image) are not recognised: they open as garbage, or are refused if their
  bytes would change on save. A file that fails while several are dropped shows one error per file.
- Every unsaved tab is called "Untitled" (no numbering), and Save As to a file that is already open
  in another tab leaves two tabs for the same file.
- A file with mixed line endings is saved using its most common line ending.
- UTF-16 files without a BOM are only recognised for mostly Latin text.
