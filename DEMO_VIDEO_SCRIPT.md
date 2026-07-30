# 7-Minute Presentation Recording Script

**Target duration:** 7:00 (420 seconds)  
**Format:** Screen recording with voice-over  
**Environment:** Linux or WSL (Ubuntu recommended)

---

## Before You Record (Setup Checklist)

Open **6 terminal tabs** prepared in advance:

| Tab | Purpose | Command (do not run until script says so) |
|-----|---------|-------------------------------------------|
| 1 | Project 1 | `cd "PROJECT 1"` |
| 2 | Project 2 | `cd "PROJECT 2"` |
| 3 | Project 3 | `cd "PROJECT 3"` |
| 4 | Project 4 | `cd "PROJECT 4"` |
| 5 | Project 5 Server | `cd "PROJECT 5" && ./server` |
| 6 | Project 5 Client | `cd "PROJECT 5" && ./client` |

**Build everything once before recording:**

```bash
# Project 1
gcc -Wall -O0 -fno-inline -o program program.c && strip program

# Project 2
nasm -f elf32 -o sensor_analysis.o sensor_analysis.asm
ld -m elf_i386 -o sensor_analysis sensor_analysis.o

# Project 3
python3 setup.py build_ext --inplace

# Project 4
make -C "PROJECT 4" all

# Project 5
make -C "PROJECT 5" all
```

---

## SCRIPT

### [0:00 – 0:30] Introduction (~30 seconds)

**[ON SCREEN: Root README.md or project folder structure]**

**SAY:**
> "Hello. This presentation covers my Linux Programming Summative Project — five programs demonstrating ELF binary analysis, x86 assembly, Python C extensions, POSIX multithreading, and concurrent TCP client-server communication.
>
> Each project was implemented in C or assembly, compiled on Linux, and documented with a README covering build steps, inputs, and expected outputs. I'll walk through all five in the next seven minutes."

---

### [0:30 – 2:00] Project 1 — ELF Executable Investigation (~90 seconds)

**[ON SCREEN: `PROJECT 1/program.c` — scroll to the three functions and global variable]**

**SAY:**
> "Project 1 investigates how a C program becomes an ELF binary. My program defines three user functions — generate_data, analyze_data, and display_report — plus main. It uses a global variable, loops, if-statements, malloc for dynamic memory, and standard library calls like printf and rand."

**[ON SCREEN: Run compilation]**

```bash
gcc -Wall -O0 -fno-inline -o program program.c
strip program
echo 5 | ./program
```

**SAY:**
> "After compiling with no optimization and stripping symbols, the program generates random integers and prints a statistical report."

**[ON SCREEN: Quick static analysis — run these commands and show output]**

```bash
readelf -h program          # Show x86-64 architecture, entry point
readelf -S program | grep -E '\.(text|data|bss|plt|got)' 
file program                # Show dynamically linked PIE executable
```

**SAY:**
> "Static analysis with readelf shows this is a 64-bit dynamically linked PIE executable. The .text section holds code, .data holds initialized globals like g_total_numbers, .bss holds zero-initialized data, and .plt and .got handle dynamic library calls like malloc and printf.
>
> Using objdump, I reconstructed all four functions from assembly — for example, the for-loop in generate_data uses compare and jump-if-less instructions, and input validation in main uses test and jump-if-greater."

**[ON SCREEN: Show one strace and one gdb snippet from README or live]**

```bash
strace -e trace=memory,write,read ./program <<< "3" 2>&1 | head -20
```

**SAY:**
> "Dynamic analysis with strace shows system calls for startup, brk for heap expansion during malloc, write for terminal output, and exit_group at termination. In GDB, I set breakpoints at _start, main, and generate_data, inspected the call stack, and compared stack locals, heap data, and the global variable in .data."

---

### [2:00 – 2:45] Project 2 — Assembly Text File Analysis (~45 seconds)

**[ON SCREEN: `PROJECT 2/sensor_readings.txt` then `sensor_analysis.asm`]**

**SAY:**
> "Project 2 is an x86 assembly program using NASM and Linux syscalls. It opens sensor_readings.txt, reads the file into a buffer, and traverses it character by character to count total lines and valid non-empty lines. It handles both Unix LF and Windows CRLF line endings, with error handling if the file cannot be opened or is empty."

**[ON SCREEN: Build and run]**

```bash
nasm -f elf32 -o sensor_analysis.o sensor_analysis.asm
ld -m elf_i386 -o sensor_analysis sensor_analysis.o
./sensor_analysis
```

**SAY:**
> "The output shows Total records and Valid records. Empty lines are excluded from the valid count. The counting logic uses a loop with conditional branches — skipping carriage returns, detecting line feeds, and handling a final line without a trailing newline."

**[Expected output on screen]**
```
Total records: 13
Valid records: 9
```

---

### [2:45 – 3:45] Project 3 — Python C Extension (~60 seconds)

**[ON SCREEN: `PROJECT 3/sensor_analysis.c` — scroll to method table and one function]**

**SAY:**
> "Project 3 is a Python C extension module called sensor_analysis for high-performance IoT sensor data processing. All five required functions are implemented in C using the Python C API: average, range_value, variance, count_above, and statistics.
>
> Input lists or tuples are validated and converted to C doubles. Calculations use double precision, empty datasets raise Python exceptions, and unnecessary allocation is avoided except for a temporary C array during extraction."

**[ON SCREEN: Build and run test]**

```bash
python3 setup.py build_ext --inplace
python3 test_sensor_analysis.py
```

**SAY:**
> "The test script imports the module, runs every function on sample sensor data, and tests boundary conditions — empty lists raise ValueError, non-numeric input raises TypeError. Variance uses a two-pass algorithm for numerical stability, and all functions run in O(n) time."

**[ON SCREEN: Highlight one successful output block and one error-handling block from test output]**

---

### [3:45 – 5:00] Project 4 — Multithreaded Order Processing (~75 seconds)

**[ON SCREEN: `PROJECT 4/order_processing.c` — show mutex, condition variables, queue]**

**SAY:**
> "Project 4 simulates a food delivery platform with three POSIX threads. The kitchen thread is the producer — it prepares orders every two seconds and enqueues them. The delivery thread is the consumer — it dequeues and delivers orders every four seconds. The monitoring thread reports status every five seconds.
>
> Synchronization uses a mutex to protect the shared queue and counters, plus two condition variables — one signals when the queue is not full, the other when it is not empty. Queue capacity is fixed at five orders."

**[ON SCREEN: Run with a small order count for faster demo]**

```bash
./order_processing 8
```

**SAY while output scrolls:**
> "Watch the kitchen prepare orders and block when the queue is full. The delivery thread waits when the queue is empty. The monitor safely reads shared state under the mutex and prints orders prepared, orders delivered, and current queue size. No order is lost and no race conditions occur because every shared access is inside a critical section."

**[ON SCREEN: Pause on a MONITOR report and the Final Summary]**

---

### [5:00 – 6:30] Project 5 — TCP Client-Server System (~90 seconds)

**[ON SCREEN: Split view or switch between server terminal and client terminal]**

**SAY:**
> "Project 5 implements a university laboratory equipment booking system over TCP. The server accepts multiple simultaneous clients using a thread-per-client model, authenticates users against a registered list, and manages equipment reservations with mutex-protected shared state."

**[ON SCREEN: Start server in Tab 5]**

```bash
./server
```

**SAY:**
> "The server listens on port 8080 and displays connected users and equipment status."

**[ON SCREEN: Client Tab 6 — demonstrate full flow]**

**Step 1 — Failed authentication:**
- Run `./client`
- Enter: `baduser`
- **SAY:** "Invalid users receive AUTH_FAIL and cannot access equipment."

**Step 2 — Successful login and list:**
- Run `./client` again (or continue if your client allows retry)
- Enter: `student01`
- Choose menu option `1` (List equipment)
- **SAY:** "After authentication, the server sends the full equipment list with availability status."

**Step 3 — Successful reservation:**
- Choose option `2`
- Enter: `Microscope_A`
- **SAY:** "The server checks availability under the mutex, reserves the item, and sends RESERVED confirmation."

**Step 4 — Conflicting reservation (open a second client tab):**
- Start another `./client`, login as `student02`
- Try to reserve `Microscope_A` again
- **SAY:** "A second client receives UNAVAILABLE because the equipment is already reserved — preventing double booking."

**Step 5 — Graceful logout:**
- Choose option `3`
- **SAY:** "The client closes with: Session closed. Goodbye, USER_ID. The server releases the reservation and removes the user from the active list on disconnect."

**[ON SCREEN: Server log showing multiple clients and status display]**

**SAY:**
> "The communication protocol uses text commands — LOGIN, LIST, RESERVE, QUIT — with newline-terminated responses. Mutex locks protect equipment records and the active user list, and unexpected disconnections are handled without crashing the server."

---

### [6:30 – 7:00] Conclusion (~30 seconds)

**[ON SCREEN: Return to root README or show all five project folders]**

**SAY:**
> "In summary, these five projects demonstrate the full assessment scope: analyzing ELF binaries with readelf, objdump, strace, and GDB; low-level file processing in x86 assembly; high-performance Python integration via C extensions; thread-safe producer-consumer synchronization with pthreads; and a concurrent TCP client-server system with authentication and resource locking.
>
> All source code, build instructions, sample outputs, and analysis documentation are included in each project's README. Thank you for watching."

**[END RECORDING]**

---

## Timing Summary

| Section | Duration | Cumulative |
|---------|----------|------------|
| Introduction | 0:30 | 0:30 |
| Project 1 — ELF Analysis | 1:30 | 2:00 |
| Project 2 — Assembly | 0:45 | 2:45 |
| Project 3 — C Extension | 1:00 | 3:45 |
| Project 4 — Multithreading | 1:15 | 5:00 |
| Project 5 — TCP Client-Server | 1:30 | 6:30 |
| Conclusion | 0:30 | **7:00** |

---

## Rubric Alignment Checklist

Use this checklist while recording to ensure full coverage:

- [ ] **ELF:** architecture, entry point, sections (.text, .data, .bss, .plt, .got), dynamic linking, function reconstruction, branch/loop in assembly, strace categories, GDB breakpoints/stack/memory
- [ ] **Assembly:** file open/read, character traversal, line counting, LF/CRLF handling, error handling, correct output format
- [ ] **C Extension:** all 5 functions, Python C API, input validation, empty dataset handling, build with setup.py, test script with boundary case
- [ ] **Multithreading:** 3 threads, mutex + condition variables, queue capacity 5, timing (2s/4s/5s), monitor reports, no race conditions
- [ ] **TCP System:** multiple clients, authentication, equipment list, reservation/conflict, mutex protection, graceful disconnect message, server status display

---

## Tips for a Strong Recording

1. **Rehearse once** with a timer — Project 5 needs the most terminals; practice tab switching.
2. **Use a small order count** for Project 4 (`./order_processing 8`) so monitor output appears within ~15 seconds.
3. **Pre-build all binaries** so compilation does not eat into your speaking time.
4. **Speak clearly** over the terminal output — briefly pause when showing important results.
5. **Keep mouse movement minimal** — zoom terminal font to 14–16pt for readability.
