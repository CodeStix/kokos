#pragma once

// Returns non-zero if the lock has been acquired. Release this lock using lock_release
int lock_try_acquire(int *lock_state);

// Waits until the current CPU has acquired the lock. Release this lock using lock_release
void lock_acquire(int *lock_state);

// Releases the lock previously acquired by this CPU
void lock_release(int *lock_state);