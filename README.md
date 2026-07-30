# Linux Programming Summative Project — ELF Analysis, Assembly, C Extensions, Multithreading & Networking

This repository contains solutions for **5 Projects** covering core Linux programming concepts:

| Project       | Topic                              | Key Concepts                                   |
| ------------- | ---------------------------------- | ---------------------------------------------- |
| **Project 1** | ELF Binary Investigation           | Static/dynamic analysis, strace, GDB, Assembly |
| **Project 2** | Assembly-Based Text File Analysis  | NASM, x86 syscalls, file I/O in assembly       |
| **Project 3** | Python C Extension for Sensor Data | Python/C API, C modules, setuptools            |
| **Project 4** | POSIX Threads Synchronization      | pthreads, mutexes, order processing            |
| **Project 5** | Socket Programming (Client/Server) | TCP sockets, select(), IPC, chat server        |

---

## Repository Structure

```
linux_programming_Summative_Project/
├── README.md                          ← This file (project-wide overview)
├── PROJECT 1/
│   ├── README.md                      ← Detailed ELF analysis report
│   ├── analysis_report.md             ← Full project analysis report
│   ├── demo_script.md                 ← GDB/strace demo script
│   ├── program.c                      ← C source code
│   ├── program                        ← Stripped ELF binary
│   └── strace_output.txt              ← Full strace trace log
├── PROJECT 2/
│   ├── README.md                      ← Assembly analysis documentation
│   ├── sensor_analysis.asm            ← NASM assembly source
│   └── sensor_readings.txt            ← Input data file for assembly program
├── PROJECT 3/
│   ├── README.md                      ← C extension documentation
│   ├── sensor_analysis.c              ← Python C extension source
│   ├── setup.py                       ← Build configuration (setuptools)
│   └── test_sensor_analysis.py        ← Python test suite
├── PROJECT 4/
│   ├── README.md                      ← Multithreading documentation
│   ├── Makefile                       ← Build automation
│   └── order_processing.c             ← Pthread order processing program
└── PROJECT 5/
    ├── README.md                      ← Socket programming documentation
    ├── Makefile                       ← Build automation
    ├── client.c                       ← TCP client program
    └── server.c                       └ TCP server program
```

---

## Project 1 — ELF Binary Investigation

### Description

Investigates the structure and execution of an ELF (Executable and Linkable Format) binary. A C program was developed, compiled, stripped, and analyzed using **static analysis** (`readelf`, `objdump`), **dynamic analysis** (`strace`), and **debugging** (`gdb`).

### Program: `program.c`

Generates random integers, performs statistical analysis (min, max, sum, average), and displays a formatted report.

### Key Requirements Satisfied

| Requirement                       | Implementation                                                             |
| --------------------------------- | -------------------------------------------------------------------------- |
| 3 user-defined functions + main() | `generate_data()`, `analyze_data()`, `display_report()` + `main()`         |
| Global variable                   | `g_total_numbers`                                                          |
| Loops                             | `for` loops for data generation, analysis, and printing                    |
| Decision-making                   | `if` for min/max comparison, input validation                              |
| Dynamic memory allocation         | `malloc()` in `generate_data()`                                            |
| Standard library functions        | `printf()`, `scanf()`, `rand()`, `srand()`, `time()`, `malloc()`, `free()` |

### Build & Run

```bash
gcc -Wall -O0 -fno-inline -o program program.c
strip program
./program
```

### Analysis Performed

1. **Static Analysis**: ELF header, section headers (.text, .data, .bss, .plt, .got, .rodata), dynamic linking
2. **Function Reconstruction**: All 4 functions mapped from assembly to C with addresses
3. **Dynamic Analysis**: ~49 system calls across 8 categories using `strace`
4. **Memory Inspection**: Stack, heap, and global memory regions analyzed in GDB
5. **Security Features**: PIE, NX, RELRO, and stack canaries verified

### Key Findings

- **Architecture**: x86-64, little endian, dynamically linked PIE executable
- **Entry Point**: `0x11a0` (`_start`), not `main()` at `0x163f`
- **Dependencies**: Only `libc.so.6`
- **malloc() → brk()**: Heap expansion via `brk()` syscall
- **free() NOT visible**: Libc caches freed blocks; no `brk`/`munmap` for free in strace
- **Stripping**: Reduced binary size by ~12.7%

### Detailed Report

See `PROJECT 1/README.md` and `PROJECT 1/analysis_report.md` for the complete analysis.

---

## Project 2 — Assembly-Based Text File Analysis

### Description

A **NASM assembly program** that reads a text file (`sensor_readings.txt`) containing sensor measurements (one per line) and counts:

- **Total records** (total number of lines)
- **Valid records** (non-empty lines)

### Assembly Program: `sensor_analysis.asm`

Traverses the file buffer character by character, handling both Unix (`\n`) and Windows (`\r\n`) line endings.

### Build & Run (Linux)

```bash
nasm -f elf32 -o sensor_analysis.o sensor_analysis.asm
ld -m elf_i386 -o sensor_analysis sensor_analysis.o
./sensor_analysis
```

### Key Subroutines

| Subroutine      | Purpose                                                                 |
| --------------- | ----------------------------------------------------------------------- |
| `_start`        | Entry point: open file, read into buffer, close, process, display, exit |
| `count_records` | Traverse buffer character-by-character, count total and valid lines     |
| `print_results` | Display "Total records:" and "Valid records:" with values               |
| `print_number`  | Convert integer to ASCII string and print via `sys_write`               |

### Line Ending Handling

- **Unix (LF)**: `\n` (0x0A) — single character terminator
- **Windows (CRLF)**: `\r\n` (0x0D, 0x0A) — skips `\r`, counts `\n` as line end
- **Unterminated final line**: Detected and counted after buffer end

### Input Data (`sensor_readings.txt`)

Contains several sensor readings with empty lines interspersed to test proper handling.

### Error Handling

- File open failure → "Error: Unable to open file"
- Read failure → "Error: Failed to read file contents"
- Empty file → "Error: File is empty"

### System Calls Used

| Syscall     | Number | Purpose                        |
| ----------- | ------ | ------------------------------ |
| `sys_open`  | 5      | Open file for reading          |
| `sys_read`  | 3      | Read file contents into buffer |
| `sys_close` | 6      | Close file descriptor          |
| `sys_write` | 4      | Output results to stdout       |
| `sys_exit`  | 1      | Terminate program              |

---

## Project 3 — Python C Extension for Sensor Data Analysis

### Description

A **Python C extension module** (`sensor_analysis`) that performs statistical operations on sensor data directly in C for improved performance. The module provides high-performance data analysis for smart agriculture/IoT sensor data.

### Module Functions

| Function                   | Description                                        | Time Complexity  |
| -------------------------- | -------------------------------------------------- | ---------------- |
| `average(data)`            | Arithmetic mean of sensor readings                 | O(n)             |
| `range_value(data)`        | Difference between max and min                     | O(n)             |
| `variance(data)`           | Sample variance (two-pass for numerical stability) | O(n)             |
| `count_above(data, limit)` | Count readings > limit                             | O(n)             |
| `statistics(data)`         | Return dict with samples, avg, min, max            | O(n) single pass |

### Build Instructions

```bash
# Ensure Python development headers are installed (Linux):
sudo apt-get install python3-dev

# Build the extension in-place:
python setup.py build_ext --inplace

# Test the module:
python test_sensor_analysis.py
```

### Python API Usage

```python
import sensor_analysis

data = [23.5, 24.1, 22.8, 23.9, 25.0]
avg = sensor_analysis.average(data)
rng = sensor_analysis.range_value(data)
var = sensor_analysis.variance(data)
count = sensor_analysis.count_above(data, 24.0)
stats = sensor_analysis.statistics(data)
# stats == {'samples': 5, 'average': 23.86, 'minimum': 22.8, 'maximum': 25.0}
```

### Implementation Details

- **Memory Management**: Input accessed via borrowed references; output returned as Python objects
- **Validation**: `validate_and_extract()` helper validates input and converts to C double array
- **Numerical Accuracy**: Two-pass variance for numerical stability; `double` precision for sensor data
- **Error Handling**: Proper Python exceptions (TypeError, ValueError, MemoryError) with `PyErr_SetString`

### Input Types Supported

- Lists (`[1.0, 2.0, 3.0]`)
- Tuples (`(1.0, 2.0, 3.0)`)
- Single elements
- Negative values, very small numbers, all-identical values

---

## Project 4 — POSIX Threads: Order Processing with Synchronization

### Description

A **multi-threaded order processing program** that uses POSIX threads and mutex synchronization to process orders concurrently.

### Program: `order_processing.c`

Uses `pthread` to create worker threads that process orders from a shared queue, with:

- **Mutex** protection for shared data (order queue, statistics)
- **Condition variables** for thread coordination
- **Thread pool** pattern for efficient order processing

### Build & Run

```bash
make -C "PROJECT 4" all
./PROJECT 4/order_processing
```

or manually:

```bash
gcc -O2 -pthread -Wall -Wextra "PROJECT 4/order_processing.c" -o "PROJECT 4/order_processing"
./PROJECT 4/order_processing
```

### Key Features

| Feature               | Implementation                                     |
| --------------------- | -------------------------------------------------- |
| Thread creation       | `pthread_create()` for worker threads              |
| Mutex synchronization | `pthread_mutex_lock/unlock` for shared data        |
| Condition variables   | `pthread_cond_wait/signal` for thread coordination |
| Thread joining        | `pthread_join()` for clean shutdown                |
| Error handling        | `perror()` on pthread failures                     |

### Usage

Makefile included with targets:

```bash
make all        # Build the program
make clean      # Remove build artifacts
make run        # Build and execute
```

---

## Project 5 — Socket Programming: TCP Client-Server

### Description

A **TCP socket-based client-server system** implementing networked communication using **Berkeley sockets** API.

### Components

#### Server (`server.c`)

- Creates a TCP socket and binds to a port
- Listens for incoming connections
- Accepts multiple clients using `select()` or threading
- Handles client messages and broadcasts

#### Client (`client.c`)

- Connects to the server via TCP
- Sends messages from user input
- Receives responses from server

### Build & Run

```bash
make -C "PROJECT 5" all
# Terminal 1 (server):
./PROJECT 5/server

# Terminal 2 (client):
./PROJECT 5/client
```

### Key Concepts Demonstrated

| Concept              | Implementation                      |
| -------------------- | ----------------------------------- |
| Socket creation      | `socket(AF_INET, SOCK_STREAM, 0)`   |
| Address binding      | `bind()` with `struct sockaddr_in`  |
| Connection listening | `listen()` with backlog             |
| Client acceptance    | `accept()` returning new fd         |
| Data exchange        | `send()` / `recv()`                 |
| I/O multiplexing     | `select()` for multiple connections |
| Clean shutdown       | Graceful `close()` of sockets       |

---

## How to Build Everything

### Prerequisites

- **Linux** environment (or WSL2 for Windows users)
- **GCC** compiler with pthread support
- **NASM** assembler (for Project 2)
- **Python 3** with development headers (for Project 3)

### Build Commands by Project

#### Project 1

```bash
gcc -Wall -O0 -fno-inline -o "PROJECT 1/program" "PROJECT 1/program.c"
strip "PROJECT 1/program"
```

#### Project 2

```bash
nasm -f elf32 -o "PROJECT 2/sensor_analysis.o" "PROJECT 2/sensor_analysis.asm"
ld -m elf_i386 -o "PROJECT 2/sensor_analysis" "PROJECT 2/sensor_analysis.o"
```

#### Project 3

```bash
cd "PROJECT 3" && python3 setup.py build_ext --inplace
```

#### Project 4

```bash
make -C "PROJECT 4" all
```

#### Project 5

```bash
make -C "PROJECT 5" all
```

---

## Quick Reference

| Project | Language/Framework   | Key APIs                                                                        |
| ------- | -------------------- | ------------------------------------------------------------------------------- |
| 1       | C (ELF Analysis)     | `readelf`, `objdump`, `strace`, `gdb`                                           |
| 2       | NASM Assembly (x86)  | `int 0x80` syscalls: open, read, write, close, exit                             |
| 3       | Python/C API         | `PyArg_ParseTuple`, `PySequence_GetItem`, `PyFloat_FromDouble`, `Py_BuildValue` |
| 4       | C (POSIX Threads)    | `pthread_create`, `pthread_mutex_lock/unlock`, `pthread_cond_wait/signal`       |
| 5       | C (Berkeley Sockets) | `socket`, `bind`, `listen`, `accept`, `send`, `recv`, `select`                  |

---
