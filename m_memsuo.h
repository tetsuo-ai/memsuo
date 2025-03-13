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

#ifndef M_MEMSUO_H
#define M_MEMSUO_H

/**
 * MEMSUO memory management library.
 *
 * Optimized memory allocation macros for GCC/Clang.
 * jemalloc (for performance) and libsodium (for secure memory).
 *
 * To enable features, compile with:
 *   -DUSE_JEMALLOC -DUSE_SODIUM -DENABLE_MEM_STATS
 * jemalloc and libsodium need to be installed.
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#ifdef USE_JEMALLOC
#include <jemalloc/jemalloc.h>
#ifndef je_malloc
#define je_malloc malloc
#endif
#ifndef je_calloc
#define je_calloc calloc
#endif
#ifndef je_realloc
#define je_realloc realloc
#endif
#ifndef je_free
#define je_free free
#endif
#ifndef je_posix_memalign
#define je_posix_memalign posix_memalign
#endif
#endif
#ifdef USE_SODIUM
#include <sodium.h>
#endif
#ifdef ENABLE_MEM_ATOMICS
#include <stdatomic.h>
#endif
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#include <stdalign.h>
#endif

#ifndef LOG_ERROR
#define LOG_ERROR(fmt, ...)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(stderr, "[ERROR] (%s:%d) " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);                               \
    } while (0)
#endif
#ifndef LOG_WARN
#define LOG_WARN(fmt, ...)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(stderr, "[WARN]  (%s:%d) " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);                               \
    } while (0)
#endif
#ifndef LOG_INFO
#define LOG_INFO(fmt, ...)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__);                                                           \
    } while (0)
#endif
#ifndef LOG_DEBUG
#ifdef DEBUG
#define LOG_DEBUG(fmt, ...)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(stderr, "[DEBUG] (%s:%d) " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);                               \
    } while (0)
#else
#define LOG_DEBUG(fmt, ...)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif
#endif

#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#define IS_ALIGNED(ptr, align) ((((uintptr_t)(ptr)) & ((align) - 1)) == 0)
#if defined(__GNUC__) || defined(__clang__)
#define ASSUME_ALIGNED(ptr, align) (__builtin_assume_aligned((ptr), (align)))
#else
#define ASSUME_ALIGNED(ptr, align) (ptr)
#endif

#define ATOMIC_INC(var) __atomic_add_fetch(&(var), 1, __ATOMIC_SEQ_CST)
#define ATOMIC_DEC(var) __atomic_sub_fetch(&(var), 1, __ATOMIC_SEQ_CST)
#define ATOMIC_LOAD(var) __atomic_load_n(&(var), __ATOMIC_SEQ_CST)
#define ATOMIC_STORE(var, val) __atomic_store_n(&(var), (val), __ATOMIC_SEQ_CST)

#ifdef ENABLE_MEM_STATS
#ifdef __cplusplus
extern std::atomic<size_t> g_total_alloc_bytes;
extern std::atomic<size_t> g_alloc_count;
extern std::atomic<size_t> g_free_count;
#else
extern _Atomic size_t g_total_alloc_bytes;
extern _Atomic size_t g_alloc_count;
extern _Atomic size_t g_free_count;
#endif
#define __MEMSTAT_ADD_BYTES(sz) __atomic_add_fetch(&g_total_alloc_bytes, (sz), __ATOMIC_RELAXED)
#define __MEMSTAT_INC_ALLOC() __atomic_add_fetch(&g_alloc_count, 1, __ATOMIC_RELAXED)
#define __MEMSTAT_INC_FREE() __atomic_add_fetch(&g_free_count, 1, __ATOMIC_RELAXED)
#else
#define __MEMSTAT_ADD_BYTES(sz) ((void)0)
#define __MEMSTAT_INC_ALLOC() ((void)0)
#define __MEMSTAT_INC_FREE() ((void)0)
#endif

#if defined(USE_JEMALLOC)
#define MALLOC(size)                                                                                                   \
    (__extension__({                                                                                                   \
        size_t _msz = (size);                                                                                          \
        void *_mptr = je_malloc(_msz);                                                                                 \
        if (!_mptr && _msz != 0)                                                                                       \
        {                                                                                                              \
            LOG_ERROR("%s", "je_malloc failed");                                                                       \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            __MEMSTAT_ADD_BYTES(_msz);                                                                                 \
            __MEMSTAT_INC_ALLOC();                                                                                     \
        }                                                                                                              \
        _mptr;                                                                                                         \
    }))
#define CALLOC(n, sz)                                                                                                  \
    (__extension__({                                                                                                   \
        size_t _cnt = (n), _sz = (sz);                                                                                 \
        void *_mptr = je_calloc(_cnt, _sz);                                                                            \
        if (!_mptr && (_cnt * _sz) != 0)                                                                               \
        {                                                                                                              \
            LOG_ERROR("%s", "je_calloc failed");                                                                       \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            __MEMSTAT_ADD_BYTES(_cnt *_sz);                                                                            \
            __MEMSTAT_INC_ALLOC();                                                                                     \
        }                                                                                                              \
        _mptr;                                                                                                         \
    }))
#define REALLOC(ptr, new_size)                                                                                         \
    (__extension__({                                                                                                   \
        void *_oldp = (ptr);                                                                                           \
        size_t _newsz = (new_size);                                                                                    \
        void *_mptr = je_realloc(_oldp, _newsz);                                                                       \
        if (!_mptr && _newsz != 0)                                                                                     \
        {                                                                                                              \
            LOG_ERROR("%s", "je_realloc failed");                                                                      \
        }                                                                                                              \
        else if (_mptr)                                                                                                \
        {                                                                                                              \
            __MEMSTAT_INC_ALLOC();                                                                                     \
            __MEMSTAT_INC_FREE();                                                                                      \
        }                                                                                                              \
        _mptr;                                                                                                         \
    }))
#define FREE(ptr)                                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        void *_fptr = (ptr);                                                                                           \
        if (_fptr)                                                                                                     \
        {                                                                                                              \
            __MEMSTAT_INC_FREE();                                                                                      \
            je_free(_fptr);                                                                                            \
        }                                                                                                              \
    } while (0)
#define ALIGNED_ALLOC(align, size)                                                                                     \
    (__extension__({                                                                                                   \
        void *_aptr = NULL;                                                                                            \
        size_t _asz = (size);                                                                                          \
        size_t _align = (align);                                                                                       \
        if (je_posix_memalign(&_aptr, _align, _asz) != 0)                                                              \
        {                                                                                                              \
            _aptr = NULL;                                                                                              \
        }                                                                                                              \
        if (!_aptr && _asz != 0)                                                                                       \
        {                                                                                                              \
            LOG_ERROR("%s", "je_posix_memalign failed");                                                               \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            __MEMSTAT_ADD_BYTES(_asz);                                                                                 \
            __MEMSTAT_INC_ALLOC();                                                                                     \
        }                                                                                                              \
        _aptr;                                                                                                         \
    }))
#else
#define MALLOC(size)                                                                                                   \
    (__extension__({                                                                                                   \
        size_t _msz = (size);                                                                                          \
        void *_mptr = malloc(_msz);                                                                                    \
        if (!_mptr && _msz != 0)                                                                                       \
        {                                                                                                              \
            LOG_ERROR("%s", "malloc failed");                                                                          \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            __MEMSTAT_ADD_BYTES(_msz);                                                                                 \
            __MEMSTAT_INC_ALLOC();                                                                                     \
        }                                                                                                              \
        _mptr;                                                                                                         \
    }))
#define CALLOC(n, sz)                                                                                                  \
    (__extension__({                                                                                                   \
        size_t _cnt = (n), _sz = (sz);                                                                                 \
        void *_mptr = calloc(_cnt, _sz);                                                                               \
        if (!_mptr && (_cnt * _sz) != 0)                                                                               \
        {                                                                                                              \
            LOG_ERROR("%s", "calloc failed");                                                                          \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            __MEMSTAT_ADD_BYTES(_cnt *_sz);                                                                            \
            __MEMSTAT_INC_ALLOC();                                                                                     \
        }                                                                                                              \
        _mptr;                                                                                                         \
    }))
#define REALLOC(ptr, new_size)                                                                                         \
    (__extension__({                                                                                                   \
        void *_oldp = (ptr);                                                                                           \
        size_t _newsz = (new_size);                                                                                    \
        void *_mptr = realloc(_oldp, _newsz);                                                                          \
        if (!_mptr && _newsz != 0)                                                                                     \
        {                                                                                                              \
            LOG_ERROR("%s", "realloc failed");                                                                         \
        }                                                                                                              \
        else if (_mptr)                                                                                                \
        {                                                                                                              \
            __MEMSTAT_INC_ALLOC();                                                                                     \
            __MEMSTAT_INC_FREE();                                                                                      \
        }                                                                                                              \
        _mptr;                                                                                                         \
    }))
#define FREE(ptr)                                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        void *_fptr = (ptr);                                                                                           \
        if (_fptr)                                                                                                     \
        {                                                                                                              \
            __MEMSTAT_INC_FREE();                                                                                      \
            free(_fptr);                                                                                               \
        }                                                                                                              \
    } while (0)
#define ALIGNED_ALLOC(align, size)                                                                                     \
    (__extension__({                                                                                                   \
        void *_aptr = NULL;                                                                                            \
        size_t _asz = (size);                                                                                          \
        size_t _align = (align);                                                                                       \
        if (posix_memalign(&_aptr, _align, _asz) != 0)                                                                 \
        {                                                                                                              \
            _aptr = NULL;                                                                                              \
        }                                                                                                              \
        if (!_aptr && _asz != 0)                                                                                       \
        {                                                                                                              \
            LOG_ERROR("%s", "posix_memalign failed");                                                                  \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            __MEMSTAT_ADD_BYTES(_asz);                                                                                 \
            __MEMSTAT_INC_ALLOC();                                                                                     \
        }                                                                                                              \
        _aptr;                                                                                                         \
    }))
#endif

#ifdef USE_SODIUM
#define SODIUM_MALLOC(size)                                                                                            \
    (__extension__({                                                                                                   \
        size_t _ssz = (size);                                                                                          \
        void *_sptr = sodium_malloc(_ssz);                                                                             \
        if (!_sptr && _ssz != 0)                                                                                       \
        {                                                                                                              \
            LOG_ERROR("%s", "sodium_malloc failed");                                                                   \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            __MEMSTAT_ADD_BYTES(_ssz);                                                                                 \
            __MEMSTAT_INC_ALLOC();                                                                                     \
        }                                                                                                              \
        _sptr;                                                                                                         \
    }))
#define SODIUM_FREE(ptr)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        void *_fsptr = (ptr);                                                                                          \
        if (_fsptr)                                                                                                    \
        {                                                                                                              \
            __MEMSTAT_INC_FREE();                                                                                      \
            sodium_free(_fsptr);                                                                                       \
        }                                                                                                              \
    } while (0)
#if defined(__GNUC__) || defined(__clang__)
static inline void __init_libsodium_auto(void) __attribute__((constructor));
static inline void __init_libsodium_auto(void)
{
    if (sodium_init() < 0)
    {
        LOG_ERROR("%s", "sodium_init failed");
        exit(1);
    }
}
#else
static inline void __init_libsodium_auto(void)
{
    if (sodium_init() < 0)
    {
        LOG_ERROR("%s", "sodium_init failed");
        exit(1);
    }
}
#endif
#endif

#define MALLOC_ARRAY(n, type) ((type *)MALLOC((n) * sizeof(type)))
#define CALLOC_ARRAY(n, type) ((type *)CALLOC((n), sizeof(type)))
#define REALLOC_ARRAY(ptr, n, type) ((type *)REALLOC((ptr), (n) * sizeof(type)))
#define FREE_PTR(ptr)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (ptr)                                                                                                       \
        {                                                                                                              \
            FREE(ptr);                                                                                                 \
            (ptr) = NULL;                                                                                              \
        }                                                                                                              \
    } while (0)

#endif /* M_MEMSUO_H */
