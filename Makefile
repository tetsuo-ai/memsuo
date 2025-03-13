CC      = gcc
CFLAGS  = -std=c11 -O3 -Wall -Wextra -pedantic -D_GNU_SOURCE
DEFINES = -DUSE_JEMALLOC -DUSE_SODIUM -DENABLE_MEM_STATS
LIBS    = -ljemalloc -lsodium -pthread

TARGET       = test_memory
TARGET_SRCS  = test_memory.c
ARENA_TARGET = test_arena
ARENA_SRCS   = test_arena.c

all: $(TARGET) $(ARENA_TARGET)

$(TARGET): $(TARGET_SRCS) m_memsuo.h
	$(CC) $(CFLAGS) $(DEFINES) -o $(TARGET) $(TARGET_SRCS) $(LIBS)

$(ARENA_TARGET): $(ARENA_SRCS) a_memsuo.h
	$(CC) $(CFLAGS) $(DEFINES) -o $(ARENA_TARGET) $(ARENA_SRCS) $(LIBS)

clean:
	rm -f $(TARGET) $(ARENA_TARGET)
