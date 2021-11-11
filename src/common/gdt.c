#include "kokos/gdt.h"
#include "kokos/console.h"

extern GdtEntry gdt64[];

void gdt_debug()
{
    for (int i = 0; i < 3; i++)
    {
        GdtEntry entry = gdt64[i];
        if (!entry.present)
            continue;
        console_print("[gdt] entry #");
        console_print_u64(i, 10);
        console_print(" base=0x");
        console_print_u64(entry.base1 | (entry.base2 << 16) | (entry.base3 << 24), 16);
        console_print(" limit=0x");
        console_print_u64(entry.limit1 | (entry.limit2 << 16), 16);
        console_print(" access=");
        if (entry.present)
            console_print("P");
        console_print_u32((unsigned int)entry.privilege, 10);
        if (entry.type)
            console_print("T");
        if (entry.executable)
            console_print("E");
        if (entry.direction_conforming)
            console_print("D");
        if (entry.read_write)
            console_print("W");
        if (entry.accessed)
            console_print("a");
        console_print(" flags=");
        if (entry.granularity)
            console_print("G");
        if (entry.size)
            console_print("S");
        if (entry.long_mode)
            console_print("L");
        console_new_line();
    }
}