# Tests

## Running

```
ctest --preset debug
ctest --preset debug --output-on-failure   # show stdout on failure
ctest --preset debug -R <pattern>          # filter by test name
```

## Fixture generation

Tests that exercise file parsers (PCX, PAK, MD2) need binary fixture files.
Rather than checking in assets with potential licensing concerns, fixtures are
generated at build time by `gen_fixtures.cpp`.

### How it works

1. CMake builds `gen_fixtures` as a standalone executable (no project
   dependencies — only the standard library).
2. An `add_custom_command` runs `gen_fixtures <build>/tests/fixtures/` whenever
   the executable changes, writing the fixture files into the build tree.
3. The `test_fixtures` custom target depends on those outputs, and `test_md2v`
   depends on `test_fixtures`, so fixtures are always up to date before tests run.
4. `fixtures.hpp.in` is processed by `configure_file` into
   `fixtures.hpp` in the build tree, providing `test_fixtures_dir()` to
   test source files.

### Adding a new fixture

1. Add a `write_<name>()` function in `gen_fixtures.cpp` that writes the file.
2. Call it from `main()`.
3. Add the output path to the `OUTPUT` list and `DEPENDS` list in the
   `add_custom_command` block in `CMakeLists.txt`.

### Current fixtures

| File | Description |
|------|-------------|
| `minimal.pcx` | 2×2 PCX image; palette index 0 = red (255,0,0), index 1 = blue (0,0,255); pixels: (0,0)=red (1,0)=blue (0,1)=blue (1,1)=red |
| `minimal.pak` | PAK archive with one entry `models/player/tris.md2` whose content is the ASCII string `HELLO` |
