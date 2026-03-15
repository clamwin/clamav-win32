---
description: How to run tests and add new tests for the ClamWin GUI
---

# Testing Workflow

## Strategy: doctest + C++ Test Runner
The GUI test suite uses doctest in `src/clamwin-gui-cpp/tests/`.
The primary executable is `clamwin_gui_test.exe`, with CMake targets that run
normal checks and optional real-tools smoke tests.

## Running Tests

1. Configure and build test target:
```powershell
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH
cmake -S . -B build-gui -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_MAKE_PROGRAM=mingw32-make
cmake --build build-gui --target clamwin_gui_test
```

2. Run standard GUI test checks:
```powershell
cmake --build build-gui --target clamwin_gui_check
```

3. Optional: run real-tools smoke checks (requires binaries/db and may be environment-dependent):
```powershell
cmake --build build-gui --target clamwin_gui_check_real_tools
```

## Adding New Tests

1. Add a new test file in `src/clamwin-gui-cpp/tests/` (e.g., `test_feature.cpp`).
2. Include `doctest.h` and the header(s) under test.
3. Add `TEST_CASE(...)` blocks with deterministic inputs.
4. Ensure the file is listed in the GUI test target sources in CMake.

## Test File Template
```cpp
#include "doctest.h"

TEST_CASE("example")
{
    CHECK(1 == 1);
}
```

## What Can Be Tested
- **Config/Parsing/Validation**: deterministic unit tests.
- **Scheduler/CLI/Command-building**: behavior-focused tests.
- **Real tools**: smoke tests behind opt-in targets.

## TDD Cycle
1. Write a failing test for the new feature
2. Run tests — confirm failure
3. Implement the feature in production code
4. Run tests — confirm pass
5. Refactor if needed
