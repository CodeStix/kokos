#include "util.c"

struct MemoryChunk
{
    struct MemoryChunk *next;
    struct MemoryChunk *previous;
    int size;
};

extern struct MemoryChunk memoryChunk;

void memory_set(void *pointer, unsigned char value, int amount)
{
    if (IS_ALIGNED(amount, 8))
    {
        for (int i = 0; i < amount / sizeof(unsigned long); i++)
        {
            *((unsigned long *)pointer + i) = value;
        }
    }
    else if (IS_ALIGNED(amount, 4))
    {
        for (int i = 0; i < amount / sizeof(unsigned int); i++)
        {
            *((unsigned int *)pointer + i) = value;
        }
    }
    else if (IS_ALIGNED(amount, 2))
    {
        for (int i = 0; i < amount / sizeof(unsigned short); i++)
        {
            *((unsigned short *)pointer + i) = value;
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
