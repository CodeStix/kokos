#include "cpu.h"
#include "console.h"
#include "apic.h"
#include "memory_physical.h"

static int current_cpu_id = 0;

void cpu_entrypoint()
{
    console_print("[cpu] starting cpu\n");

    // Get the local APIC address
    unsigned long local_apic_info = cpu_read_msr(CPU_MSR_LOCAL_APIC);
    Apic *local_apic = local_apic_info & 0x000ffffffffff000;

    // Check 11th bit to check if it is enabled
    if (!(local_apic_info & 0x800))
    {
        console_print("[cpu] fatal: local apic not enabled, cannot enable\n");
        return;
    }

    console_print("[cpu] local_apic = 0x");
    console_print_u64((unsigned long)local_apic, 16);
    console_new_line();

    // The FS segment register will point to cpu-specific information
    struct Cpu *cpu_info = memory_physical_allocate();
    cpu_info->address = cpu_info;
    cpu_info->id = current_cpu_id++;
    cpu_info->local_apic;
    cpu_write_msr(CPU_MSR_FS_BASE, cpu_info);

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