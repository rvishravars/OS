# 3-proc-filesystem-rw

This directory contains a Linux kernel module that demonstrates how to create a custom `/proc` filesystem entry with both read and write capabilities.

## Files

- `proc.c`:  
  Kernel module that creates a writable entry under `/proc/driver/my_proc/rw_info`. Users can read from and write to this entry, allowing two-way communication between user space and the kernel module.

- `Makefile`:  
  Builds the kernel module.

- `test.sh`:  
  Script to build, load, test, and unload the module, including writing to and reading from the `/proc/driver/my_proc/rw_info` entry.

## Functionality

The kernel module registers a new file in the `/proc` filesystem at `/proc/driver/my_proc/rw_info`.  
- **Read:** When a user reads this file (e.g., using `cat`), the module's read handler returns the current value or message stored in the module (e.g., `param = <value>`).
- **Write:** When a user writes to this file (e.g., using `echo "1234" > /proc/driver/my_proc/rw_info`), the module's write handler updates the stored value if the input is valid.

This mechanism is useful for exposing kernel parameters to user space and allowing user space to modify kernel module behavior at runtime.

## Usage

### Building the Module

```sh
make
```

### Loading the Module

```sh
sudo insmod proc.ko
```

### Interacting with the `/proc` Entry

After loading, you can read from and write to the custom `/proc` entry:

**Read:**
```sh
cat /proc/driver/my_proc/rw_info
```

**Write:**
```sh
echo "1234" | sudo tee /proc/driver/my_proc/rw_info
```

**Read again to verify:**
```sh
cat /proc/driver/my_proc/rw_info
```

### Unloading the Module

```sh
sudo rmmod proc
```

### Running the Test Script

```sh
./test.sh
```

This script automates the build, load, write, read, and unload steps, and checks for correct behavior and error handling.

## Tested Environment

This code was tested under:

```
Linux raspberrypi 6.12.38-v7+ #1 SMP Sun Jul 20 16:40:04 NZST 2025 armv7l GNU/Linux
```

## Notes

- The `/proc/driver/my_proc/rw_info` entry is created when the module is loaded and removed when the module is unloaded.
- Both reading from and writing to the `/proc` entry are supported.
- Invalid input (e.g., non-integer values) is rejected and does not change the stored value.
- Useful for learning about two-way communication between user space and kernel modules.

## License

See [../LICENSE](../LICENSE)