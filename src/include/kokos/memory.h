#pragma once

struct memory_chunk
{
    struct memory_chunk *next;
    struct memory_chunk *previous;
    int size;
};

extern struct memory_chunk memory_chunk;

// Sets a region of memory starting at `pointer` and ending at `pointer + amount` equal to `value`
void memory_set(void *pointer, unsigned long size, unsigned char amount);

// Sets a region of memory to zero
void memory_zero(void *address, unsigned long size);

// Copies `amount` of bytes from `from` to `to`
void memory_copy(void *from, void *to, int amount);

// Allocates `bytes` bytes and returns the address.
void *memory_allocate(int bytes);

// Tries to resize previously allocated pointer to `bytes` bytes
void *memory_resize(void *pointer, int bytes);

// Frees the memory previously allocated at `pointer`
void memory_free(void *pointer);