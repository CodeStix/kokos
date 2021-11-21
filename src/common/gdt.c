#include "kokos/cpu.h"
#include "kokos/gdt.h"
#include "kokos/memory_physical.h"
#include "kokos/console.h"
#include "kokos/memory.h"

void gdt_debug()
{
    struct cpu *cpu = cpu_get_current();

    for (int i = 0; i < 6; i++)
    {
        struct gdt_entry entry = cpu->global_descriptor_table[i];
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
        if (!entry.non_system)
        {
            console_print("S");
            i++;
        }
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
            console_print("Z");
        if (entry.long_mode)
            console_print("L");
        console_new_line();
    }
}

void gdt_initialize()
{
    // Allocate space for the global descriptor table
    struct gdt_entry *global_descriptor_table = memory_physical_allocate();
    memory_zero(global_descriptor_table, 4096);

    // Skip entry 0, it is considered a null entry

    // Create kernel code segment (at 0x8)
    // Format of this segment: https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDE1NCxbMTU0LDM1LDE1NCwzNV1d
    struct gdt_entry *code_segment = (struct gdt_entry *)((unsigned char *)global_descriptor_table + GDT_KERNEL_CODE_SEGMENT);
    code_segment->read_write = 1;
    code_segment->executable = 1;
    code_segment->non_system = 1;
    code_segment->present = 1;
    code_segment->long_mode = 1;

    // Create kernel data segment (at 0x10)
    // Format of this segment: https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDE1NSxbMTU1LDQ3LDE1NSw0N11d
    struct gdt_entry *data_segment = (struct gdt_entry *)((unsigned char *)global_descriptor_table + GDT_KERNEL_DATA_SEGMENT);
    data_segment->read_write = 1;
    data_segment->non_system = 1;
    data_segment->present = 1;
    data_segment->long_mode = 1;

    // Task state format: https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDQyMCxbNDIwLDksNDIwLDldXQ==
    struct gdt_task_state *task_state = memory_physical_allocate();
    memory_zero(task_state, sizeof(struct gdt_task_state));

    console_print("[gdt] allocate task_state at 0x");
    console_print_u64((unsigned long)task_state, 16);
    console_new_line();

    // Allocate interrupt stacks, add 4096 because the stack grows down, see enum idt_stack_type in gdt.h for more information
    task_state->ist1 = (unsigned long)memory_physical_allocate() + 4096ul;
    task_state->ist2 = (unsigned long)memory_physical_allocate() + 4096ul;

    console_print("[gdt] allocate ist1 at 0x");
    console_print_u64(task_state->ist1, 16);
    console_new_line();

    // Create kernel task segment (at 0x18)
    // Format of this segment: https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDE1NyxbMTU3LDM5LDE1NywzOV1d
    struct gdt_system_entry *task_segment = (struct gdt_system_entry *)((unsigned char *)global_descriptor_table + GDT_KERNEL_TASK_SEGMENT);

    // When .non_system = 0, the following four fields decribe the type of system segment, a list of available system segments (in 64 bit mode) can be found here:
    // https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMi5wZGYiLDE1NixbMTU2LDE1OCwxNTYsMTY4XV0=
    // Set system segment type to 0b1001, which means task state segment
    task_segment->base.accessed = 1;
    task_segment->base.read_write = 0;
    task_segment->base.direction_conforming = 0;
    task_segment->base.executable = 1;

    unsigned long base = (unsigned long)task_state;
    unsigned int limit = sizeof(struct gdt_task_state);
    task_segment->base.present = 1;
    task_segment->base.limit1 = limit & 0xFFFFul;
    task_segment->base.limit2 = (limit >> 16) & 0xFFul;
    task_segment->base.base1 = base & 0xFFFFul;
    task_segment->base.base2 = (base >> 16) & 0xFFul;
    task_segment->base.base3 = (base >> 24) & 0xFFul;
    task_segment->base4 = (base >> 32) & 0xFFFFFFFFul;

    // Load new global descriptor table, passing the GDT pointer
    // Instruction info: https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMy5wZGYiLDQ0MyxbNDQzLDQ2LDQ0Myw0Nl1d
    struct gdt_pointer pointer = {
        .address = global_descriptor_table,
        .limit = sizeof(struct gdt_entry) * 3 + sizeof(struct gdt_system_entry),
    };
    asm volatile("lgdt [%0]" ::"rm"(pointer));

    // Load the task segment from the GDT at index 0x18
    // Instruction info: https://kokos.run/#WzAsIkFNRDY0Vm9sdW1lMy5wZGYiLDQ1MixbNDUyLDQ0LDQ1Miw0NF1d
    asm volatile("ltr %0" ::"r"(GDT_KERNEL_TASK_SEGMENT));

    // Set current cpu information
    struct cpu *cpu = cpu_get_current();
    cpu->global_descriptor_table = global_descriptor_table;
}