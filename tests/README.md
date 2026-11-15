# Tests

Automated tests for the parts of Purple Polished Map that read and write files: maps (.blk), blocksets, tileset graphics, palette maps, collisions, map events, and the rules for finding a project's files.

They are a separate console program, built from the program's own source files (all of them except main.cpp) plus the files in this folder. They open no window. What the program looks like and how its window behaves is not tested here.

## Running them

On Windows, from the folder that contains build-windows.bat:

```
build-windows.bat test
```

On Linux, after building FLTK as described in [INSTALL.md](../INSTALL.md):

```
make check
```

Every test is listed with `ok` or `FAILED`, and a failed check shows the file, the line, and the actual and expected values. The exit code is 0 only if every test passed.

To run only some tests, run the test program directly with part of their names:

```
bin\Release\purple-polished-map-tests.exe map_ events_
```

`--list` prints the names of all the tests.

## How they work

- [test.h](test.h) is a very small test framework, so nothing has to be downloaded: `TEST(name) { ... }` defines a test, and `CHECK`, `CHECK_EQ` and `REQUIRE` check things inside it.
- The tests never write to [example/](../example/) or to [data/](data/). Each test that saves a file gets its own empty folder under tmp/tests-work/, which is cleared at the start of every run.
- The program keeps a few settings in global variables (monochrome, 256 tiles, priority colors). They are reset to their defaults before each test.

## Adding a test

Add a `TEST` to the file that covers the same part of the program, or add a new `test-something.cpp` file here; both build systems pick up every `tests/*.cpp` file by themselves. A new file must start with `test-`, so that its object file cannot clash with one of the program's.

A fix for a bug should come with a test that fails without the fix.
