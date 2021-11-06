#include "cpu.h"
#include "console.h"
#include "apic.h"
#include "memory_physical.h"
#include "interrupt.h"

void cpu_entrypoint()
{
    console_print("[cpu] starting cpu\n");

    struct Cpu *cpu_info = cpu_initialize();
    interrupt_initialize();

    cpu_wait_millisecond();

    console_print("[cpu] cpu info at 0x");
    console_print_u32(cpu_info, 16);
    console_print(" and fs is 0x");
    console_print_u32(cpu_read_msr(CPU_MSR_FS_BASE), 16);
    console_print(" or 0x");
    console_print_u32(cpu_get_current(), 16);
    console_new_line();

    console_print("[cpu] current cpu id ");
    console_print_u32(cpu_info->id, 10);
    console_print(" and fs is ");
    console_print_u32(cpu_get_current()->id, 10);
    console_new_line();
}