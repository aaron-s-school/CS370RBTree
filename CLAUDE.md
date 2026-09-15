# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A CS370 lab: implement a red-black tree in C providing a `string key -> void* value` map ADT. The public API contract lives entirely in `include/rbtree.h`; `src/rbtree.c` is the implementation to write. `tests/test_rbtree.c` and `tests/fuzz.c` are currently empty stubs the Makefile already wires up to build.

## Build & Test Commands

The Makefile assumes a Unix toolchain (`gcc`, `mkdir -p`, `rm -rf`, `valgrind`) — run these under WSL/Linux, not native PowerShell.

- `make` or `make all` — build `build/test_rbtree` and `build/fuzz`
- `make test` — build, then run the test binary followed by the fuzzer with 100000 iterations
- `make asan` — clean rebuild with `-fsanitize=address,undefined` and run tests
- `make memcheck` — build, then run both binaries under `valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1` (test binary, then fuzz binary with 20000 iterations)
- `make clean` — remove `build/`

To run a single test, invoke the built binary directly with whatever selection mechanism `test_rbtree.c` implements once written (e.g. `./build/test_rbtree <name>`), since there is no test framework wired in yet beyond the Makefile targets.

- A change is DONE only when make test, make asan, and make memcheck all pass. Always run them; show output.

## Style
- C23. -Wall -Wextra -Werror must stay clean. No VLAs.
- Error handling: goto-cleanup pattern for multi-allocation functions.
- Prefer the smallest diff that passes. Do not refactor unrelated code.
- Every non-obvious loop gets a one-line invariant comment.

## Architecture

- `include/rbtree.h` is the single source of truth for the ADT's contract. Key ownership/memory rules encoded there that the implementation and tests must honor:
  - `rb_create` takes an optional `rb_value_free_fn`; if non-NULL the tree owns values and frees them via this callback on overwrite/delete/destroy. If NULL, values are borrowed and never freed by the tree.
  - `rb_insert` copies the key (tree owns the copy) and takes ownership of `value` only on success (return 0). On failure (-1, allocation failure) the tree is unchanged and the caller still owns `value`.
  - `rb_delete` frees both the key copy and the value (via `value_free` if set).
  - `rb_destroy` is NULL-safe.
  - `rb_validate` returns 0 iff all red-black invariants hold — this is the primary correctness oracle for tests/fuzzing, not a separate hand-rolled checker.
- `tests/fuzz.c` takes an iteration count as its argument (see `make test`/`make memcheck` invocations) and is expected to exercise the tree against `rb_validate` and/or a reference model over many random operations.
- There is one translation unit (`src/rbtree.c`) implementing the whole ADT — no internal module boundaries to preserve beyond what the header exposes.

## Hard constraints
- NEVER modify include/rbtree.h. It is the graded contract.
- Check every allocation. malloc can return NULL; a NULL return must
leave the tree unchanged and return the documented error code.
- NEVER weaken, skip, or delete a test to make the suite pass. If a test
looks wrong, stop and explain why instead.


## Workflow
- For any multi-file or algorithmic change: propose a plan and wait for
approval before editing.
- Commit only from a green state; message format "M<n>: <what>".