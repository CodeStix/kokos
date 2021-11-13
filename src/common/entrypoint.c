#include "kokos/cpu.h"
#include "kokos/console.h"
#include "kokos/apic.h"
#include "kokos/memory_physical.h"
#include "kokos/idt.h"

extern unsigned short cpu_startup_increment;
extern unsigned short cpu_startup_done;

void cpu_entrypoint()
{

    // console_print("[cpu] starting cpu\n");
    // cpu_startup_increment = 1;
    // return;

    // Cpu *cpu_info = cpu_initialize();
    // idt_initialize();

    cpu_wait_millisecond();

    // console_print("[cpu] cpu info at 0x");
    // console_print_u32(cpu_info, 16);
    // console_print(" and fs is 0x");
    // console_print_u32(cpu_read_msr(CPU_MSR_FS_BASE), 16);
    // console_print(" or 0x");
    // console_print_u32(cpu_get_current(), 16);
    // console_new_line();

    // console_print("[cpu] current cpu id ");
    // console_print_u32(cpu_info->id, 10);
    // console_print(" and fs is ");
    // console_print_u32(cpu_get_current()->id, 10);
    // console_new_line();

    // console_print("[cpu] enabling schedule interrupt\n");

    // asm volatile("cli");
    // idt_register_interrupt(0x22, interrupt_schedule, IDT_GATE_TYPE_INTERRUPT);
    // asm volatile("sti");

    // Enable local APIC by setting bit 8 in the spurious_interrupt_vector register
    // cpu_info->local_apic_physical->spurious_interrupt_vector = 0x1FF;
    // cpu_info->local_apic_physical->timer_initial_count = 10000000;
    // cpu_info->local_apic_physical->timer_divide_config = 0b1010;
    // cpu_info->local_apic_physical->timer_vector = 0x22 | (1 << 17);

    // console_print("[cpu] done\n");

    // unsigned long rip;
    // asm volatile("push rip; pop %0"
    //              : "=r"(rip)::);
    // console_print("[cpu] rip = 0x");
    // console_print_u64(rip, 16);
    // console_new_line();

    cpu_startup_increment = 1;
    while (1)
    {
        asm volatile("hlt");
    }
}