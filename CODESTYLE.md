# Code style

One of the mail goals of this operating system is readability. That is why the following naming conventions are enforced:

- Use `UPPER_SNAKE_CASE` for constant defines. `#define TEST_VALUE 100`
- Don't use const variables, use defines.
- Every function (excluding some utility functions) must start with a namespace. For example, every function that has something to do with console, should look like this:

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
    ```

- Use `lower_snake_case` for function names. `string_length(...)`
- Use `UpperCaseCamelCase` for struct names. (even shorts like `ACPI` are written as `Acpi`, to stop confusion like this: `ACPIXSDT` -> `AcpiXsdt`)
- Use typedef for every struct.

    ```c
    typedef struct {
        char* name;
        int32 age;
    } Person;
    ```

- Every struct name should be prefixed with ist namespace

    ```c
    // in memory.c

    typedef struct {
        void* location;
        unsigned int size;
        unsigned short flags;
    } MemoryAllocatedChunk;

    ```

- Use `lower_snake_case` for variables:

    ```c
    int main() {
        char* name_string = "your name";
        console_print(name_string);
    }
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

    typedef struct {
        int type; // this field can determine if it is a PointerTable or a PICTable
        int length;
    } Table;

    typedef struct {
        Table base; // always call it base
        void* pointer;
    } PointerTable;

    typedef struct {
        Table base; // always call it base
        unsigned short pic_number;
    } PICTable;

    ```

- Length refers to the amount of item in a sequence, array or list.
- Size refers to the amount of bytes some structure takes.
