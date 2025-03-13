/**
 * Copyright (c) 2025, 7etsuo  https://tetsuo.ai/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef A_MEMSUO_H
#define A_MEMSUO_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#ifdef USE_LIBSODIUM
#include <sodium.h>
#endif

typedef struct ArenaBlock
{
    struct ArenaBlock *next;
    size_t capacity;
    size_t used;
    unsigned char *base;
} ArenaBlock;

typedef struct Arena
{
    ArenaBlock *blocks;
    int secure;
} Arena;

#define ARENA_NO_ZERO 1

static void arena_destroy(Arena *arena);
static void *arena_alloc(Arena *arena, size_t size, size_t align, size_t count, int flags);
static int arena_grow(Arena *arena, size_t min_size);

#define ARENA_INIT(arenaPtr, initial_size, secure_flag) (arena_init((arenaPtr), (initial_size), (secure_flag)))

#if defined(__GNUC__) || defined(__clang__)
#define ARENA_SCOPE(name, initial_size)                                                                                \
    __attribute__((cleanup(arena_destroy))) Arena name;                                                                \
    arena_init(&(name), (initial_size), 0)
#define ARENA_SCOPE_SECURE(name, initial_size)                                                                         \
    __attribute__((cleanup(arena_destroy))) Arena name;                                                                \
    arena_init(&(name), (initial_size), 1)
#else
#define ARENA_SCOPE(name, initial_size)                                                                                \
    Arena name;                                                                                                        \
    arena_init(&(name), (initial_size), 0)
#define ARENA_SCOPE_SECURE(name, initial_size)                                                                         \
    Arena name;                                                                                                        \
    arena_init(&(name), (initial_size), 1)
#endif

#define ARENA_ALLOC(arenaPtr, Type, count) ((Type *)arena_alloc((arenaPtr), sizeof(Type), _Alignof(Type), (count), 0))
#define ARENA_ALLOC_NOZERO(arenaPtr, Type, count)                                                                      \
    ((Type *)arena_alloc((arenaPtr), sizeof(Type), _Alignof(Type), (count), ARENA_NO_ZERO))

static int arena_init(Arena *arena, size_t initial_size, int secure_flag)
{
    arena->secure = secure_flag;
    arena->blocks = NULL;
    if (initial_size == 0)
        return 0;
#ifdef USE_LIBSODIUM
    if (arena->secure)
    {
        if (sodium_init() < 0)
            return -1;
        unsigned char *ptr = (unsigned char *)sodium_malloc(initial_size);
        if (!ptr)
            return -1;
        ArenaBlock *block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
        if (!block)
        {
            sodium_free(ptr);
            return -1;
        }
        block->next = NULL;
        block->capacity = initial_size;
        block->used = 0;
        block->base = ptr;
        arena->blocks = block;
    }
    else
#endif
    {
        unsigned char *ptr = (unsigned char *)malloc(initial_size);
        if (!ptr)
            return -1;
        ArenaBlock *block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
        if (!block)
        {
            free(ptr);
            return -1;
        }
        block->next = NULL;
        block->capacity = initial_size;
        block->used = 0;
        block->base = ptr;
        arena->blocks = block;
    }
    return 0;
}

static void *arena_alloc(Arena *arena, size_t size, size_t align, size_t count, int flags)
{
    if (count == 0 || size == 0)
        return NULL;
    size_t total = size * count;
    if (count > SIZE_MAX / size)
        return NULL;
    ArenaBlock *block = arena->blocks;
    if (!block)
    {
        if (arena_grow(arena, total) != 0)
            return NULL;
        block = arena->blocks;
    }
    else
    {
        while (block->next)
            block = block->next;
    }
    uintptr_t base_addr = (uintptr_t)block->base;
    uintptr_t curr_ptr = base_addr + block->used;
    size_t padding = 0;
    if ((curr_ptr & (align - 1)) != 0)
        padding = (align - (curr_ptr & (align - 1))) & (align - 1);
    if (block->used + padding + total > block->capacity)
    {
        if (arena_grow(arena, total) != 0)
            return NULL;
        block = arena->blocks;
        while (block->next)
            block = block->next;
        base_addr = (uintptr_t)block->base;
        curr_ptr = base_addr + block->used;
        padding = 0;
        if ((curr_ptr & (align - 1)) != 0)
            padding = (align - (curr_ptr & (align - 1))) & (align - 1);
    }
    curr_ptr += padding;
    void *out_ptr = (void *)curr_ptr;
    block->used += padding + total;
    if (!(flags & ARENA_NO_ZERO))
        memset(out_ptr, 0, total);
    return out_ptr;
}

static int arena_grow(Arena *arena, size_t min_size)
{
    size_t new_cap;
    if (arena->blocks)
    {
        ArenaBlock *last = arena->blocks;
        while (last->next)
            last = last->next;
        new_cap = last->capacity * 2;
        if (new_cap < min_size)
            new_cap = min_size;
    }
    else
    {
        new_cap = min_size;
    }
#ifdef USE_LIBSODIUM
    if (arena->secure)
    {
        unsigned char *ptr = (unsigned char *)sodium_malloc(new_cap);
        if (!ptr)
            return -1;
        ArenaBlock *block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
        if (!block)
        {
            sodium_free(ptr);
            return -1;
        }
        block->next = NULL;
        block->capacity = new_cap;
        block->used = 0;
        block->base = ptr;
        if (!arena->blocks)
            arena->blocks = block;
        else
        {
            ArenaBlock *last = arena->blocks;
            while (last->next)
                last = last->next;
            last->next = block;
        }
    }
    else
#endif
    {
        unsigned char *ptr = (unsigned char *)malloc(new_cap);
        if (!ptr)
            return -1;
        ArenaBlock *block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
        if (!block)
        {
            free(ptr);
            return -1;
        }
        block->next = NULL;
        block->capacity = new_cap;
        block->used = 0;
        block->base = ptr;
        if (!arena->blocks)
            arena->blocks = block;
        else
        {
            ArenaBlock *last = arena->blocks;
            while (last->next)
                last = last->next;
            last->next = block;
        }
    }
    return 0;
}

static void arena_destroy(Arena *arena)
{
    ArenaBlock *block = arena->blocks;
    while (block)
    {
        ArenaBlock *next = block->next;
#ifdef USE_LIBSODIUM
        if (arena->secure)
            sodium_free(block->base);
        else
            free(block->base);
#else
        free(block->base);
#endif
        free(block);
        block = next;
    }
    arena->blocks = NULL;
}

#endif // A_MEMSUO_H
