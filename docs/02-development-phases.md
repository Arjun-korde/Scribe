# Development Phases

> Working name: **Scribe** (placeholder). Each phase ends in a tagged, runnable release. Features marked **[stretch]** can slip to the next phase without blocking the release.

## Guiding Principles

1. Ship a working editor at every phase. Never leave `main` broken.
2. Every editor action is a **command** in a central registry from Phase 1, so macros, the command palette, and plugins come cheaply later.
3. Document access goes through `IDocumentBackend`, so large-file support can be added after v1 without rewrites.
4. Each phase adds tests for the logic it introduces.

## Phase Overview

| Phase | Release | Theme | Rough effort |
|---|---|---|---|
| 0 | none | Project setup and tooling | 2–3 days |
| 1 | v0.1 | Notepad parity | 1–2 weeks |
| 2 | v0.2 | Windows polish and reliability | 1–2 weeks |
| 3 | v0.3 | Power editing | 1–2 weeks |
| 4 | v0.4 | Automation: macros, snippets, tail mode | 1–2 weeks |
| 5 | v0.5 | Extensibility: plugins | 1–2 weeks |
| 6 | v1.0 | Speech-to-text, installer, release | 2–3 weeks |
| 7 | v1.x | Large-file support and further features | open-ended |

Effort assumes part-time work and can be adjusted.

---

## Phase 0: Project Setup

**Goal:** A repo that builds, tests, and runs CI before any feature work.

- [x] Git repo, `.gitignore`, README skeleton
- [ ] License (on hold)
- [x] CMake project with Qt 6, CMake presets (debug and release), vcpkg manifest
- [x] Empty `MainWindow` that launches
- [x] Qt Test and GoogleTest wired into CTest, with one passing test
- [x] `.clang-format`, `.clang-tidy`
- [x] GitHub Actions: build and test on `windows-latest`
- [x] Folder layout from the tech stack document

**Exit criteria:** CI is green on an empty app.

---

## Phase 1: Notepad Parity (v0.1)

**Goal:** Anything a user does in Windows Notepad works here.

**File**
- [x] New, Open, Save, Save As, Exit
- [x] Unsaved-changes prompt (Save / Don't Save / Cancel)
- [ ] Drag and drop to open files
- [ ] Recent files list
- [ ] Print and Page Setup **[stretch]**

**Tabs**
- [x] Tabbed documents, with modified indicator
- [x] New tab, close tab, Ctrl+Tab and Ctrl+Shift+Tab switching
- [x] Close other tabs / close all

**Edit**
- [ ] Undo, Redo, Cut, Copy, Paste, Delete, Select All
- [ ] Insert date/time (F5)

**Search**
- [ ] Find, Find Next, Find Previous
- [ ] Replace and Replace All
- [ ] Match case, wrap around
- [ ] Go To Line

**Format and View**
- [ ] Word wrap toggle
- [ ] Font selection
- [ ] Zoom: Ctrl+wheel, Ctrl+Plus, Ctrl+Minus, Ctrl+0
- [ ] Status bar: line and column, character count, zoom %, encoding, line ending

**Text correctness**
- [x] Encoding detection and handling: UTF-8 (with and without BOM), UTF-16 LE/BE, ANSI
- [x] Line-ending detection and preservation: CRLF, LF, CR
- [ ] Convert line endings; Save with a different encoding (logic done in `Document`, menu entries pending)
- [ ] Core architecture: `Document`, `IDocumentBackend` (in-memory implementation), `CommandRegistry` (done), `SettingsManager` (pending)

**Tests:** encoding detection, line-ending detection and conversion, document load/save round trips.

**Exit criteria:** Can replace Notepad for everyday editing. No data loss when saving any supported encoding.

---

## Phase 2: Windows Polish and Reliability (v0.2)

**Goal:** Feel like a native, trustworthy Windows application.

- [ ] Dark and light themes, following the Windows setting
- [ ] Dark title bar through DWM (`DWMWA_USE_IMMERSIVE_DARK_MODE`)
- [ ] Per-monitor High-DPI correctness
- [ ] Single instance: second launch sends its files to the running window
- [ ] Command-line arguments: `scribe file.txt +42` (open at line 42)
- [ ] Session restore: reopen tabs, cursor positions, and unsaved content on next launch
- [ ] Autosave and crash recovery (periodic snapshots to `%LOCALAPPDATA%`)
- [ ] External change detection (`QFileSystemWatcher`): prompt to reload
- [ ] Read-only and locked file handling with a "Save as" fallback
- [ ] Persist window size, position, and preferences
- [ ] Settings dialog (font, theme, defaults for encoding and line endings)
- [ ] Reopen with encoding **[stretch]**

**Tests:** session save/restore, crash-recovery file handling, command-line parsing.

**Exit criteria:** Kill the process during typing, relaunch, and no text is lost.

---

## Phase 3: Power Editing (v0.3)

**Goal:** Go beyond Notepad with the features people miss most.

**Search**
- [ ] Regex find and replace, whole-word option
- [ ] Highlight all matches
- [ ] Find in all open tabs
- [ ] Find in files (folder search, background thread, results panel)

**Editing**
- [ ] Line operations: duplicate, delete, move up/down, join
- [ ] Sort lines, remove duplicate lines, trim trailing whitespace
- [ ] Case conversion (upper, lower, title)
- [ ] Column (block) selection with Alt+drag **[stretch]**
- [ ] Show whitespace and tab characters
- [ ] Bookmarks

**View**
- [ ] Line numbers gutter, current-line highlight
- [ ] Split view of one document **[stretch]**

**Productivity**
- [ ] Command palette (Ctrl+Shift+P) built on `CommandRegistry`
- [ ] Quick tab switcher (Ctrl+P)
- [ ] Configurable keyboard shortcuts
- [ ] Statistics dialog (words, characters, lines)

**Tests:** find/replace engine (including regex edge cases), line operations, sorting.

**Exit criteria:** All editing features are available as registered commands and appear in the command palette.

---

## Phase 4: Automation: Macros, Snippets, Tail Mode (v0.4)

**Goal:** Features that reduce repetitive work and help with logs.

**Macros**
- [ ] Record, stop, and replay command sequences
- [ ] Replay N times or until end of file
- [ ] Save, name, and delete macros; assign shortcuts
- [ ] Macros stored as JSON

**Snippets**
- [ ] Snippet definitions in editable JSON
- [ ] Trigger by text plus Tab
- [ ] Placeholders and tab stops: `${1:name}`
- [ ] Variables: `${DATE}`, `${TIME}`, `${FILENAME}`, `${CLIPBOARD}`
- [ ] Snippet management dialog

**Tail mode**
- [ ] Follow a growing file (incremental reads from the last offset)
- [ ] Auto-scroll when at the bottom, pause when the user scrolls up, "Resume" button
- [ ] Handle log rotation and truncation
- [ ] Do not show partial last lines
- [ ] Open with shared read/write/delete access (Win32 `CreateFile`) so writers are not blocked
- [ ] Line limit to cap memory
- [ ] Highlight ERROR/WARN lines; filter box **[stretch]**

**Tests:** macro record/replay determinism, snippet expansion, tail reader (append, truncate, partial lines).

**Exit criteria:** Tail a log being written by another process for 30 minutes with stable memory and no UI freezes.

---

## Phase 5: Extensibility: Plugins (v0.5)

**Goal:** Others can add features without changing the editor's source.

- [ ] `IPlugin` interface with versioning
- [ ] `IEditorHost` API exposed to plugins: current text, selection, replace, add menu action, register command, show message, document events
- [ ] `PluginManager` using `QPluginLoader`, loading from the `plugins/` folder
- [ ] Plugin enable/disable UI and error handling for failed loads
- [ ] Refactor a few built-in features to use `IEditorHost` to prove the API
- [ ] Sample plugins: JSON formatter, Base64 encode/decode, word count, hash calculator
- [ ] Plugin developer guide in `docs/`
- [ ] Theme files (JSON) loadable without recompiling

**Tests:** plugin loading and unloading, API contract tests with a mock host.

**Exit criteria:** A sample plugin built separately can be dropped into the folder and appears in the menu.

---

## Phase 6: Speech-to-Text, Installer, Release (v1.0)

**Goal:** Differentiating AI feature and a polished first release.

**Speech-to-text (offline, whisper.cpp)**
- [ ] `ISpeechEngine` interface, `WhisperEngine` implementation
- [ ] Microphone capture with Qt Multimedia (16 kHz mono)
- [ ] Voice activity detection to split speech segments
- [ ] Transcription on a worker thread; text inserted at the cursor as a command
- [ ] Push-to-talk or toggle hotkey, microphone indicator in the status bar
- [ ] Model manager: download on first use, choose model size, show progress
- [ ] Language selection
- [ ] Voice commands ("new line", "full stop", "delete that") mapped to commands **[stretch]**
- [ ] GPU acceleration (Vulkan or CUDA) **[stretch]**

**Windows integration**
- [ ] File associations (`.txt`, `.log`, and others)
- [ ] Explorer context menu: "Edit with Scribe"
- [ ] Jump list with recent files
- [ ] Taskbar integration

**Release**
- [ ] Inno Setup installer (per-user and per-machine options)
- [ ] `windeployqt` packaging, portable ZIP build
- [ ] GitHub Releases with changelog
- [ ] README with screenshots/GIFs, architecture diagram, build instructions
- [ ] Performance numbers and test coverage recorded in the README
- [ ] Code signing **[stretch]**

**Exit criteria:** A new user can download the installer, dictate a paragraph, and uninstall cleanly.

---

## Phase 7: Large Files and Beyond (v1.x)

**Large-file support**
- [ ] New `IDocumentBackend` using memory-mapped, chunked or piece-table storage
- [ ] Read-only mode above a size threshold, then editing support
- [ ] Custom viewport rendering for huge files
- [ ] Fast search across large files
- [ ] Target: open a 1 GB file in under a second

**Other candidates (prioritise later)**
- [ ] Hex view for binary files
- [ ] Diff / compare two tabs
- [ ] Multi-cursor editing
- [ ] Minimap
- [ ] Spell check
- [ ] Syntax highlighting for common formats (JSON, XML, Markdown, log levels)
- [ ] Out-of-process plugins for crash isolation
- [ ] Auto-update mechanism
- [ ] Localization (Qt Linguist)

---

## Definition of Done (every feature)

1. Implemented as a registered command where it is an action.
2. Unit tests for the logic; manual test notes for the UI.
3. Passes CI, including `clang-tidy` and formatting checks.
4. No regressions in earlier phases' exit criteria.
5. Documented in the README or `docs/`, and added to the changelog.

## Risks and Mitigations

| Risk | Mitigation |
|---|---|
| Encoding bugs corrupting user files | Round-trip tests with sample files in every supported encoding, written early in Phase 1 |
| Scope creep | Phase gates; **[stretch]** items may slip |
| Tail mode and file watchers behaving differently on network drives | Timer-based polling fallback |
| Whisper too slow on weak CPUs | Smaller default model, clear model selector, optional GPU build |
| Plugin ABI breakage | Versioned interface, plugins built with the same compiler and Qt version |
| Large files require architecture changes | `IDocumentBackend` abstraction from Phase 1 |
