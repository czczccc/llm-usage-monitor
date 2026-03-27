# LLM Usage Monitor

`LlmUsageMonitor` is a Windows desktop utility built with Qt Widgets + C++17 for checking provider credentials, viewing official usage/cost data where available, and estimating remaining spend from a user-defined monthly budget.

## What v1 includes

- DeepSeek provider with official `/user/balance` support.
- DeepSeek daily/monthly spend trends derived from locally stored balance snapshots.
- OpenAI provider with organization cost polling and budget-based remaining spend.
- Anthropic provider with organization cost polling and budget-based remaining spend.
- Gemini (AI Studio) provider with API key validation and model discovery.
- Local persistence with `QSettings` for non-sensitive settings.
- Local SQLite database at `%LOCALAPPDATA%\\LlmUsageMonitor\\monitor.db` for balance snapshots and sync logs.
- Windows Credential Manager for API keys.

## Build prerequisites

- Windows 10/11
- Visual Studio 2022 Build Tools or Visual Studio 2022
- CMake 3.28+
- Qt 6.x with `Widgets`, `Network`, `Sql`, `Charts`, and `Test`

## Build

Set `CMAKE_PREFIX_PATH` to your Qt installation, then configure and build:

```powershell
cd D:\workspace\llm-usage-monitor
cmake --preset msvc-debug -DCMAKE_PREFIX_PATH="C:\Qt\6.8.2\msvc2022_64"
cmake --build --preset build-debug
ctest --preset test-debug
```

If your Qt toolchain is not in the default location, pass the correct path to `CMAKE_PREFIX_PATH`.

## Project layout

- `src/app`: Qt Widgets UI and window composition
- `src/core`: domain models, provider abstraction, service orchestration, parsers
- `src/providers`: provider-specific API adapters
- `src/storage`: QSettings, SQLite repository, Windows Credential Manager integration
- `tests`: parser fixtures and unit tests

## Provider notes

- DeepSeek: shows official balance; usage trend is computed from local snapshots, so you need at least two refreshes to see spend.
- OpenAI: designed for organization/admin cost endpoints; if you only provide a normal inference key, authorization may fail.
- Anthropic: designed for admin cost endpoints; normal keys may not be sufficient.
- Gemini: v1 validates the key and lists models, but does not display real usage or remaining spend.

## Verification status

The source project, tests, fixtures, and CMake setup are implemented. This machine does not currently have `cmake` or a Qt toolchain available in `PATH`, so I could not complete a local configure/build/test run here.
