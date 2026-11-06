# UI / UX Design

> Working name: **Scribe**. Target: Windows 10/11 desktop, Qt 6.7+ Widgets. Input: mouse and keyboard first, touch/pen tolerated. Status: **draft v0.3**, decisions are listed in section 9 (toolbar still open). Figma status and pending work are in section 12.

## 1. Principles

1. **Text first.** The editor gets about 90% of the window. Chrome is quiet and neutral.
2. **Native feel.** Windows 11 conventions (Fluent-inspired), system font for chrome, DWM dark title bar.
3. **One accent color**, used only for things you can interact with or focus.
4. **Keyboard complete.** Every action is a command (see `CommandRegistry`) reachable by shortcut and the command palette.
5. **Progressive disclosure.** Menus and the palette hold the long tail; the default window shows only tabs, text and status.
6. **Never lose work.** Destructive actions confirm or are undoable; session restore is silent.

## 2. Main window layout

```
+--------------------------------------------------------------+
| [icon] File Edit Search View Encoding Language Help [mic] - [] x|  Title bar (32 px)
+--------------------------------------------------------------+
| notes.txt  x | todo.md  *x | untitled 3  x |  +               |  Tab strip (36 px)
+--------------------------------------------------------------+
|  1 | Lorem ipsum dolor sit amet, consectetur adipiscing      |
|  2 | elit, sed do eiusmod tempor incididunt ut labore        |
|  3 |                                                         |  Editor
|  . |                                                         |  (gutter + text)
+--------------------------------------------------------------+
| Find: [__________] [Aa] [.*] [W]  3 of 12  [<] [>]  [x]      |  Find bar (optional, 40 px)
+--------------------------------------------------------------+
| Ln 3, Col 14 | 1,204 chars | UTF-8 | CRLF | 100% | Plain Text|  Status bar (26 px)
+--------------------------------------------------------------+
```

- **Title bar (VS Code style):** app icon, then the menus, then free drag area, then the microphone control, then the window controls. There is no app name in the bar. The window title (file name) goes to the taskbar and `Alt+Tab` only. The bar is a custom (frameless) title bar, see decision 6 in section 9.
- **Narrow windows (spec):**
  - Below about 900 px the menus collapse into one `...` overflow button (same items, same shortcuts).
  - Below about 640 px the microphone control shows the icon and level meter only, no label.
  - Below about 480 px it shows the icon only (the level meter moves to the tooltip and the status bar).
  - The drag area never shrinks below 48 px, so the window can always be moved.
- **Tabs:** modified marker `*` plus a close button (the marker is not color-only). Middle-click closes. Ctrl+Tab cycles; Ctrl+P opens the quick switcher (Phase 3).
- **Tab overflow (spec):** tabs shrink from 168 px to a minimum of 96 px. After that the strip scrolls, with `<` and `>` buttons at the edges and a "list all tabs" dropdown at the right end. The active tab is always scrolled into view. Tab names truncate in the middle so the extension stays visible.
- **Gutter:** line numbers (toggle in View). Muted text color, no background fill.
- **Find bar:** inline at the bottom, not a modal dialog. Esc closes it. Replace row expands below on Ctrl+H.
- **Status bar:** every segment is clickable (encoding, EOL, language, zoom, Ln/Col opens Go to line).
- **Empty state:** a new untitled tab, no splash screen.

## 3. Screens and dialogs

| Screen | Pattern | Notes |
|---|---|---|
| Find / Replace | Inline bar | Match case, whole word, regex toggles; live match count; no match shown as an error icon plus text |
| Go to line | Small modal | One field, Enter confirms, Esc cancels |
| Command palette (Ctrl+Shift+P) | Centered overlay, 560 px wide | Type to filter, arrows to move, Enter runs, shows shortcut at right |
| Settings | Single dialog, left category list | Categories: General, Editor, Appearance, Files, Shortcuts |
| Unsaved changes | Standard 3-button dialog | Save / Don't save / Cancel; Save is the default |
| File changed on disk | Non-modal banner above editor | "Reload" and "Keep mine"; never silently overwrite |
| Encoding mismatch / binary file | Banner | Offers "Reopen with encoding..." |
| About | Small modal | Version, license, Qt version, whisper.cpp version |
| Editor context menu | Native-style popup, rows 28 px | Cut, Copy, Paste, Select All, separator, Start dictation (Ctrl+Shift+D). Disabled rows stay visible at 40% opacity |
| Tab context menu | Popup | Close, Close Others, Close to the Right, Close All, separator, Copy Path, Open Containing Folder |
| Large file open | Inline progress in the editor area | Progress bar, bytes read, Cancel. Shown only when opening takes more than 1 s |
| Voice setup (first run) | Small modal | See section 10 |
| Microphone blocked | Banner | See section 10 |

## 4. Color tokens

Names are role-based. Values passed a contrast check (section 4.1; target: text 4.5:1, UI components 3:1).

| Token | Light | Dark | Use |
|---|---|---|---|
| `surface` | `#FFFFFF` | `#1E1E1E` | Editor background |
| `surfaceChrome` | `#F3F3F3` | `#2B2B2B` | Tab strip, status bar, find bar |
| `outline` | `#E0E0E0` | `#3A3A3A` | Dividers and decorative borders (dialogs, popups, buttons that also have a text label) |
| `outlineStrong` | `#858585` | `#858585` | Borders that are the only thing marking a control: inputs, unselected toggles and segments |
| `textPrimary` | `#1B1B1B` | `#E6E6E6` | Body and editor text |
| `textMuted` | `#5C5C5C` | `#A0A0A0` | Gutter, status bar, hints |
| `accent` | `#0F6CBD` | `#60CDFF` | Focus ring, primary button, links, toggled find options |
| `selection` | `#CCE4F7` | `#264F78` | Text selection |
| `currentLine` | `#F5F5F5` | `#262626` | Current line highlight |
| `error` | `#C42B1C` | `#FF99A4` | Errors (always paired with icon or text) |
| `warning` | `#9D5D00` | `#FCE100` | Warnings, "too loud" microphone level |
| `success` | `#0F7B0F` | `#6CCB5F` | Success |
| `hover` | `#E9E9E9` | `#353535` | Hover fill for neutral controls, tabs, menu rows |
| `pressed` | `#DDDDDD` | `#444444` | Pressed fill for neutral controls |
| `accentHover` | `#115EA3` | `#7BD6FF` | Hover fill for accent-filled buttons |
| `accentPressed` | `#0C4B86` | `#4DBAEB` | Pressed fill for accent-filled buttons |

Rules:
- `accent` is never decorative.
- Color is never the only signal: modified tab = `*`, error = icon + message, match count = text.
- The Theme is a data table loaded into `QPalette` plus a QSS template, so user themes (JSON) need no code changes.
- Text on a `selection` fill (selected toggles and segments) uses `textPrimary`, not `accent`; the accent shows in the border. Accent text on the light selection color is only 4.1:1.
- Control borders that identify a control use `outlineStrong`; `outline` is only for dividers and decoration.

### 4.1 Contrast check (WCAG 2.x, calculated)

Every text and control pair used in the design was calculated. The tokens above are the result of fixing the failures:

| Failure found | Fix |
|---|---|
| Light: `accent` text on `selection` was 4.10:1 | Selected labels use `textPrimary` (13.1:1); accent stays on the border |
| Dark: `textMuted` on `hover` was 4.48:1 | Dark `hover` changed from `#383838` to `#353535` (4.69:1) |
| Both: `outline` on surfaces was 1.2 to 1.5:1, too faint for control borders | New `outlineStrong` `#858585`: 3.7:1 on `surface`, 3.3:1 on `surfaceChrome` (light); 4.5:1 and 3.8:1 (dark) |

Lowest ratios that now pass (text needs 4.5:1):

| Pair | Light | Dark |
|---|---|---|
| `textPrimary` on any surface or fill | 12.7 | 6.8 |
| `textMuted` on `surfaceChrome` / `hover` | 6.0 / 5.5 | 5.4 / 4.7 |
| `accent` on `surface` / `surfaceChrome` | 5.4 / 4.9 | 9.3 / 7.9 |
| `surface` text on `accent`, `accentHover`, `accentPressed` | 5.4 | 7.6 |
| `error`, `warning`, `success` on `surfaceChrome` | 5.1, 4.7, 4.9 | 7.0, 10.7, 7.0 |

Not required to pass: disabled controls (40% opacity) and decorative dividers.

### 4.2 Syntax highlighting palette

Eight tokens per theme (`syntax/<name>` in Figma). Each is at least 4.5:1 on both `surface` and `currentLine`. Operators, punctuation and identifiers use `textPrimary`. Inside a text selection all text is drawn in `textPrimary` (Qt `HighlightedText`), so syntax colors never sit on the selection fill.

| Token | Light | Dark | Style | Used for |
|---|---|---|---|---|
| `syntax/keyword` | `#8E24AA` | `#D291D0` | Regular | Keywords, preprocessor |
| `syntax/string` | `#A31515` | `#E0A184` | Regular | String and character literals |
| `syntax/number` | `#0B7A53` | `#B5CEA8` | Regular | Numbers |
| `syntax/comment` | `#5F6B76` | `#7FA66E` | Italic | Comments |
| `syntax/function` | `#7A5A1E` | `#E3D79F` | Regular | Function and method names at call sites |
| `syntax/type` | `#1F7A8C` | `#56D4BD` | Regular | Types and classes |
| `syntax/constant` | `#0B63B5` | `#6CB6F0` | Regular | Enum values, constants, booleans |
| `syntax/tag` | `#C2410C` | `#F0907C` | Semibold for Markdown headings | Markup tags |

Lowest ratio on `surface`: light 4.98:1 (`type`), dark 6.02:1 (`comment`). Color is not the only cue: comments are italic and headings are semibold. Language support (Phase 2) maps each language's token types onto these eight names; users can override them in a theme file.

## 5. Typography

| Role | Font | Size |
|---|---|---|
| Chrome (menus, tabs, dialogs) | Segoe UI Variable (system) | 9 pt (about 12 px) menus and status, 10 pt (about 13 px) tabs and dialogs |
| Editor | Cascadia Mono, fallback Consolas | 11 pt default, user-configurable, Ctrl+wheel zoom |
| Dialog title | Segoe UI Variable Semibold | 12 pt |

- Use `pt` sizes so OS scaling is respected.
- Editor line height 1.4. Weights: Regular and Semibold only.
- Test at 150% scaling and with OS text size set to Large.

## 6. Spacing and shape

- Spacing scale: 4, 8, 12, 16, 24 px. No other values.
- Corner radius: 4 px controls, 8 px dialogs and the palette.
- Borders: 1 px `outline`. No shadows on in-window elements. Overlays (palette) may use the OS-style drop shadow.
- Minimum click target for toolbar-style controls: 28 x 28 px. Tab close button: 20 x 20 px with a 28 px hit area.

## 7. Interaction and motion

- Feedback within 100 ms. Anything longer than 1 s shows progress (large file open, tail mode load).
- Animations only where they explain state: palette fade-in 120 ms, banner slide 200 ms. None above 300 ms.
- Respect the Windows "Animation effects" setting; when off, transitions are instant.
- Focus: a 2 px `accent` ring on every focusable control, never removed. Exception: on accent-filled buttons an accent ring is invisible, so those use a 2 px `textPrimary` ring.
- Control states: Default, Hover, Pressed, Focus, Disabled (40% opacity, not clickable, still readable). Tooltips appear after 500 ms of hover or on keyboard focus, and disappear on Esc.
- Tab order follows reading order. Esc always closes the topmost overlay.

## 8. Accessibility and localization

- Set `accessibleName` on every icon-only control (close tab, find options).
- Tooltips supplement, never replace, labels or shortcuts shown in menus.
- Dark and light themes follow the OS setting, with a manual override in Settings.
- All strings go through `tr()`. Containers grow to fit 40% longer text. Numbers and dates use `QLocale`.
- RTL: out of scope for v1, but avoid hardcoded left/right assumptions in layouts.
- Voice input (see section 10 for the states):
  - Announce state changes to screen readers with a Qt accessibility alert event: "Dictation started", "Dictation stopped", "Transcribing", and the error messages. Do not announce each inserted phrase; the screen reader reads the editor text itself.
  - The level meter has `accessibleName` "Microphone level", role progress bar, and a value from 0 to 100. Update the accessible value at most twice a second, even though the visual meter updates about 15 times a second.
  - The microphone control is a toggle button with `accessibleName` "Dictation" and a checked state. Its tooltip shows the shortcut.
  - With "Animation effects" off, the meter still shows the level (it is data, not decoration) but without smoothing.

## 9. Decisions

| # | Topic | Decision |
|---|---|---|
| 1 | Qt version | **6.7+**, built-in `windows11` style as the base; tokens are layered on top via `QPalette` and QSS |
| 2 | Accent hue | **Fluent blue** (`#0F6CBD` / `#60CDFF`) for now. It is a single token, so a brand color can replace it later without code changes |
| 3 | Tab style | **Windows 11 Notepad style**: rounded top corners (4 px), the active tab uses the `surface` color and joins the editor, inactive tabs sit on `surfaceChrome`. No underline |
| 4 | Menu bar | **Classic**, always visible |
| 5 | Toolbar | **Undecided.** Default is none. Revisit after the first prototype |
| 6 | Title bar | **Custom (frameless) title bar** with app icon, menus and microphone control inside it. The layout in section 2 cannot be built with the native title bar. Implementation notes below |
| 7 | Speech engine | **Offline whisper.cpp** (see `01-tech-stack-and-setup.md`). No cloud option, so no online/offline indicator and no account UI |
| 8 | Microphone control placement | Right side of the title bar, before the window controls. The status bar shows the language only |

**Decision 6, implementation notes.** A custom title bar in Qt Widgets means we handle what Windows normally does for us:
- Keep the native resize border and shadow by extending the frame into the client area (`DwmExtendFrameIntoClientArea`) and handling `WM_NCCALCSIZE` so the client area covers the title bar.
- Answer `WM_NCHITTEST` so the drag area reports caption, the window buttons report their button codes (this keeps Windows 11 snap layouts on the maximize button), and the menus and microphone control stay clickable.
- Draw our own minimize, maximize and close buttons using the tokens (close turns red on hover), or keep the system ones if the spike shows they can be kept.
- Double-click on the drag area maximizes; right-click shows the system menu; dragging to a screen edge snaps.
- **Fallback if the spike is too costly:** keep the native title bar, put the menu bar under it, and put the microphone control at the right end of the menu bar (`QMenuBar::setCornerWidget`). Nothing else in the design changes.
- Do a one-day spike before Phase 1 styling work, see section 12.

## 10. Speech to text (offline, whisper.cpp)

How whisper behaves drives the UI. It transcribes whole speech segments after a pause, not word by word. So there is no live word-by-word text. While the user speaks, the level meter shows input; after a pause, the editor shows a muted `...` placeholder at the cursor until the segment is ready, then the text replaces it. Whisper adds punctuation itself, so there is no "insert punctuation" setting.

### 10.1 Microphone control states (title bar)

| State | Look | Notes |
|---|---|---|
| Idle | Mic icon, no fill | Tooltip: "Start dictation (Ctrl+Shift+D)" |
| Setup needed | Mic icon with a dot | Click opens Voice setup (10.3) |
| Listening | Accent fill, mic icon, 5-bar level meter, label "Listening" | Meter updates about 15 times a second |
| Transcribing | Outlined, label "Transcribing..." | Shown while a segment is being processed; the user can keep speaking |
| Paused | Outlined, muted, flat meter | Dictation is on but no input detected for 2 s |
| No microphone | Outlined `error`, crossed mic icon, label "No microphone" | Icon plus text, not color alone |
| Blocked by Windows | Outlined `error`, crossed mic icon, label "Mic blocked" | Opens the blocked banner (10.2) |

Level meter: silence is 5 flat muted bars; normal speech uses the accent color; when the input clips, the top bar and a "Too loud" label use `warning`.

Controls: click or `Ctrl+Shift+D` toggles. Mode setting: "Toggle" (default) or "Hold to talk" (hold the shortcut). `Esc` stops dictation and discards any segment still being transcribed.

### 10.2 Microphone permission (Windows privacy)

Windows can block microphone access for desktop apps (Settings > Privacy & security > Microphone). This is a different failure from having no microphone, so it has its own state and its own recovery.

- Detect it when capture starts (no audio device access, or the OS access status is denied).
- The control switches to **Mic blocked** and a non-modal banner appears above the editor: "Scribe can't use your microphone. Windows is blocking microphone access for desktop apps." Buttons: **Open Windows settings** (opens `ms-settings:privacy-microphone`) and **Dismiss**.
- When the user returns to Scribe, re-check access automatically (on window activation). If it is now allowed, the banner closes and the control returns to Idle, with no restart needed.
- Not blocked but no device present: **No microphone** state, banner text "No microphone found. Connect one and try again", with a **Try again** button.
- Device removed mid-dictation: stop dictation, keep all text already inserted, show the No microphone state.

### 10.3 First run: Voice setup and model download

Models are not shipped with the installer. They are downloaded on first use into `%LOCALAPPDATA%\Scribe\models`.

1. User clicks the microphone control for the first time. A small modal opens: **Set up voice input**.
2. Content: one line saying audio is processed on this computer and never leaves it; a model choice (Tiny, Base, Small) with download size and a speed/accuracy hint; the default is the smallest model that is usable ("Base" proposed); a language choice.
3. Buttons: **Download** (primary) and **Not now**.
4. While downloading: progress bar, bytes done and total, **Cancel**. The editor stays usable. The microphone control shows "Downloading model 42%".
5. On finish: verify the file (checksum), then check microphone access (10.2), then start listening. If the download fails: error text with **Retry**; partial files are removed.
6. "Not now" leaves the control in the Setup needed state.

Settings > Speech has the **model manager**: a list of models with size, status (Installed, Download, Downloading), and **Delete** for installed ones; the active model is marked; the storage path is shown with an **Open folder** button.

### 10.4 Settings > Speech page

Microphone (device picker plus a live level test), language, model (links to the model manager), mode (Toggle or Hold to talk), shortcut, and "Stop automatically after 5 seconds of silence" (off by default).

## 11. Open questions

- Default model size: Base or Small? Needs a speed test on a low-end CPU.
- Should dictated text be marked in the undo stack as one step per segment (proposed) or one step per session?
- Are voice commands ("new line", "full stop", "delete that") in v1? They are a stretch goal in the phases doc, so no UI is designed for them yet.

## 12. Design status and pending work

**Figma file:** https://www.figma.com/design/ArJI49RD77otKfQ4WJxfZ8 ("Scribe UI Design"). All sheets exist in light and dark.

Done in Figma:
- **Foundations:** color variables in `Scribe/Light` and `Scribe/Dark` (two collections, because the Figma Starter plan allows one mode per collection), `Scribe/Spacing` (space and radius values), and 6 text styles.
- **Main window:** title bar with app icon, menus and microphone control (listening), tabs, editor with the `...` transcribing placeholder, status bar. A second frame per theme shows the find and replace bar open.
- **Title bar responsive:** full, menus collapsed to `...`, mic without label, mic icon only.
- **Overlays:** find bar, no-match state, file-changed banner, command palette, unsaved-changes dialog, Settings (Appearance).
- **Components:** Button, Input, Tab, Icon Button, Tooltip, Menu Item, each with its states (hover, pressed, focus, disabled, error where it applies). The new sheets below are built from these.
- **Menus, states and dialogs:** editor and tab context menus, tooltips in context, tab overflow, Go to line, About, large file progress, encoding mismatch banner.
- **Voice input:** microphone control states (idle, setup needed, listening, transcribing, paused, downloading, no microphone, blocked by Windows), level meter levels, the editor placeholder, mic blocked and no microphone banners, Voice setup (first run), model download and failure state, Settings > Speech with the model manager, accessibility notes.

Pending in Figma:
- [ ] The main window and Overlays frames (built earlier) are plain frames, not instances of the components. Rebuild them from the components so token and state changes propagate.
- [ ] Light and Dark components are separate sets. Merge into one set with two modes when the plan allows it.
- [ ] Visual check of the dark versions of the newer sheets (menus, voice, find open, title bar responsive). They use the same code as the light ones, which were checked.
- [ ] Window-control glyphs are text characters in the mock; replace with real icons.
- [ ] The tab context menu and editor context menu have no submenu designs (not needed yet).

Also done in Figma: contrast fixes applied to the variables and existing frames (`outlineStrong`, dark `hover`, selected labels), the `syntax/*` variables and a Syntax highlighting sheet per theme (sample plus token table with ratios).

Pending design decisions and checks:
- [ ] Final app name. "Scribe" is a working name; check availability (trademark, GitHub, winget) before the release phase
- [ ] Syntax palette is only checked by numbers; review it on real code in the prototype and adjust
- [ ] Inter stands in for Segoe UI in Figma (Segoe UI is not available there)
- [ ] Toolbar decision (decision 5)
- [ ] Test at 150% and 200% scaling and with large OS text
- [ ] Title bar spike (decision 6) before styling work starts
- [ ] Default model size (section 11)

## 13. Next steps

1. Do the title bar spike (decision 6).
2. Decide the toolbar question (decision 5) and the final app name.
3. Rebuild the early frames from components and merge the light and dark component sets when possible.
4. Implement `Theme` (tokens to `QPalette` and QSS), then the title bar, tab strip and status bar as the first styled components.
