#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "m_memsuo.h"

#ifdef ENABLE_MEM_STATS
_Atomic size_t g_total_alloc_bytes = 0;
_Atomic size_t g_alloc_count = 0;
_Atomic size_t g_free_count = 0;
#endif

#define THREAD_COUNT 4
#define THREAD_ITERATIONS 1000

void *thread_alloc(void *arg);

int main(void)
{
    printf("Starting full test coverage for memory management library.\n");

    char *msg = (char *)MALLOC(128);
    if (!msg)
    {
        LOG_ERROR("%s", "MALLOC returned NULL");
        return 1;
    }
    strcpy(msg, "Hello Memory Management!");
    printf("MALLOC: %s\n", msg);

    int *array = (int *)CALLOC(10, sizeof(int));
    if (!array)
    {
        LOG_ERROR("%s", "CALLOC returned NULL");
        FREE(msg);
        return 1;
    }
    for (int i = 0; i < 10; i++)
    {
        if (array[i] != 0)
            LOG_ERROR("%s", "CALLOC did not zero initialize memory");
        array[i] = i * i;
    }
    printf("CALLOC array: ");
    for (int i = 0; i < 10; i++)
        printf("%d ", array[i]);
    printf("\n");

    array = (int *)REALLOC(array, 20 * sizeof(int));
    if (!array)
    {
        LOG_ERROR("%s", "REALLOC returned NULL");
        FREE(msg);
        return 1;
    }
    for (int i = 10; i < 20; i++)
        array[i] = i;
    printf("REALLOC array: ");
    for (int i = 0; i < 20; i++)
        printf("%d ", array[i]);
    printf("\n");

    void *aligned_ptr = ALIGNED_ALLOC(64, 256);
    if (!aligned_ptr)
    {
        LOG_ERROR("%s", "ALIGNED_ALLOC returned NULL");
    }
    else
    {
        printf("ALIGNED_ALLOC pointer: %p\n", aligned_ptr);
        if (!IS_ALIGNED(aligned_ptr, 64))
            LOG_ERROR("%s", "Aligned pointer is not aligned to 64 bytes");
    }

    char *ptr_for_free = (char *)MALLOC(64);
    if (!ptr_for_free)
    {
        LOG_ERROR("%s", "MALLOC for FREE_PTR test returned NULL");
    }
    else
    {
        strcpy(ptr_for_free, "Testing FREE_PTR macro");
        printf("FREE_PTR before: %s\n", ptr_for_free);
        FREE_PTR(ptr_for_free);
        if (ptr_for_free != NULL)
            LOG_ERROR("%s", "FREE_PTR did not set pointer to NULL");
        else
            printf("FREE_PTR succeeded, pointer is NULL.\n");
    }

    int *int_array = MALLOC_ARRAY(5, int);
    if (!int_array)
    {
        LOG_ERROR("%s", "MALLOC_ARRAY returned NULL");
    }
    else
    {
        for (int i = 0; i < 5; i++)
            int_array[i] = i + 1;
        printf("MALLOC_ARRAY: ");
        for (int i = 0; i < 5; i++)
            printf("%d ", int_array[i]);
        printf("\n");

        int_array = REALLOC_ARRAY(int_array, 10, int);
        if (!int_array)
        {
            LOG_ERROR("%s", "REALLOC_ARRAY returned NULL");
        }
        else
        {
            for (int i = 5; i < 10; i++)
                int_array[i] = (i + 1) * 10;
            printf("REALLOC_ARRAY: ");
            for (int i = 0; i < 10; i++)
                printf("%d ", int_array[i]);
            printf("\n");
            FREE(int_array);
        }
    }

#ifdef USE_SODIUM
    char *secure_msg = (char *)SODIUM_MALLOC(64);
    if (!secure_msg)
    {
        LOG_ERROR("%s", "SODIUM_MALLOC returned NULL");
    }
    else
    {
        strcpy(secure_msg, "Secure memory test.");
        printf("SODIUM_MALLOC: %s\n", secure_msg);
        SODIUM_FREE(secure_msg);
    }
#endif

    FREE(NULL);

    pthread_t threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (pthread_create(&threads[i], NULL, thread_alloc, NULL) != 0)
        {
            LOG_ERROR("%s", "Failed to create thread");
            return 1;
        }
    }
    for (int i = 0; i < THREAD_COUNT; i++)
        pthread_join(threads[i], NULL);
    printf("Multithreaded allocation test completed.\n");

    FREE(msg);
    FREE(array);
    FREE(aligned_ptr);

#ifdef ENABLE_MEM_STATS
    printf("Memory stats:\n");
    printf("  Total allocated bytes: %zu\n", ATOMIC_LOAD(g_total_alloc_bytes));
    printf("  Allocation count: %zu\n", ATOMIC_LOAD(g_alloc_count));
    printf("  Free count: %zu\n", ATOMIC_LOAD(g_free_count));
#endif

    printf("All tests completed successfully.\n");
    return 0;
}

void *thread_alloc(void *arg)
{
    (void)arg;
    for (int i = 0; i < THREAD_ITERATIONS; i++)
    {
        char *ptr0 = (char *)MALLOC(0);
        if (ptr0)
            FREE(ptr0);

        char *ptr = (char *)MALLOC(32);
        if (!ptr)
        {
            LOG_ERROR("%s", "Thread MALLOC returned NULL");
            continue;
        }
        strcpy(ptr, "Thread allocation test");
        FREE(ptr);
    }
    return NULL;
}
