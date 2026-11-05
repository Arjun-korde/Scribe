# Changelog

All notable user-visible changes to Scribe are listed here, newest first. The format follows
[Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added
- File menu with New, Open, Save, Save As and Exit (`Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`).
- The window title shows the file name, with a `*` while there are unsaved changes.
- Text encoding detection for UTF-8 (with and without BOM), UTF-16 LE/BE and ANSI. Files are saved
  back in the encoding they were opened in, including the BOM.
- Line-ending detection (CRLF, LF, CR). Files are saved back with the same line ending.
- Saving never replaces a file with partly written data, and a file that could not be read back
  byte for byte is refused instead of being altered.
- Clear error messages when a file cannot be opened or saved, or when the text contains characters
  the file's encoding cannot store.

### Known limitations
- Closing the window, New and Open do not yet ask about unsaved changes.
- A file with mixed line endings is saved using its most common line ending.
- UTF-16 files without a BOM are only recognised for mostly Latin text.
