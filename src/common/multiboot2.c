#include "kokos/multiboot2.h"
#include "kokos/util.h"

struct multiboot_tag *multiboot2_info_get(enum multiboot_tag_type type)
{
    unsigned char *address = ((unsigned char *)multiboot2_info) + sizeof(struct multiboot_info);
    while (1)
    {
        struct multiboot_tag *tag = (struct multiboot_tag *)address;

        if (tag->type == MULTIBOOT_TAG_TYPE_END)
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