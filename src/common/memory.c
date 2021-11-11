#include "kokos/util.h"
#include "kokos/memory.h"

// Simple memory allocation strategy:
// Every allocated block of memory starts with a header (MemoryChunk) and is linked together to the next and previous blocks of memory
// When Allocating, the linked list of memory blocks is iterated and the first gap is filled. O(n)
// When freeing, the memory block is removed from the linked list. O(1)

void memory_zero(void *address, unsigned long size)
{
    // TODO optimize
    for (unsigned long i = 0; i < size; i++)
    {
        *((unsigned char *)address + i) = 0;
    }
}

void memory_set(void *address, unsigned long size, unsigned char value)
{
    // TODO optimize
    for (unsigned long i = 0; i < size; i++)
    {
        *((unsigned char *)address + i) = value;
    }
}

int memory_compare(void *a, void *b, unsigned long size)
{
    for (unsigned long i = 0; i < size; i++)
    {
        if (*((unsigned char *)a) != *((unsigned char *)b))
        {
            return 0;
        }

        a = ((unsigned char *)a) + 1;
        b = ((unsigned char *)b) + 1;
    }
    return 1;
}

void memory_copy(void *from, void *to, int size)
{
    // TODO optimize
    for (int i = 0; i < size; i++)
    {
        *((unsigned char *)to + i) = *((unsigned char *)from + i);
    }
}

void *memory_allocate(int bytes)
{
    bytes = ALIGN_TO_NEXT(bytes, sizeof(void *));

    MemoryChunk *chk = &memory_chunk;
    while (chk)
    {
        if (!chk->next || (unsigned char *)chk->next - ((unsigned char *)chk + chk->size) > bytes + sizeof(MemoryChunk) * 2)
        {
            // Insert memory chunk after chk
            MemoryChunk *new_chunk = (MemoryChunk *)((unsigned char *)chk + sizeof(MemoryChunk) + chk->size);
            new_chunk->size = bytes;
            new_chunk->next = chk->next;
            new_chunk->previous = chk;
            if (chk->next)
            {
                chk->next->previous = new_chunk;
            }
            chk->next = new_chunk;
            return (void *)((unsigned char *)new_chunk + sizeof(MemoryChunk));
        }

        chk = chk->next;
    }

    return 0;
}

void memory_free(void *pointer)
{
    MemoryChunk *header = (MemoryChunk *)((unsigned char *)pointer - sizeof(MemoryChunk));

    if (header->previous)
    {
        header->previous->next = header->next;
    }
    if (header->next)
    {
        header->next->previous = header->previous;
    }
}

void *memory_resize(void *pointer, int bytes)
{
    bytes = ALIGN_TO_NEXT(bytes, sizeof(void *));

    MemoryChunk *header = (MemoryChunk *)((unsigned char *)pointer - sizeof(MemoryChunk));

    if (!header->next || (unsigned char *)header + header->size >= (unsigned char *)header->next)
    {
        header->size = bytes;
        return pointer;
    }
    else
    {
        void *new_pointer = memory_allocate(bytes);
        memory_copy(pointer, new_pointer, bytes);
        memory_free(pointer);
        return new_pointer;
    }
}
