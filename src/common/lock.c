#include "kokos/lock.h"

int lock_try_acquire(int *lock_state)
{
    int compare_value = 0;

    // The cmpxchg instruction will compare compare_value (eax register) and first its operand lock_state, if they are equal,
    // it will copy the second operand into lock_state. Otherwise, it will copy its first operand lock_state into compare_value (eax register).
    asm volatile("lock cmpxchg %1, %3"
                 : "=a"(compare_value), "=m"(*lock_state)
                 : "a"(compare_value), "r"(1)
                 : "memory");

    // If compare_value is 1, it means the comparison has failed (another CPU was probably first)
    return !compare_value;
}

void lock_acquire(int *lock_state)
{
    while (1)
    {
        int compare_value = 0;

        asm volatile("lock cmpxchg %1, %3"
                     : "=a"(compare_value), "=m"(*lock_state)
                     : "a"(compare_value), "r"(1)
                     : "memory");

        if (!compare_value)
            return;

        // Keep looping while the lock is taken, then, try to aquire the lock again
        do
        {
            asm volatile("pause" ::
                             : "memory");
        } while (*lock_state);
    }
}

void lock_release(int *lock_state)
{
    // Make sure all memory operations have been done before unlocking the lock
    // This is called a 'memory barrier'
    asm volatile("" ::
                     : "memory");
    *lock_state = 0;
}