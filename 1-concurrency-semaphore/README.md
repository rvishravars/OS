# 1-concurrency-semaphore

This directory contains three Linux kernel modules that demonstrate the use of semaphores for concurrency control between loadable modules.

## Files

- `lab1_semaphore1.c`:  
  Exports a named semaphore (`my_sem`) with an initial count of 1. This module acts as the semaphore provider for the other modules.

- `lab1_semaphore2.c`:  
  Imports and attempts to acquire the exported semaphore. If the semaphore is available, it acquires it and holds it until the module is unloaded. If not, it refuses to load.

- `lab1_semaphore3.c`:  
  Similar to `lab1_semaphore2.c`, this module also tries to acquire the semaphore on load and releases it on unload.

- `Makefile`:  
  Builds all three modules.

- `test.sh`:  
  Automated test script to build, load, and unload the modules in various sequences, checking correct semaphore behavior and logging kernel messages.

## Lock Type Used

These modules use a **binary semaphore** (implemented with the Linux kernel's `struct semaphore`) as a locking mechanism.  
A binary semaphore acts as a mutual exclusion (mutex) lock, ensuring that only one consumer module can hold the lock at a time. This prevents concurrent access to shared resources between modules, providing safe synchronization.

## Why `down_trylock` is Used

The consumer modules (`lab1_semaphore2.c` and `lab1_semaphore3.c`) use the `down_trylock()` function to attempt to acquire the semaphore.  
`down_trylock()` is a non-blocking call:  
- If the semaphore is available, it acquires it and the module loads successfully.
- If the semaphore is not available (already held by another module), it fails immediately and the module refuses to load.

This approach is used to avoid blocking the module loading process. Kernel module initialization functions should not sleep or block indefinitely, as this can cause system instability. Using `down_trylock()` ensures that the module either acquires the lock instantly or fails gracefully, maintaining safe and predictable module behavior.

## Usage

### Building the Modules

```sh
make
```

### Tested Environment

This code was tested under:

```
Linux raspberrypi 6.12.38-v7+ #1 SMP Sun Jul 20 16:40:04 NZST 2025