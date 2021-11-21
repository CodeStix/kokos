# Code style

One of the mail goals of this operating system is readability. That is why the following naming conventions are enforced:

- Use `UPPER_SNAKE_CASE` for constant defines. `#define TEST_VALUE 100`
- Don't use const variables, use defines.
- Every function and type must start with its namespace. For example, every function that has something to do with console, should look like this:

    ```c
    int main() {
        console_clear();
        console_print("this is a test");
        console_print_i32(string_length("test!"));

        void* ptr = memory_allocate(200);
        // ...
        ptr = memory_resize(ptr);
        // ...
        memory_free(ptr);
    }

    struct console_context {
        // ...
    };
    ```

- Use `lower_snake_case` for function, struct, union, enum and variable names.
- Use `lower_snake_case` for struct names. 
- DON'T use typedefs for structs, specify struct when using them

    ```c
    struct person {
        char* name;
        int age;
    } ;

    // struct person p = ...

    ```

- Start every comment capitalized and before the line (NOT after) it will describe.

    ```c
    int main() {
        // The following line prints to the screen.
        console_print("text here");
    }
    ```

- When using a shared/header struct between multiple types, use 'base' as the name for the 'inherited' struct:

    ```c

    struct table {
        int type; // This field can determine if it is a PointerTable or a PICTable
        int length;
    } ;

    struct pointer_table {
        struct table base; // Always call it base
        void* pointer;
    } ;

    struct pic_table {
        struct table base; // Always call it base
        unsigned short pic_number;
    } ;

    ```

- 'Length' refers to the amount of item in a sequence, array or list.
- 'Size' refers to the amount of bytes some structure takes.
- Every enumeration/iteration function should follow the following format:

    ```c
    // definition
    next_item namespace_type_iterate(the_thing_to_iterate, previous_item);
    ```

    Example usage:

    ```c
        struct acpi_madt_entry_io_apic *current_ioapic = 0;
        while (current_ioapic = acpi_madt_iterate_type(madt, current_ioapic, ACPI_MADT_TYPE_IO_APIC))
        {
            console_print("io apic at 0x");
            console_print_u64(current_ioapic->io_apic_address, 16);
            console_print(" with irq ");
            console_print_u64(current_ioapic->global_system_interrupt_base, 10);
            console_new_line();
        }
    ```