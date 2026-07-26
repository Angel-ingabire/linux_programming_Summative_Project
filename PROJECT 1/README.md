# Project 1: Investigating an ELF Executable

## Overview

This project investigates the structure and execution of an ELF (Executable and Linkable Format) binary. A C program was developed, compiled, stripped, and analyzed using static analysis (`readelf`, `objdump`), dynamic analysis (`strace`), and debugging (`gdb`).

---

## Program: `program.c`

### Description

The program generates a user-specified number of random integers, performs statistical analysis (min, max, sum, average), and displays a formatted report.

### Requirements Satisfied

| Requirement                       | Implementation                                                                                   |
| --------------------------------- | ------------------------------------------------------------------------------------------------ |
| 3 user-defined functions + main() | `generate_data()`, `analyze_data()`, `display_report()` + `main()`                               |
| Global variable                   | `g_total_numbers` - tracks number of data points processed globally                              |
| Loops                             | Multiple `for` loops for data generation, analysis loop, and printing                            |
| Decision-making                   | `if` statements for min/max comparison, input validation (`if (count <= 0)`)                     |
| Dynamic memory allocation         | `malloc()` in `generate_data()`                                                                  |
| Standard library function         | `printf()`, `scanf()`, `rand()`, `srand()`, `time()`, `malloc()`, `free()`, `fwrite()`, `exit()` |
| Meaningful output                 | Formatted data analysis report with min, max, sum, average                                       |

### User-defined Functions

1. **`int* generate_data(int count)`** - Allocates memory and fills with random values (0-999)
2. **`void analyze_data(int* data, int count, int* min, int* max, int* sum)`** - Finds min, max, and sum
3. **`void display_report(int* data, int count, int min, int max, int sum)`** - Prints formatted report

---

## Compilation and Execution

### Compilation

```bash
# Compile with no optimizations and no inlining
gcc -Wall -O0 -fno-inline -o program program.c

# Strip debugging and symbol information
strip program
```

### Execution

```bash
./program
```

### Input

The program prompts for the number of data points to generate (integer).

### Expected Output

```
ELF Binary Investigation - Project 1
====================================
Enter the number of data points to generate: 5

========================================
      DATA ANALYSIS REPORT
========================================
Total numbers processed (global): 5
Array size analyzed:              5
----------------------------------------
Generated Data: [896, 214, 784, 285, 927]
----------------------------------------
Minimum value:  214
Maximum value:  927
Sum of values:  3106
Average value:  621.20
========================================
Memory successfully freed. Program terminating.
```

---

## Static Analysis

### ELF Header (`readelf -h program`)

| Field        | Value                                  |
| ------------ | -------------------------------------- |
| Architecture | x86-64 (Advanced Micro Devices X86-64) |
| Entry Point  | `0x11a0`                               |
| Class        | ELF64                                  |
| Data         | 2's complement, little endian          |
| Type         | DYN (Position-Independent Executable)  |
| OS/ABI       | UNIX - System V                        |

### Section Analysis (`readelf -S program`)

| Section   | Offset | Size               | Purpose                                                                     |
| --------- | ------ | ------------------ | --------------------------------------------------------------------------- |
| `.text`   | 0x11a0 | 0x5b2 (1458 bytes) | Contains executable code (program instructions)                             |
| `.data`   | 0x4000 | 0x10 (16 bytes)    | Initialized global/static variables                                         |
| `.bss`    | 0x4020 | 0x10 (16 bytes)    | Uninitialized global/static variables (zero-initialized at runtime)         |
| `.plt`    | 0x1020 | 0xc0 (192 bytes)   | Procedure Linkage Table - stub code for calling dynamic library functions   |
| `.got`    | 0x3f68 | 0x98 (152 bytes)   | Global Offset Table - pointers to global symbols resolved by dynamic linker |
| `.rodata` | 0x2000 | 0x290 (656 bytes)  | Read-only data (string literals, constants)                                 |
| `.interp` | 0x318  | 0x1c (28 bytes)    | Path to dynamic linker/loader (`/lib64/ld-linux-x86-64.so.2`)               |
| `.init`   | 0x1000 | 0x1b (27 bytes)    | Code executed before main() (C runtime initialization)                      |
| `.fini`   | 0x1754 | 0xd (13 bytes)     | Code executed after program termination                                     |
| `.dynsym` | 0x3d8  | 0x1b0 (432 bytes)  | Dynamic symbol table                                                        |
| `.dynstr` | 0x588  | 0xf2 (242 bytes)   | Dynamic symbol string table                                                 |

### Linking

The executable is **dynamically linked** (type DYN = Position-Independent Executable). It requires `/lib64/ld-linux-x86-64.so.2` as the dynamic linker and depends on `libc.so.6`.

### Function Reconstruction from Assembly

#### `main()` (0x163f)

1. Prints title banner (`puts`)
2. Prints separator lines
3. Calls `time(0)` and `srand()` to seed random generator
4. Calls `printf()` to prompt user input
5. Calls `scanf()` to read count
6. Tests if count <= 0 (conditional branch at 0x16bd: `test %eax,%eax` / `jg` jump-if-greater)
7. Calls `generate_data()` (0x1289) passing count
8. Calls `analyze_data()` (0x1344) passing data, count, &min, &max, &sum
9. Calls `display_report()` (0x1474) passing data, count, min, max, sum
10. Calls `free()` to release memory
11. Returns 0

#### `generate_data()` (0x1289)

1. Allocates memory via `malloc` (`count * sizeof(int)` = `count * 4`)
2. If malloc returns NULL, prints error via `fwrite` to stderr and calls `exit(1)` (conditional branch at 0x12ad: `cmpq $0x0,-0x8(%rbp)` / `jne`)
3. Loop from i=0 to count-1 (0x132d: `cmp` / `jl` jump-if-less):
   - Calls `rand()` and computes `rand() % 1000` (remainder after division by 1000)
   - Stores result in `data[i]`
4. Sets global variable `g_total_numbers` = count
5. Returns pointer to allocated data

#### `analyze_data()` (0x1344)

1. Checks if data is NULL or count <= 0; if so, prints error and returns (conditional branch at 0x1363-0x136e)
2. Initializes: `*min = data[0]`, `*max = data[0]`, `*sum = 0`
3. Loop from i=0 to count-1:
   - **Conditional branch (min update)**: `if (data[i] < *min)` → at 0x13e2: `cmp %eax,%edx` / `jge` (jump-if-greater-or-equal to skip update)
   - **Conditional branch (max update)**: `if (data[i] > *max)` → at 0x141e: `cmp %eax,%edx` / `jle` (jump-if-less-or-equal to skip update)
   - `*sum += data[i]`

#### `display_report()` (0x1474)

1. Prints report header using `puts()` and `printf()`
2. Reads and prints global variable `g_total_numbers` (at 0x402c)
3. Loop from i=0 to count-1:
   - Conditional check: if `i < count-1` print with comma, else print closing bracket
   - Calls `printf("%3d, ", data[i])` or `printf("%3d", data[i])` accordingly
4. Prints min, max, sum, average (computed via `cvtsi2sd` float conversion and `divsd` division)

### Conditional Branch Example

At address `0x16bb-0x16bd` in main():

```assembly
16bb:  test   %eax,%eax          ; Check if count <= 0
16bd:  jg     0x16d5              ; Jump if greater than 0
```

This corresponds to: `if (count <= 0) { count = 10; }`

### Loop Example

At address `0x132d-0x1333` in generate_data():

```assembly
132d:  mov    -0xc(%rbp),%eax    ; Load loop counter i
1330:  cmp    -0x14(%rbp),%eax   ; Compare i with count
1333:  jl     0x12ea              ; Jump if less (continue loop)
```

This corresponds to: `for (int i = 0; i < count; i++)`

---

## Dynamic Analysis (`strace`)

### System Calls Classification

#### Program Start-up & Dynamic Linking

| syscall           | Purpose                                                                      |
| ----------------- | ---------------------------------------------------------------------------- |
| `execve`          | Execute the program                                                          |
| `brk(NULL)`       | Get initial program break (heap start)                                       |
| `mmap` (x6)       | Map memory for shared libraries (ld.so, libc.so.6), anonymous mappings, etc. |
| `access`          | Check `/etc/ld.so.preload` for preloaded libraries                           |
| `openat`          | Open `/etc/ld.so.cache` and `/lib/x86_64-linux-gnu/libc.so.6`                |
| `read`            | Read ELF headers of libc                                                     |
| `close`           | Close file descriptors                                                       |
| `arch_prctl`      | Set thread-local storage (FS segment)                                        |
| `set_tid_address` | Set thread ID address                                                        |
| `set_robust_list` | Set robust futex list                                                        |
| `rseq`            | Restartable sequences initialization                                         |
| `mprotect`        | Set memory protection (make GOT read-only after relocation)                  |
| `prlimit64`       | Get stack resource limits                                                    |
| `munmap`          | Unmap temporary memory (ld.so cache)                                         |
| `getrandom`       | Seed random number generator (libc initialization)                           |

#### Memory Management

| syscall               | Purpose                                              |
| --------------------- | ---------------------------------------------------- |
| `brk(0x559445314000)` | Extend heap for dynamic memory allocation (`malloc`) |

#### Input/Output

| syscall                | Purpose                                                    |
| ---------------------- | ---------------------------------------------------------- |
| `fstat(1, ...)`        | Get terminal file descriptor info (stdout)                 |
| `write(1, ...)`        | Write output to terminal (16 calls for printf/puts output) |
| `fstat(0, ...)`        | Get terminal file descriptor info (stdin)                  |
| `read(0, "5\n", 4096)` | Read user input from terminal (scanf)                      |

#### Program Termination

| syscall         | Purpose                                         |
| --------------- | ----------------------------------------------- |
| `lseek`         | Attempted seek on pipe (ignored - ESPIPE error) |
| `exit_group(0)` | Terminate all threads with status 0             |

### Memory Allocation Explanation

The `malloc(20)` call in `generate_data()` triggers the `brk()` system call to expand the heap. The standard library's memory allocator manages the heap region between the program break and previously allocated memory. `free()` returns memory to the allocator but typically does not return it to the OS immediately (no `brk` or `munmap` for free in strace because the allocator caches freed blocks for reuse).

### Terminal Output Explanation

Each `printf()`/`puts()` call in the C code results in a `write()` system call to file descriptor 1 (stdout). The `fwrite()` calls for error messages would go to file descriptor 2 (stderr), but no errors occurred during this execution.

---

## Debugging and Memory Inspection (`gdb`)

### Breakpoints

```gdb
# Break at the program entry point (_start)
break *0x11a0

# Break at main() - address found via objdump
break *0x163f

# Break at generate_data()
break *0x1289

# Break at analyze_data()
break *0x1344

# Break at display_report()
break *0x1474
```

### Call Stack Inspection

```gdb
# After hitting breakpoint in generate_data():
bt
#0  generate_data (count=5) at program.c:22
#1  0x00005555555556df in main () at program.c:109
```

### Memory Inspection

```gdb
# Inspect global variable g_total_numbers (at address 0x402c from .data section)
x/d &g_total_numbers      # Display global variable value
p g_total_numbers         # Print global variable

# Inspect dynamically allocated memory (heap)
info registers            # Find pointer in rax/rdi after malloc returns
x/10xw <address>          # View 10 words of heap data

# Inspect local variable on stack
info locals               # Print all local variables in current frame
x/d &count                # Inspect count variable on stack
x/d &i                    # Inspect loop counter on stack
```

### Stack, Heap, and Global Memory Differences

| Memory Region     | Location                              | Characteristics                                                                                                                                                                    |
| ----------------- | ------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Stack**         | High addresses (grows downward)       | Stores local variables (`count`, `i`, `min`, `max`, `sum`), function parameters, return addresses. Managed automatically via push/pop. Lifetime = function scope. Fast allocation. |
| **Heap**          | Between data and stack (grows upward) | Stores dynamically allocated data (`dataset` array). Managed by `malloc`/`free`. Lifetime = until explicitly freed. Slower allocation but flexible size.                           |
| **Global/Static** | `.data` / `.bss` sections             | Stores `g_total_numbers` (in `.data` since initialized). Exists for entire program lifetime. Fixed address determined at link time.                                                |

---

## Security Features

| Feature          | Status  | Details                                                                                                                            |
| ---------------- | ------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| **PIE**          | Enabled | Type: DYN (Position-Independent Executable) - code can be loaded at random base address (ASLR)                                     |
| **NX**           | Enabled | Readable sections (`.rodata`) and writable sections (`.data`, `.bss`, `.got`) have separate permissions - no execute on stack/heap |
| **RELRO**        | Partial | `.got` is writable during dynamic linking but can be made read-only after resolution (Full RELRO requires additional linker flags) |
| **Stack Canary** | Present | `__stack_chk_fail@plt` in PLT, and `mov %fs:0x28` / `sub %fs:0x28` in `main()`                                                     |

---

## Files Submitted

| File                | Description                                          |
| ------------------- | ---------------------------------------------------- |
| `program.c`         | Source code                                          |
| `program`           | Stripped ELF executable (64-bit, dynamically linked) |
| `README.md`         | This file - compilation, execution, analysis report  |
| `strace_output.txt` | Full strace output from program execution            |

---

## Summary

This project demonstrates a complete understanding of:

- ELF binary structure and sections
- Static analysis using `readelf` and `objdump`
- Dynamic analysis using `strace`
- Debugging and memory inspection using `gdb`
- Program execution flow from `_start` through C runtime to `main()`
- PLT/GOT operation for dynamic linking
- Effect of symbol stripping on the binary
- Memory organization: stack, heap, global data
- Security features: PIE, NX, RELRO, stack canaries
