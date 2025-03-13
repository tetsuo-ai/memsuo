# MEMSUO Memory Management Library

---

**$TETSUO on Solana**: `8i51XNNpGaKaj4G4nDdmQh95v4FKAxw8mhtaRoKd9tE8`

[![Twitter](https://img.shields.io/badge/Twitter-Follow%20%407etsuo-1DA1F2)](https://x.com/7etsuo)
[![Discord](https://img.shields.io/badge/Discord-Join%20Our%20Community-7289DA)](https://discord.gg/tetsuo-ai)

---

This repository contains two distinct memory management modules:

1. **Standard Memory Management**  
   Provides optimized memory allocation macros using jemalloc for high-performance allocations and libsodium for secure memory. It includes functions to allocate, reallocate, and free memory as well as maintain statistics if enabled.

2. **Arena Memory Management**  
   Implements an arena-based allocator that performs fast bump-pointer allocations from large memory blocks. It supports both normal and secure memory modes. The arena automatically frees all allocated memory when it goes out of scope.

---

## Features

### Standard Memory Management
- **Optimized Allocation Macros:**  
  Implements `MALLOC`, `CALLOC`, `REALLOC`, and `FREE` macros to wrap memory allocation functions.
- **Performance Focus:**  
  Uses jemalloc to minimize fragmentation and improve allocation throughput.
- **Secure Memory Option:**  
  Uses libsodium to allocate secure memory that is locked and zeroed on free.
- **Memory Statistics:**  
  Provides macros to track total allocated bytes, allocation counts, and free counts when memory statistics are enabled.

### Arena Memory Management
- **Fast Bump-Pointer Allocator:**  
  Groups small allocations into large blocks, reducing allocation overhead.
- **Automatic Cleanup:**  
  Uses GCC/Clang cleanup attributes so that the arena is automatically destroyed at scope exit.
- **Secure Arena Support:**  
  Offers a secure mode that leverages libsodium for guarded memory allocation.
- **Precise Allocation Macros:**  
  Provides `ARENA_SCOPE` and `ARENA_SCOPE_SECURE` for automatic arena declaration, and `ARENA_ALLOC` and `ARENA_ALLOC_NOZERO` for allocating memory with proper alignment.

---

## Requirements

- A C compiler with C11 support.
- [jemalloc](http://jemalloc.net/) installed for high-performance memory operations.
- [libsodium](https://libsodium.gitbook.io/doc/) installed for secure memory management.
- POSIX environment for thread support and atomic operations.

---

## Building

A Makefile is provided in the repository.

### To Build Both Test Programs
Run:
```bash
make
```
This command will build the standard test program (`test_memory`) and the arena test program (`test_arena`).

### To Build the Arena Test Program Separately
Run:
```bash
make arena
```

### Cleaning Up
Run:
```bash
make clean
```
This will remove the generated executables.

---

## Usage

### Standard Memory Management

Include the header file `m_memsuo.h` in your project. Use the provided macros:
- **`MALLOC(size)`** – Allocates a block of memory.
- **`CALLOC(n, size)`** – Allocates and zero-initializes an array.
- **`REALLOC(ptr, new_size)`** – Resizes an allocated memory block.
- **`FREE(ptr)`** – Frees a memory block.
- **`MALLOC_ARRAY(n, type)`** – Allocates an array of a specified type.
- **`CALLOC_ARRAY(n, type)`** – Allocates and zero-initializes an array of a specified type.
- **`REALLOC_ARRAY(ptr, n, type)`** – Resizes an array of a specified type.
- **`FREE_PTR(ptr)`** – Frees a pointer and sets it to `NULL`.

### Arena Memory Management

Include the header file `a_memsuo.h` in your project. The following macros are available:
- **`ARENA_SCOPE(name, initial_size)`**  
  Declares a normal arena that automatically cleans up at the end of its scope.
- **`ARENA_SCOPE_SECURE(name, initial_size)`**  
  Declares a secure arena that uses libsodium’s guarded memory functions.
- **`ARENA_ALLOC(arena, Type, count)`**  
  Allocates an array of objects of the specified type from the arena.
- **`ARENA_ALLOC_NOZERO(arena, Type, count)`**  
  Allocates memory from the arena without zero-initializing it (for performance-sensitive allocations).

See the provided test files (`test_memory.c` and `test_arena.c`) for concrete usage examples.

---

## Running Valgrind

To check for memory leaks when running the arena test program, use the following command:
```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./test_arena
```

## License

This library is released under the BSD 2-Clause License. See the source files for complete license details.
