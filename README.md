# Scribe

A Notepad-style text editor for Windows, built with modern C++20 and Qt 6 Widgets.

> Working name. Status: early development (Phase 0 complete, Phase 1 in progress).

## Planned Features

- Notepad parity: tabs, find/replace, encodings, line endings, zoom, word wrap
- Windows polish: dark/light themes, session restore, crash recovery
- Power editing, macros, snippets, plugins
- Offline speech-to-text

See [docs/02-development-phases.md](docs/02-development-phases.md) for the roadmap.

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 Build Tools (MSVC)
- Qt 6.5 LTS or newer
- CMake 3.25+, Ninja, Git, vcpkg

Full setup details: [docs/01-tech-stack-and-setup.md](docs/01-tech-stack-and-setup.md).

## Build and Test

```
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

See [docs/04-development-workflow.md](docs/04-development-workflow.md) for the day-to-day workflow.

## Documentation

- [Tech stack and setup](docs/01-tech-stack-and-setup.md)
- [Development phases](docs/02-development-phases.md)
- [Coding standards](docs/03-coding-standards.md)
- [Development workflow](docs/04-development-workflow.md)
- [UI design](docs/04-ui-design.md)

## License

To be decided.
