#include "util.c"

// Simple memory allocation strategy:
// Every allocated block of memory starts with a header (MemoryChunk) and is linked together to the next and previous blocks of memory
// When Allocating, the linked list of memory blocks is iterated and the first gap is filled. O(n)
// When freeing, the memory block is removed from the linked list. O(1)

struct MemoryChunk
{
    struct MemoryChunk *next;
    struct MemoryChunk *previous;
    int size;
};

extern struct MemoryChunk memoryChunk;

void memory_set(void *pointer, unsigned char value, int amount)
{
    // if (IS_ALIGNED(amount, 8))
    // {
    //     unsigned long v = value | value << 8ul | value << 16ul | value << 24ul | value << 32ul | value << 40ul | value << 48ul | value << 56ul;
    //     for (int i = 0; i < amount / sizeof(unsigned long); i++)
    //     {
    //         *((unsigned long *)pointer + i) = v;
    //     }
    // }
    // else
    if (IS_ALIGNED(amount, 4))
    {
        unsigned int v = value | value << 8u | value << 16u | value << 24u;
        for (int i = 0; i < amount / sizeof(unsigned int); i++)
        {
            *((unsigned int *)pointer + i) = v;
        }
    }
    else if (IS_ALIGNED(amount, 2))
    {
        unsigned short v = value | value << 8u;
        for (int i = 0; i < amount / sizeof(unsigned short); i++)
        {
            *((unsigned short *)pointer + i) = v;
        }
    }
    else
    {
        for (int i = 0; i < amount / sizeof(unsigned char); i++)
        {
            *((unsigned char *)pointer + i) = value;
        }
    }
}

void memory_copy(void *from, void *to, int amount)
{
    if (IS_ALIGNED(amount, 8))
    {
        for (int i = 0; i < amount / sizeof(unsigned long); i++)
        {
            *((unsigned long *)to + i) = *((unsigned long *)from + i);
        }
    }
    else if (IS_ALIGNED(amount, 4))
    {
        for (int i = 0; i < amount / sizeof(unsigned int); i++)
        {
            *((unsigned int *)to + i) = *((unsigned int *)from + i);
        }
    }
    else if (IS_ALIGNED(amount, 2))
    {
        for (int i = 0; i < amount / sizeof(unsigned short); i++)
        {
            *((unsigned short *)to + i) = *((unsigned short *)from + i);
        }
    }
    else
    {
        for (int i = 0; i < amount / sizeof(unsigned char); i++)
        {
            *((unsigned char *)to + i) = *((unsigned char *)from + i);
        }
    }
}

void *memory_allocate(int bytes)
{
    bytes = ALIGN_TO_NEXT(bytes, sizeof(void *));

    struct MemoryChunk *chk = &memoryChunk;
    while (chk)
    {
        if (!chk->next || (unsigned char *)chk->next - ((unsigned char *)chk + chk->size) > bytes + sizeof(struct MemoryChunk) * 2)
        {
            // Insert memory chunk after chk
            struct MemoryChunk *newChunk = (struct MemoryChunk *)((unsigned char *)chk + sizeof(struct MemoryChunk) + chk->size);
            newChunk->size = bytes;
            newChunk->next = chk->next;
            newChunk->previous = chk;
            if (chk->next)
            {
                chk->next->previous = newChunk;
            }
            chk->next = newChunk;
            return (void *)((unsigned char *)newChunk + sizeof(struct MemoryChunk));
        }

        chk = chk->next;
    }

    return 0;
}

void memory_free(void *pointer)
{
    struct MemoryChunk *header = (struct MemoryChunk *)((unsigned char *)pointer - sizeof(struct MemoryChunk));

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

    struct MemoryChunk *header = (struct MemoryChunk *)((unsigned char *)pointer - sizeof(struct MemoryChunk));

    if (!header->next || (unsigned char *)header + header->size >= (unsigned char *)header->next)
    {
        header->size = bytes;
        return pointer;
    }
    else
    {
        void *newPointer = memory_allocate(bytes);
        memory_copy(pointer, newPointer, bytes);
        memory_free(pointer);
        return newPointer;
    }
}
