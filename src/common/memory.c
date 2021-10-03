struct MemoryChunk
{
    struct MemoryChunk *next;
    struct MemoryChunk *previous;
    int size;
};

extern struct MemoryChunk memoryChunk;

void memory_set(void *pointer, char value, int amount)
{
    for (int i = 0; i < amount; i++)
    {
        *((char *)pointer + i) = value;
    }
}

void *memory_allocate(int bytes)
{
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

// void *memory_resize(void *pointer, int bytes)
// {
// }
