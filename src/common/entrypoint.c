#include "cpu.h"
#include "console.h"
#include "apic.h"

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

    cpu_wait_millisecond();
}