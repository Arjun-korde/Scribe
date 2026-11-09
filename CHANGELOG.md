# Changelog

All notable user-visible changes to Scribe are listed here, newest first. The format follows
[Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Changed
- The unsaved-changes prompt follows the Figma design: a rounded card with a shadow instead of a
  system message box, "Save changes to <name>?", and the buttons Save (filled, the default), Don't
  save and Cancel. A very long file name is shortened in the middle. Esc and Alt+F4 still mean Cancel.
- Buttons in all dialogs follow the design: 28 px high, outlined, with the default button filled in
  the accent color, and hover, pressed, focus and disabled states.
- Menus follow the Figma design: rounded panels with an outline and shadow, 28 px rows, the shortcut
  muted on the right, a rounded hover fill, faded disabled rows, thin separators, and check marks and
  submenu arrows in the same style. This applies to the menu bar, Open Recent, and the pop-ups from
  the status bar.
- The main window follows the Figma design: light and dark themes that follow the Windows setting
  (and change while the app runs), a tab strip with left-aligned names, a muted close button, hover
  states and a `+` button, a line-number gutter with a highlighted current line, and a status bar
  with the cursor position and character count on the left and the encoding, line ending and zoom
  on the right.

## [0.1.0] - 2026-11-08

Release candidate: the automated tests and checks pass on both presets, but the manual tests in
`docs/05-manual-tests.md` have not been run yet for this release.

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
- View menu: Word Wrap, Font..., Zoom In / Out / Restore Default Zoom (`Ctrl++` or `Ctrl+=`, `Ctrl+-`,
  `Ctrl+0`, or Ctrl+mouse wheel), and a Status Bar toggle. Wrapping, font and zoom (10% to 500%) apply
  to every tab, including new ones. The editor starts in Cascadia Mono or Consolas at 11 pt.
- Find and Replace: `Ctrl+F` opens a bar under the editor (`Ctrl+H` adds the Replace row, `Esc` closes
  it). Typing jumps to the first match from the caret; `Enter` / `Shift+Enter` (or `F3` /
  `Shift+F3`, also with the bar closed) go to the next / previous match. Options: Match case and
  Wrap around (on by default). The bar shows "3 of 12", "No results" or "No more results", and the
  current match is highlighted even while you type in the bar. A one-line selection is used as the
  text to find. Replace replaces the current match and moves to the next; Replace All replaces
  every match as one undo step (one `Ctrl+Z` undoes it) and reports "12 replaced" in the bar.
  Matches are counted up to 100,000.
- Go To Line (`Ctrl+G`, in the new Search menu, or click the line/column in the status bar): a small
  dialog that opens on the current line. Only a number from 1 to the number of lines is accepted;
  anything else (for example 310 in a 300-line document) disables Go and shows "Enter a number from
  1 to 300", so Enter does nothing until the number is valid. The caret goes to the start of the
  line, and the view only scrolls if that line was off screen.
- Encoding menu: choose the encoding the document is saved in (UTF-8, UTF-8 with BOM, UTF-16 LE/BE
  with or without BOM, ANSI) and its line ending (Windows CRLF, Unix LF, Old Mac CR). The current
  choice is checked. Clicking the encoding or line ending in the status bar opens the same choices.
  This changes how the file is written on the next save; it does not re-read the file. Choosing an
  encoding that cannot store the text (for example ANSI with an emoji in it) is refused with an
  explanation, so a later save cannot fail for that reason.
- Status bar showing the line and column, the character count, the encoding, the line ending and the
  zoom of the active tab, updated as you type, move the caret or switch tabs.
- Drag and drop: drop one or more files from Explorer onto the tab strip or any editor to open them,
  one tab each, with the last one active. A file that is already open is switched to, and one that
  fails shows its own error. Folders are ignored, and dragging or dropping text still works.
- Remembered settings: word wrap, the font (family, size, bold, italic), the zoom, whether the
  status bar is shown, the Find options (Match case, Wrap around) and the window's size, position
  and maximized state are saved and restored the next time Scribe starts. A window that no longer
  fits the screen is shrunk and moved into view, and one that was on a monitor that is no longer
  connected opens at the default size and place, so it is never out of reach. Everything is kept in
  a plain, editable file,
  `%APPDATA%\Scribe\scribe.ini`. Changes are saved a moment after you stop making them (so turning
  the mouse wheel through many zoom steps writes once) and again when the window closes. A missing,
  damaged or hand-edited file never prevents startup: every bad value just falls back to its
  default, and if the file cannot be written Scribe carries on without saving. The text you search
  for is not remembered.
- File > Open Recent lists the last 10 files you opened or saved with Save As (newest first, with
  the keys `1`-`9` and `0`), and Clear Recent Files empties the list. A file counts when it is
  opened by File > Open, drag and drop or this menu, even if it was already open; a plain Save, a
  failed open and Untitled tabs do not count. The list is kept in `scribe.ini` like the other
  settings. Choosing a file that has been moved or deleted shows one message and removes it from
  the list; a file that exists but cannot be opened shows the usual error and stays listed.
- The window title shows the file name, with a `*` while there are unsaved changes.
- Text encoding detection for UTF-8 (with and without BOM), UTF-16 LE/BE and ANSI. Files are saved
  back in the encoding they were opened in, including the BOM.
- Line-ending detection (CRLF, LF, CR). Files are saved back with the same line ending.
- Saving never replaces a file with partly written data, and a file that could not be read back
  byte for byte is refused instead of being altered.
- Clear error messages when a file cannot be opened or saved, or when the text contains characters
  the file's encoding cannot store.

### Known limitations
- Find does not support regular expressions, whole-word matching, highlighting every match, or
  searching all tabs or files yet (planned for a later version). The search text is a single line,
  so text containing a line break cannot be searched, and case-insensitive matching uses simple
  case folding (for example German "ß" does not match "SS").
- An invalid line number in Go To Line is shown by text only for now; the red field border from the
  design comes with the theme.
- A file opened with the wrong encoding cannot be fixed from the Encoding menu: the menu changes how
  the file is saved, not how it was read. "Reopen with encoding" is planned for a later version.
- The status bar's zoom and character count are not clickable yet. The character count counts each
  line break as one.
- The font's underline and strikeout effects are not remembered between runs (only family, size,
  bold and italic are). A saved font that is no longer installed is replaced by the default one.
- Hand-typed values in the settings file that contain commas lose the spaces after the commas.
- Dropping files on the menu bar does not open them (only the tab strip and the editors do).
- Binary files (an `.exe`, an image) are not recognised: they open as garbage, or are refused if their
  bytes would change on save. A file that fails while several are dropped shows one error per file.
- Every unsaved tab is called "Untitled" (no numbering), and Save As to a file that is already open
  in another tab leaves two tabs for the same file.
- A file with mixed line endings is saved using its most common line ending.
- UTF-16 files without a BOM are only recognised for mostly Latin text.
