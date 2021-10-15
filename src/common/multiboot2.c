#include "../include/multiboot2.h"
#include "../include/util.h"

Multiboot2InfoTag *multiboot2_info_get(MULTIBOOT2_TYPE type)
{
    unsigned char *address = ((unsigned char *)multiboot2_info) + sizeof(Multiboot2Info);
    while (1)
    {
        Multiboot2InfoTag *tag = (Multiboot2InfoTag *)address;

        if (tag->type == MULTIBOOT2_TYPE_END)
        {
            return 0;
        }
        else if (tag->type == type)
        {
            return tag;
        }

        address += ALIGN_TO_NEXT(tag->size, 8);
    }
}

int multiboot2_info_available()
{
    // Check if the multiboot2_info is valid
    return multiboot2_info && !multiboot2_info->unused && multiboot2_info->total_size < 100000 && multiboot2_info->total_size >= 8;
}