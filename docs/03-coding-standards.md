# Coding Standards and Rules

> Applies to all code in the Scribe repository. Where this document and a tool (`clang-format`, `clang-tidy`) disagree, fix the tool configuration or this document so they match.

## 1. Language and Build

- C++20. No compiler extensions (`CMAKE_CXX_EXTENSIONS OFF`).
- Build with CMake presets only (`cmake --preset debug`). No qmake, no IDE-only project settings.
- Warnings are errors in CI. New code must compile without warnings on MSVC.
- Use separate build directories per configuration (`build/debug`, `build/release`).
- Plain C arrays are fine for small fixed lists, such as test data. Do not wrap them in `std::array` only to satisfy a tool; `modernize-avoid-c-arrays` is turned off in `.clang-tidy`.

## 2. Ownership and Pointers

- **No raw `new` / `delete`** outside Qt parent-child ownership.
- Objects that are not QObjects and have a single owner: `std::unique_ptr`, created with `std::make_unique`.
- Shared ownership only when truly needed: `std::shared_ptr`, created with `std::make_shared`. Justify it in a comment.
- QObjects: pass a parent so Qt owns them. A QObject with no parent must be held by a smart pointer.
- Prefer heap objects held by smart pointers and accessed with `->` over large stack objects when the object is a long-lived top-level one (for example the main window: `window->show()`).
- Raw pointers are **non-owning observers** only. Never `delete` through them.
- Use `nullptr`, never `NULL` or `0`.
- Pass by `const T&` for read-only input, by value when the callee needs a copy, and by pointer only when `nullptr` is a valid value.
- Never keep a pointer or reference to an object that may be destroyed first. Use `QPointer` for QObjects that can be deleted elsewhere.

## 3. Naming

| Item | Style | Example |
|---|---|---|
| Classes, structs, enums | `PascalCase` | `MainWindow`, `LineEnding` |
| Interfaces | `I` prefix + `PascalCase` | `IDocumentBackend` |
| Functions and methods | `camelCase` | `openFile()` |
| Local variables, parameters | `camelCase` | `fileName` |
| Member variables | `m_` prefix + `camelCase` | `m_document` |
| Constants and enumerators | `PascalCase` (enumerators) / `kPascalCase` (constants) | `Encoding::Utf8`, `kMaxRecentFiles` |
| Namespaces | `lowercase` | `scribe::core` |
| Files | match the class name | `MainWindow.h`, `MainWindow.cpp` |
| Macros | `UPPER_SNAKE_CASE`, avoid macros | `PROJECT_VERSION` |

- Signals and slots use verb phrases: `fileOpened`, `onSaveTriggered`.
- Test files are named `<Subject>Test.cpp`.

## 4. Formatting

- `clang-format` with the repository `.clang-format`. Run it before every commit.
- 4 spaces, no tabs. Braces on their own line for classes, functions and namespaces.
- Maximum line length 100 characters.
- One class per header/source pair. Use `#pragma once`.
- Include order: matching header, project headers, Qt headers, standard library headers. Separate groups with a blank line.

## 5. Architecture Rules

- Code in `core/` has **no widget dependencies**, so it can be unit tested.
- Every editor action is a **command** registered in `CommandRegistry`. Menus, shortcuts, macros, the command palette and plugins all invoke commands.
- Document access goes through `IDocumentBackend`. Do not read document text from the widget directly in business logic.
- Public interfaces (`IDocumentBackend`, `IPlugin`, `IEditorHost`, `ISpeechEngine`) are small, documented, and versioned where they cross a DLL boundary.
- Platform-specific code (Win32, DWM, shell integration) lives in `platform/win/` behind a small interface.
- No global mutable state. Pass dependencies in through constructors.

## 6. Qt Conventions

- Use the new-style `connect(sender, &Class::signal, receiver, &Class::slot)`. No `SIGNAL()` / `SLOT()` macros.
- Wrap user-visible strings in `tr()`.
- Use `QStringLiteral` / `u"..."_s` for string constants. Avoid implicit `const char*` to `QString` conversions.
- Never block the GUI thread: file scans, searches in files, and transcription run on worker threads and report back with signals.
- Do not call UI code from worker threads.
- Define `Q_OBJECT` only on classes that use signals, slots or properties.

## 7. Error Handling

- **Never lose user data.** Failures while saving must leave the original file intact (write to a temporary file, then replace).
- Do not use exceptions across Qt signal/slot boundaries. Return `std::optional` or a small result type for expected failures.
- Report errors to the user with a clear message; log technical detail with `qCWarning` and a logging category.
- Check every file operation and Win32 call result.

## 8. Comments and Documentation

- Comment *why*, not *what*. Do not restate the code.
- Public headers carry a short description of each class and non-obvious function.
- Keep `docs/` in step with behaviour changes. Add user-visible changes to the changelog.

## 9. Testing

- Logic added in a phase comes with tests in the same change.
- Pure C++ logic: GoogleTest. Qt classes and UI behaviour: Qt Test. Both run through CTest.
- Encoding and line-ending code needs round-trip tests for every supported encoding.
- Tests must be deterministic: no reliance on the clock, network, or other tests' side effects.
- Do not commit changes that break an earlier phase's exit criteria.

## 10. Git and Review

- `main` always builds and passes tests.
- One logical change per commit. Commit messages: a short imperative summary line (about 50 characters), then an optional body that explains *why*.
- Use short-lived feature branches and pull requests once CI exists.
- Never commit build output, IDE settings, downloaded models, or secrets (see `.gitignore`).

## 11. Definition of Done

A change is done when:

1. It is implemented as a registered command where it is an action.
2. It has unit tests for the logic and manual test notes for the UI.
3. CI passes, including `clang-tidy` and the formatting check.
4. It causes no regressions in earlier phases' exit criteria.
5. It is documented in the README or `docs/` and added to the changelog.
