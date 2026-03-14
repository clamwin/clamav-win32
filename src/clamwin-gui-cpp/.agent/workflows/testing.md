---
description: How to run tests and add new tests for the ClamWin GUI
---

# Testing Workflow

## Strategy: MinUnit + Console Test Runner
We use a minimal C unit test approach (MinUnit macros in `minunit.h`) with a console test runner.
The test executable (`clamwin_test.exe`) is a separate build target that links the same source files
as the GUI but replaces `clamwin_main.c` with `test_main.c`.

## Running Tests

// turbo-all

1. Build the test target:
```powershell
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH; mingw32-make -C build-gui-test clamwin_test > .\build-gui-test\test_build_log.txt 2>&1
```

2. Check for build errors:
```powershell
Get-Content .\build-gui-test\test_build_log.txt
```

3. Run the tests:
```powershell
$env:PATH = "C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;" + $env:PATH; Push-Location .\build-gui-test; .\clamwin_test.exe > .\test_output.txt 2>&1; Pop-Location
```

4. Check test results:
```powershell
Get-Content .\build-gui-test\test_output.txt
```

## Adding New Tests

1. Create a new test file in `src/gui/tests/` (e.g., `test_prefs.c`)
2. Include `minunit.h` and the module header
3. Write test functions using `mu_assert`, `mu_assert_eq`, `mu_assert_str_eq`
4. Add a test suite function and register it in `test_main.c`
5. Add the source file to `CLAMWIN_TEST_SOURCES` in `cmake/clamwin.cmake`

## Test File Template
```c
#include "minunit.h"
#include "../clamwin_gui.h"

MU_TEST(test_example)
{
    mu_assert(1 == 1, "basic truth");
}

MU_TEST_SUITE(test_example_suite)
{
    MU_RUN_TEST(test_example);
}
```

## Stubs
The test target includes `stubs_clamav.c` which provides minimal implementations of the `cl_*` functions (libclamav API). This allows unit testing of the `clamwin_engine.c` wrapper logic (state management, locking, error handling) without requiring the full libclamav DLL or virus databases.

## What Can Be Tested
- **Config**: Load/save/defaults, key parsing, path resolution
- **Utils**: DB header parsing, protection status logic
- **Engine**: Wrapper state transitions, thread safety, error handling (via stubs)
- **Tray/UI**: Cannot easily test GUI drawing, but can test data structures

## TDD Cycle
1. Write a failing test for the new feature
2. Run tests — confirm failure
3. Implement the feature in the C module
4. Run tests — confirm pass
5. Refactor if needed
6. Update `docs/REFACTORING_LOG.md` with the change
