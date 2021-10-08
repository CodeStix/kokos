#pragma once

typedef struct MemoryChunk
{
    struct MemoryChunk *next;
    struct MemoryChunk *previous;
    int size;
} MemoryChunk;

extern MemoryChunk memory_chunk;

// Sets a region of memory starting at `pointer` and ending at `pointer + amount` equal to `value`
void memory_set(void *pointer, unsigned char value, int amount);

// Copies `amount` of bytes from `from` to `to`
void memory_copy(void *from, void *to, int amount);

// Allocates `bytes` bytes and returns the address.
void *memory_allocate(int bytes);

// Tries to resize previously allocated pointer to `bytes` bytes
void *memory_resize(void *pointer, int bytes);

// Frees the memory previously allocated at `pointer`
void memory_free(void *pointer);