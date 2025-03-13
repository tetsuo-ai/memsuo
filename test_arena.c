#include <stdio.h>
#include <string.h>
#include "a_memsuo.h"

int main(void)
{
    /* Create a normal arena with an initial block size of 1024 bytes.
       The ARENA_SCOPE macro declares an arena variable that will be automatically
       destroyed at scope exit (using the cleanup attribute). */
    ARENA_SCOPE(arena, 1024);

    /* Allocate an array of 10 integers from the arena. */
    int *numbers = ARENA_ALLOC(&arena, int, 10);
    if (!numbers)
    {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    for (int i = 0; i < 10; i++)
    {
        numbers[i] = i * 3;
    }
    printf("Normal Arena Allocation:\n");
    for (int i = 0; i < 10; i++)
    {
        printf("numbers[%d] = %d\n", i, numbers[i]);
    }

    /* Allocate a string from the arena and print it. */
    char *message = ARENA_ALLOC(&arena, char, 50);
    strcpy(message, "Hello from the normal arena!");
    printf("Message: %s\n", message);

#if defined(USE_SODIUM) || defined(USE_LIBSODIUM)
    /* Create a secure arena that uses libsodium’s guarded memory functions.
       Memory allocated from this arena will be zeroed on free and kept locked. */
    ARENA_SCOPE_SECURE(sec_arena, 1024);
    char *secret = ARENA_ALLOC(&sec_arena, char, 50);
    strcpy(secret, "Sensitive Data");
    printf("Secure Arena Allocation: %s\n", secret);
#endif

    /* When main returns, the cleanup attribute automatically calls arena_destroy
       on both arena and sec_arena (if defined), releasing all memory in one go. */
    return 0;
}
