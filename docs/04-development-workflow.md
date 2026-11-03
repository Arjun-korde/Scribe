# Development Workflow

> The routine to follow for every change. Run all commands from the **VS 2022 developer shell** (see 1.1), because the build and `clang-tidy` need the compiler environment.

## Overview

| # | Step | Why |
|---|---|---|
| 1 | Prepare the environment | Compiler and tools must be visible |
| 2 | Start a branch | Keep `main` always working |
| 3 | Write code and tests together | Catch bugs while the change is fresh |
| 4 | Configure (only when needed) | Regenerate build files after build-system changes |
| 5 | Build | Find compile errors and warnings |
| 6 | Run the tests | Prove the logic works and nothing else broke |
| 7 | Run the app and test by hand | Tests do not cover the UI |
| 8 | Format | Keep one consistent style |
| 9 | Lint | Catch bugs and rule violations early |
| 10 | Check the release build | Debug and release behave differently |
| 11 | Update docs and changelog | Keep documentation true |
| 12 | Review, commit, push, open a pull request | Let CI verify |
| 13 | Fix CI failures, merge, clean up | Finish the change |

---

## 1. Prepare the environment

### 1.1 Open the VS developer shell
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -SkipAutomaticLocation
cd D:\Projects\Scribe
```
**What it does:** sets environment variables (`INCLUDE`, `LIB`, `PATH`) so `cl.exe`, the linker and the Windows SDK can be found.

**Why:** Ninja does not load the compiler environment itself. Without it the configure step fails with "compiler not found". `clang-tidy` also needs it to find the standard library headers, otherwise it reports "file not found" for `<type_traits>` and similar. You can skip this step only when you build from inside Visual Studio, which does it for you.

### 1.2 Check the environment variables
`VCPKG_ROOT` and `CMAKE_PREFIX_PATH` must be set (see `docs/01-tech-stack-and-setup.md`).

**Why:** the presets use `VCPKG_ROOT` to find the vcpkg toolchain file, and CMake uses `CMAKE_PREFIX_PATH` to find Qt. Without them you get "Could not find Qt6" or a missing toolchain file.

---

## 2. Start a branch

```powershell
git switch main
git pull
git switch -c feature/short-name
```
Never work directly on `main`. Use a short, descriptive name such as `feature/open-file` or `fix/encoding-bom`.

---

## 3. Write code and tests together

- Put logic with no widget dependency in `src/core/` and test it with **GoogleTest**.
- Test Qt classes and UI behaviour with **Qt Test**.
- Follow `docs/03-coding-standards.md`, especially ownership (no raw `new`/`delete`) and naming.
- Register every editor action as a command in `CommandRegistry`.
- **Add every new `.cpp`/`.h` file to the `CMakeLists.txt` in its folder.** CMake only builds files it is told about.
- Add every new test file to `tests/CMakeLists.txt` with `add_executable` and `add_test`.

**Why write tests in the same change:** a test written later is rarely written at all, and logic such as encoding detection is exactly where silent data loss hides.

---

## 4. Configure (only when needed)

```powershell
cmake --preset debug
```
**What it does:** reads `CMakeLists.txt`, `CMakePresets.json` and `vcpkg.json`, installs vcpkg dependencies, and generates the Ninja build files in `build\debug`.

**Run it when you:**
- add or remove a source or test file in a `CMakeLists.txt`
- change `CMakeLists.txt`, `CMakePresets.json` or `vcpkg.json`
- add a dependency (see section 14)

**Why only then:** the build step reruns configure on its own when a `CMakeLists.txt` changes, but running it yourself shows the errors clearly and installs new vcpkg packages. For plain source edits you can skip it, and it saves time. Visual Studio reconfigures automatically when you save a `CMakeLists.txt`.

---

## 5. Build

```powershell
cmake --build --preset debug
```
**What it does:** compiles only the files that changed, runs Qt's `moc` for classes with `Q_OBJECT`, links the executables, and runs `windeployqt` to copy the Qt DLLs next to them.

**Why:** a clean compile is the first proof that the change is valid. Read the warnings. CI treats some of them as problems, and an ignored warning today is often tomorrow's bug.

**If something looks wrong that you cannot explain**, do a clean build:
```powershell
Remove-Item -Recurse -Force build\debug
cmake --preset debug
cmake --build --preset debug
```
Stale build files cause strange errors such as "unresolved external" after you move files, or old generated `moc` files.

---

## 6. Run the tests

```powershell
ctest --preset debug
```
**What it does:** runs every test registered with `add_test` and prints failures with their output.

**Useful variants:**
```powershell
ctest --preset debug -R Encoding        # only tests whose name matches
ctest --preset debug --rerun-failed     # only what failed last time
build\debug\tests\scribe_gtests.exe --gtest_filter=Encoding.*   # one GoogleTest suite, with full output
```
**Why:** tests protect earlier phases. Every phase has an exit criterion, such as "no data loss when saving any supported encoding", and the tests are how you know a new change did not break it.

---

## 7. Run the app and test by hand

Run `build\debug\src\app\scribe.exe`, or press **F5** in Visual Studio to run with the debugger.

**What to check:**
- the feature works through the menu, the shortcut and the command palette when it exists
- the edge cases: empty file, very long line, non-ASCII text, a read-only file
- resizing the window, high-DPI, and dark/light themes once they exist

**Why:** automated tests do not cover how the UI behaves. Write short manual test notes in the pull request, such as "opened a UTF-16 file, edited, saved, reopened: text intact".

**Debugging tips:**
- Set breakpoints in Visual Studio and run with **F5**.
- Use `qDebug()` or a logging category (`qCWarning`) for quick diagnostics, and remove stray debug output before committing.
- A crash on startup with exit code `0xc0000135` means a DLL is missing. Rebuild so `windeployqt` runs.

---

## 8. Format

```powershell
clang-format -i (git ls-files 'src/*.cpp' 'src/*.h' 'tests/*.cpp' 'tests/*.h')
```
**What it does:** rewrites the files in place to match `.clang-format` (4 spaces, 100 columns, brace style, include order).

**Why:** consistent formatting keeps diffs small and reviews about logic rather than style. CI runs `clang-format --dry-run -Werror`, so unformatted code fails the build.

**Notes:**
- `git ls-files` lists only files Git already tracks. Run `git add` on new files first, or they are skipped.
- To check without changing anything, use `clang-format --dry-run -Werror <files>`.
- Run it **before** `clang-tidy` and before each commit.

---

## 9. Lint

```powershell
# files changed compared with main
$files = git diff --name-only main -- 'src/*.cpp' 'tests/*.cpp'
clang-tidy -p build\debug $files
```
or for the whole project:
```powershell
clang-tidy -p build\debug (git ls-files 'src/*.cpp' 'tests/*.cpp')
```
**What it does:** analyses the code using the checks in `.clang-tidy`: likely bugs (`bugprone`), performance problems, outdated C++ idioms (`modernize`) and the naming rules. It uses `build\debug\compile_commands.json`, so run the configure step first.

**Why:** it finds bugs a compiler does not, such as use-after-move, wrongly-sized copies and dangling references. Findings in `bugprone`, `clang-analyzer` and `performance` are errors in CI.

**Notes:**
- By default it only reports problems and changes nothing. `--fix` applies the fixes it knows how to make, but review `git diff` afterwards, rebuild, and re-run the tests. Naming fixes rename identifiers across files.
- Lines like "70094 warnings generated" are warnings in Qt and system headers that are hidden automatically. Only findings in `src/` and `tests/` matter.
- If a finding is a false positive, suppress it on that line with `// NOLINT(check-name)` and a short comment explaining why. Do not disable checks globally without updating this document.
- It needs the VS developer shell (section 1.1) or it cannot find the standard headers.

---

## 10. Check the release build

```powershell
cmake --preset release
cmake --build --preset release
ctest --preset release
```
**What it does:** builds and tests with optimisation on and debug checks off.

**Why:** some bugs only appear in release, such as uninitialised variables, undefined behaviour exposed by the optimiser, and `Q_ASSERT` that hides a side effect. CI runs both presets, so finding the problem locally is faster than waiting for CI. Do this at least before opening a pull request, and always after changes to threading, memory handling or encoding code.

---

## 11. Update documentation and the changelog

- Update the README or the files in `docs/` when behaviour, build steps or interfaces change.
- Add a line to `CHANGELOG.md` for user-visible changes (the file is created with the first feature).
- Tick the finished items in `docs/02-development-phases.md`.
- Update `docs/03-coding-standards.md`, `.clang-format` and `.clang-tidy` **together** when a rule changes.

**Why:** the Definition of Done requires it, and out-of-date documentation is worse than none.

---

## 12. Review, commit, push, open a pull request

```powershell
git status          # no build/ or .vs/ files, nothing unexpected
git diff            # read your own change once
git add <files>
git commit -m "Add Open File command"
git push -u origin feature/short-name
```
Open the pull request on GitHub. Put the manual test notes in the description. Commit messages: short imperative summary line, then an optional body explaining *why*. One logical change per commit.

---

## 13. CI failures, merge, clean-up

CI (`.github/workflows/ci.yml`) runs on every push and pull request:

| CI step | Fix when it fails |
|---|---|
| Configure or Build | Read the first error. Reproduce locally with the same preset |
| Test | Run `ctest --preset <preset> --output-on-failure` locally |
| Check formatting | Run the format command in section 8, then commit |
| clang-tidy | Run section 9 locally and fix the findings |
| Only `release` fails | Build the release preset locally (section 10) |

Merge only when every check is green.

**Note:** CI's LLVM version can differ from your local one. If formatting or lint results differ between your machine and CI, compare `clang-format --version` and tell the team, so the version can be pinned.

```powershell
git switch main
git pull
git branch -d feature/short-name
```

---

## 14. Adding a dependency

1. Search for the port: `C:\vcpkg\vcpkg.exe search <name>`.
2. Add it to `dependencies` in `vcpkg.json`.
3. Run `cmake --preset debug` so vcpkg installs it.
4. Add `find_package(...)` and `target_link_libraries(...)` in the right `CMakeLists.txt`.
5. Update `docs/01-tech-stack-and-setup.md` if it is a new tool or library.

**Why:** the `builtin-baseline` in `vcpkg.json` pins versions, so everyone, and CI, gets the same package versions. To upgrade packages, update the baseline to a newer vcpkg commit (`git -C C:\vcpkg pull`, then `git -C C:\vcpkg rev-parse HEAD`), rebuild, and run all tests.

---

## 15. Finishing a phase (release)

1. Check the phase's exit criteria in `docs/02-development-phases.md` by hand.
2. Run the full build, tests and lint on both presets.
3. Update `CHANGELOG.md` and the version in the root `CMakeLists.txt` (`project(Scribe VERSION ...)`) and `vcpkg.json`.
4. Merge to `main`, wait for green CI, then tag:
   ```powershell
   git tag v0.1.0
   git push --tags
   ```

---

## Quick reference

```powershell
# Every time
cmake --build --preset debug
ctest --preset debug
clang-format -i (git ls-files 'src/*.cpp' 'src/*.h' 'tests/*.cpp' 'tests/*.h')
clang-tidy -p build\debug (git ls-files 'src/*.cpp' 'tests/*.cpp')

# Before opening a pull request
cmake --preset release; cmake --build --preset release; ctest --preset release
```

## Planned additions

These are in the tech-stack plan but not set up yet:
- AddressSanitizer build and CI job, for memory bugs.
- `cppcheck` step in CI.
- A pre-commit hook or `scripts\format.ps1`, to run the format step automatically.
- `CHANGELOG.md`, `README.md` and a license file.
