# ELF Binary Analysis Report

## Project 1: Investigating an ELF Executable

**Author:** [Your Name]
**Date:** [Submission Date]
**Course:** Linux Programming

---

## 1. Program Overview

### 1.1 Source Code Description

The program `program.c` implements a data analysis tool that:

1. Prompts the user for a number of data points
2. Dynamically allocates memory and generates random integers (0-999)
3. Analyzes the data to find minimum, maximum, sum, and average
4. Displays a formatted report with all results

### 1.2 Structural Requirements Compliance

| Requirement                       | Implementation                                                    | Location in Code            |
| --------------------------------- | ----------------------------------------------------------------- | --------------------------- |
| 3 user-defined functions + main() | `generate_data()`, `analyze_data()`, `display_report()`, `main()` | Lines 18-139                |
| Global variable                   | `g_total_numbers` at file scope                                   | Line 15                     |
| Loop                              | `for` loops in all functions                                      | Lines 29-31, 59-72, 100-110 |
| Decision-making                   | `if` for min/max, input validation                                | Lines 21-25, 63-71, 86-89   |
| Dynamic allocation                | `malloc(count * sizeof(int))`                                     | Line 20                     |
| Library function                  | `printf()`, `scanf()`, `rand()`, `srand()`, `time()`, `free()`    | Throughout                  |
| Meaningful output                 | Formatted report with headers, values, analysis                   | Lines 78-118                |

---

## 2. Compilation and Stripping

### 2.1 Compilation Command

```bash
gcc -Wall -O0 -fno-inline -o program program.c
```

**Flags explained:**

- `-Wall`: Enables all common compiler warnings
- `-O0`: No optimization (preserves code structure for analysis)
- `-fno-inline`: Prevents function inlining (functions remain as separate callable units)

**Result:** No warnings or errors. Produces a 16576-byte executable with full symbol tables.

### 2.2 Stripping Command

```bash
strip program
```

**Effect:** Removes all symbol table entries, debug information, and relocation information from the executable. Size reduced from 16576 bytes to 14472 bytes (12.7% reduction).

**Verification:**

- `nm program`: Outputs "no symbols" - confirms all local symbols removed
- `objdump -t program`: Outputs "SYMBOL TABLE: no symbols"
- `readelf -s program`: Only shows dynamic symbols (needed for dynamic linking with libc)

---

## 3. Static Analysis

### 3.1 ELF Header (`readelf -h program`)

```
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              DYN (Position-Independent Executable file)
  Machine:                           Advanced Micro Devices X86-64
  Entry point address:               0x11a0
```

**Key Observations:**

1. **Architecture: x86-64** - The executable targets 64-bit x86 processors (AMD64/Intel 64). This is confirmed by the `ELF64` class and `X86-64` machine type.

2. **Entry Point: 0x11a0** - This is the address of the `_start` function, not `main()`. The `_start` function is the true program entry point provided by the C runtime (CRT). It performs initialization tasks and eventually calls `main()`.

3. **Type: DYN (PIE)** - Position-Independent Executable. This means address space layout randomization (ASLR) can be used for security. The binary can be loaded at any base address, not a fixed one.

4. **Endianness: Little Endian** - Least significant byte stored first. Standard for x86-64 architecture.

### 3.2 Section Headers Analysis

#### 3.2.1 `.text` Section (Executable Code)

- **Offset:** 0x11a0
- **Size:** 0x5b2 (1458 bytes)
- **Purpose:** Contains all executable machine code instructions for the program
- **Content:** The compiled code for `main()`, `generate_data()`, `analyze_data()`, `display_report()`, plus C runtime startup code (`_start`, `_init`)

#### 3.2.2 `.data` Section (Initialized Global Data)

- **Offset:** 0x4000
- **Size:** 0x10 (16 bytes)
- **Purpose:** Stores initialized global variables
- **Content:** `g_total_numbers = 0` (4 bytes), plus C runtime internal data (12 bytes)

#### 3.2.3 `.bss` Section (Uninitialized Global Data)

- **Offset:** 0x4020
- **Size:** 0x10 (16 bytes)
- **Purpose:** Holds uninitialized global variables (zero-initialized at load time)
- **Content:** The `stderr` FILE pointer data and other runtime data
- **Note:** `.bss` occupies no space in the file (NOBITS type), only in memory

#### 3.2.4 `.plt` Section (Procedure Linkage Table)

- **Offset:** 0x1020
- **Size:** 0xc0 (192 bytes)
- **Purpose:** Provides stub functions for calling dynamically linked library functions
- **Each PLT entry (16 bytes):**
  1. Jump to address in GOT entry
  2. If unresolved, push index and jump to resolver

#### 3.2.5 `.got` Section (Global Offset Table)

- **Offset:** 0x3f68
- **Size:** 0x98 (152 bytes)
- **Purpose:** Contains pointers to global variables and functions resolved by the dynamic linker
- **Structure:**
  - `.got` (first 3 entries reserved for dynamic linker)
  - `.got.plt` (used by PLT for function resolution)
  - Entries populated by `ld.so` at load time or lazily on first call

#### 3.2.6 `.rodata` Section (Read-Only Data)

- **Offset:** 0x2000
- **Size:** 0x290 (656 bytes)
- **Purpose:** Stores read-only data

**String Constants Extracted:**

```
0x2008: "Memory allocation failed!\n"
0x2023: "Invalid data for analysis.\n"
0x2040: "\n========================================"
0x206a: "      DATA ANALYSIS REPORT"
0x2088: "========================================"
0x20b8: "Total numbers processed (global): %d\n"
0x20e0: "Array size analyzed:              %d\n"
0x2108: "----------------------------------------"
0x2131: "Generated Data: ["
0x2143: "%3d, "
0x2149: "%3d"
0x214d: "]"
0x214f: "Minimum value:  %d\n"
0x2163: "Maximum value:  %d\n"
0x2177: "Sum of values:  %d\n"
0x218b: "Average value:  %.2f\n"
0x21a8: "ELF Binary Investigation - Project 1"
0x21d0: "===================================="
0x21f8: "Enter the number of data points to generate: "
0x2226: "%d"
0x2230: "Invalid input. Using default value of 10.\n"
0x2260: "Memory successfully freed. Program terminating.\n"
```

#### 3.2.7 `.interp` Section (Interpreter)

- **Content:** `/lib64/ld-linux-x86-64.so.2`
- **Purpose:** Specifies the dynamic linker/loader to use

### 3.3 Dynamic Linking Analysis (`readelf -d program`, `readelf -l program`)

```
Dynamic section entries:
  NEEDED    libc.so.6        ← Only dependency
  INIT      0x1000           ← .init section
  FINI      0x1754           ← .fini section
  PLTGOT    0x3f68           ← GOT location
  FLAGS     BIND_NOW         ← Immediate binding
  FLAGS_1   NOW PIE          ← PIE enabled, no lazy binding
```

**Dynamic Symbol Table** imports from `libc.so.6`:
| Symbol | Version | Type |
|--------|---------|------|
| `free` | GLIBC_2.2.5 | Function |
| `__libc_start_main` | GLIBC_2.34 | Function |
| `puts` | GLIBC_2.2.5 | Function |
| `printf` | GLIBC_2.2.5 | Function |
| `srand` | GLIBC_2.2.5 | Function |
| `time` | GLIBC_2.2.5 | Function |
| `__isoc99_scanf` | GLIBC_2.7 | Function |
| `exit` | GLIBC_2.2.5 | Function |
| `fwrite` | GLIBC_2.2.5 | Function |
| `rand` | GLIBC_2.2.5 | Function |
| `__cxa_finalize` | GLIBC_2.2.5 | Function |
| `stderr` | GLIBC_2.2.5 | Object (FILE\*) |

**Conclusion: Dynamically Linked.** The executable depends on `libc.so.6` for all standard library functions. This reduces file size and allows shared library updates without recompilation.

### 3.4 Program Headers (Segments)

```
Segment 1: INTERP   → Dynamic linker path
Segment 2: LOAD R   → Read-only headers and string tables
Segment 3: LOAD R E → Executable code (.text, .plt, .init)
Segment 4: LOAD R   → Read-only data (.rodata, .eh_frame)
Segment 5: LOAD RW  → Read-write data (.data, .bss, .got)
Segment 11: GNU_STACK → Stack permissions (RW, no execute - NX enabled)
Segment 12: GNU_RELRO  → Makes GOT read-only after relocation
```

**Security Observations:**

- **NX (No-Execute)**: `GNU_STACK` has flags `RW` (no `X`), meaning the stack is not executable
- **RELRO (Partial)**: `GNU_RELRO` segment covers `.init_array`, `.fini_array`, `.dynamic`, and part of `.got`
- **PIE**: Position-Independent Executable enables ASLR

### 3.5 Function Reconstruction from Assembly

Using `objdump -d program` and analyzing the disassembly, I reconstructed each function:

#### Function 1: `main()` at address `0x163f`

**Assembly Flow:**

```
163f: endbr64              ; CET endbranch (security feature)
1643: push %rbp            ; Save old base pointer
1644: mov %rsp,%rbp        ; Set new stack frame
1647: sub $0x20,%rsp       ; Allocate 32 bytes on stack
164b: mov %fs:0x28,%rax    ; Load stack canary
1654: mov %rax,-0x8(%rbp)  ; Save canary to stack
1658: xor %eax,%eax        ; Clear eax

165a: lea 0xb47(%rip),%rax ; Load "ELF Binary Investigation..."
1661: mov %rax,%rdi        ; First argument for puts
1664: call puts            ; Print banner

1669: lea 0xb60(%rip),%rax ; Load "========================"
1670: mov %rax,%rdi
1673: call puts            ; Print separator

1678: mov $0x0,%edi        ; time(NULL)
167d: call time            ; Get current time
1682: mov %eax,%edi        ; Pass time as argument
1684: call srand           ; Seed random generator

1689: lea 0xb68(%rip),%rax ; Load "Enter number of data points..."
1690: mov %rax,%rdi
1693: mov $0x0,%eax        ; Variadic function (printf)
1698: call printf          ; Print prompt

169d: lea -0x20(%rbp),%rax ; &count (address on stack)
16a1: mov %rax,%rsi        ; Second argument for scanf
16a4: lea 0xb7b(%rip),%rax ; Load "%d" format string
16ab: mov %rax,%rdi
16ae: mov $0x0,%eax
16b3: call scanf           ; Read user input

16b8: mov -0x20(%rbp),%eax ; Load count
16bb: test %eax,%eax       ; Check if count <= 0
16bd: jg 16d5              ; If count > 0, skip default
16bf: lea 0xb6a(%rip),%rax ; Load "Invalid input..."
16c6: mov %rax,%rdi
16c9: call puts            ; Print error message
16ce: movl $0xa,-0x20(%rbp); Set count = 10

16d5: mov -0x20(%rbp),%eax ; Load count
16d8: mov %eax,%edi        ; Pass count to generate_data
16da: call 1289            ; Call generate_data(count)
16df: mov %rax,-0x10(%rbp) ; Save returned pointer (dataset)

16e3: mov -0x20(%rbp),%esi ; count (6th arg)
16e6: lea -0x14(%rbp),%rdi ; &sum (5th arg)
16ea: lea -0x18(%rbp),%rcx ; &max (4th arg)
16ee: lea -0x1c(%rbp),%rdx ; &min (3rd arg)
16f2: mov -0x10(%rbp),%rax ; Load dataset pointer
16f6: mov %rdi,%r8         ; &sum as r8
16f9: mov %rax,%rdi        ; data as 1st arg
16fc: call 1344            ; Call analyze_data(...)

1701: mov -0x14(%rbp),%edi ; sum
1704: mov -0x18(%rbp),%ecx ; max
1707: mov -0x1c(%rbp),%edx ; min
170a: mov -0x20(%rbp),%esi ; count
170d: mov -0x10(%rbp),%rax ; dataset
1711: mov %edi,%r8d         ; sum (6th arg)
1714: mov %rax,%rdi        ; data (1st arg)
1717: call 1474            ; Call display_report(...)

171c: mov -0x10(%rbp),%rax ; Load dataset pointer
1720: mov %rax,%rdi        ; Pass to free
1723: call free            ; Free allocated memory

1728: lea 0xb31(%rip),%rax ; Load "Memory successfully freed..."
172f: mov %rax,%rdi
1732: call puts            ; Print termination message
1737: mov $0x0,%eax        ; Return 0
173c: mov -0x8(%rbp),%rdx  ; Load stack canary
1740: sub %fs:0x28,%rdx    ; Compare with saved canary
1749: je 1750              ; If equal, skip stack check fail
174b: call __stack_chk_fail; Stack overflow detected!
1750: leave                ; Restore rsp and rbp
1751: ret                  ; Return to __libc_start_main
```

**C to Assembly Mapping:**
| C Code | Assembly Address |
|--------|-----------------|
| `printf("ELF Binary Investigation...")` | 0x165a-0x1664 |
| `puts("========================")` | 0x1669-0x1673 |
| `srand((unsigned)time(NULL))` | 0x1678-0x1684 |
| `printf("Enter number...")` | 0x1689-0x1698 |
| `scanf("%d", &count)` | 0x169d-0x16b3 |
| `if (count <= 0)` | 0x16b8-0x16bd |
| `count = 10` | 0x16ce |
| `generate_data(count)` → `dataset` | 0x16d5-0x16df |
| `analyze_data(dataset, count, &min, &max, &sum)` | 0x16e3-0x16fc |
| `display_report(dataset, count, min, max, sum)` | 0x1701-0x1717 |
| `free(dataset)` | 0x171c-0x1723 |
| `printf("Memory freed...")` | 0x1728-0x1732 |
| `return 0` | 0x1737-0x1751 |

#### Function 2: `generate_data()` at address `0x1289`

**Assembly Flow:**

```
1289: endbr64
128d: push %rbp
128e: mov %rsp,%rbp
1291: sub $0x20,%rsp         ; 32 bytes local storage
1295: mov %edi,-0x14(%rbp)   ; Save parameter 'count'

; Step 1: malloc(count * 4)
1298: mov -0x14(%rbp),%eax   ; Load count
129b: cltq                    ; Sign-extend to 64-bit (rax)
129d: shl $0x2,%rax           ; Multiply by 4 (shift left 2)
12a1: mov %rax,%rdi           ; Argument for malloc
12a4: call malloc             ; Allocate heap memory
12a9: mov %rax,-0x8(%rbp)     ; Save pointer 'data'

; Step 2: Check if malloc failed
12ad: cmpq $0x0,-0x8(%rbp)   ; Compare data with NULL
12b2: jne 12e1               ; If not NULL, jump to loop start

; Error handling: fprintf(stderr, "Memory allocation failed!")
12b4: mov 0x2d65(%rip),%rax  ; Load stderr FILE* pointer
12bb: mov %rax,%rcx          ; 4th arg: stderr
12be: mov $0x1a,%edx         ; 3rd arg: length 26
12c3: mov $0x1,%esi          ; 2nd arg: count 1
12c8: lea 0xd39(%rip),%rax   ; 1st arg: "Memory allocation failed!\n"
12cf: mov %rax,%rdi
12d2: call fwrite            ; Write error to stderr
12d7: mov $0x1,%edi          ; Exit code 1
12dc: call exit              ; Terminate program

; Step 3: Loop initialization (i = 0)
12e1: movl $0x0,-0xc(%rbp)   ; i = 0
12e8: jmp 132d               ; Jump to loop condition check

; Step 4: Loop body - data[i] = rand() % 1000
12ea: call rand              ; Get random number
12ef: mov -0xc(%rbp),%edx    ; Load i
12f2: movslq %edx,%rdx       ; Sign extend to 64-bit
12f5: lea 0x0(,%rdx,4),%rcx  ; rcx = i * 4
12fd: mov -0x8(%rbp),%rdx    ; Load data pointer
1301: lea (%rcx,%rdx,1),%rsi ; rsi = &data[i]

; Computation: rand() % 1000 (using multiply-and-shift trick)
1305: movslq %eax,%rdx       ; Sign extend rand() result
1308: imul $0x10624dd3,%rdx,%rdx ; Multiply by magic constant
130f: shr $0x20,%rdx         ; Take upper 32 bits
1313: sar $0x6,%edx          ; Shift right 6 (divide by 64)
1316: mov %eax,%ecx          ; Save original rand() value
1318: sar $0x1f,%ecx         ; Sign extend for signed division
131b: sub %ecx,%edx          ; Adjust for negative numbers
131d: imul $0x3e8,%edx,%ecx  ; Multiply quotient by 1000
1323: sub %ecx,%eax          ; Subtract = remainder (mod 1000)
1325: mov %eax,%edx          ; Result
1327: mov %edx,(%rsi)        ; data[i] = rand() % 1000

; Step 5: Increment i
1329: addl $0x1,-0xc(%rbp)   ; i++

; Step 6: Loop condition (i < count)
132d: mov -0xc(%rbp),%eax    ; Load i
1330: cmp -0x14(%rbp),%eax   ; Compare i with count
1333: jl 12ea                ; If i < count, continue loop

; Step 7: Store global variable and return
1335: mov -0x14(%rbp),%eax   ; Load count
1338: mov %eax,0x2cee(%rip)  ; g_total_numbers = count
133e: mov -0x8(%rbp),%rax    ; Return data pointer
1342: leave
1343: ret
```

**C to Assembly Mapping:**
| C Code | Assembly Address |
|--------|-----------------|
| `int *data = malloc(count * 4)` | 0x1298-0x12a9 |
| `if (data == NULL)` | 0x12ad-0x12b2 |
| `fprintf(stderr, "...failed!")` | 0x12b4-0x12d2 |
| `exit(1)` | 0x12dc |
| `for (int i = 0; ...)` | 0x12e1 (init), 0x1330-0x1333 (cond) |
| `i < count` | 0x1330-0x1333 |
| `data[i] = rand() % 1000` | 0x12ea-0x1327 |
| `i++` | 0x1329 |
| `g_total_numbers = count` | 0x1335-0x1338 |
| `return data` | 0x133e |

#### Function 3: `analyze_data()` at address `0x1344`

**Assembly Flow:**

```
1344: endbr64
1348: push %rbp
1349: mov %rsp,%rbp
134c: sub $0x40,%rsp         ; 64 bytes local storage
1350: mov %rdi,-0x18(%rbp)   ; data pointer (1st arg)
1354: mov %esi,-0x1c(%rbp)   ; count (2nd arg)
1357: mov %rdx,-0x28(%rbp)   ; min pointer (3rd arg)
135b: mov %rcx,-0x30(%rbp)   ; max pointer (4th arg)
135f: mov %r8,-0x38(%rbp)    ; sum pointer (5th arg)

; Step 1: if (data == NULL || count <= 0)
1363: cmpq $0x0,-0x18(%rbp)  ; data == NULL?
1368: je 1370                ; If yes, error
136a: cmpl $0x0,-0x1c(%rbp)  ; count <= 0?
136e: jg 1398                ; If count > 0, proceed

; Error: fprintf(stderr, "Invalid data for analysis.\n")
1370: mov 0x2ca9(%rip),%rax  ; stderr
1377: mov %rax,%rcx
137a: mov $0x1b,%edx          ; Length 27
137f: mov $0x1,%esi
1384: lea 0xc98(%rip),%rax   ; "Invalid data for analysis.\n"
138b: mov %rax,%rdi
138e: call fwrite
1393: jmp 1472               ; Return early

; Step 2: Initialize *min = *max = data[0], *sum = 0
1398: mov -0x18(%rbp),%rax   ; Load data pointer
139c: mov (%rax),%edx        ; data[0]
139e: mov -0x28(%rbp),%rax   ; min pointer
13a2: mov %edx,(%rax)        ; *min = data[0]

13a4: mov -0x18(%rbp),%rax
13a8: mov (%rax),%edx        ; data[0]
13aa: mov -0x30(%rbp),%rax   ; max pointer
13ae: mov %edx,(%rax)        ; *max = data[0]

13b0: mov -0x38(%rbp),%rax
13b4: movl $0x0,(%rax)       ; *sum = 0

; Step 3: Loop (i = 0)
13ba: movl $0x0,-0x4(%rbp)   ; i = 0
13c1: jmp 1466               ; Check condition

; Step 4: Loop body
13c6: mov -0x4(%rbp),%eax    ; i
13c9: cltq
13cb: lea 0x0(,%rax,4),%rdx  ; rdx = i * 4
13d3: mov -0x18(%rbp),%rax   ; data
13d7: add %rdx,%rax          ; &data[i]
13da: mov (%rax),%edx        ; data[i]
13dc: mov -0x28(%rbp),%rax   ; min pointer
13e0: mov (%rax),%eax        ; *min
13e2: cmp %eax,%edx          ; data[i] < *min?
13e4: jge 1402               ; If >=, skip min update

; *min = data[i]
13e6: mov -0x4(%rbp),%eax    ; i
13e9: cltq
13eb: lea 0x0(,%rax,4),%rdx
13f3: mov -0x18(%rbp),%rax
13f7: add %rdx,%rax          ; &data[i]
13fa: mov (%rax),%edx        ; data[i]
13fc: mov -0x28(%rbp),%rax
1400: mov %edx,(%rax)        ; *min = data[i]

; Max check: if (data[i] > *max)
1402: mov -0x4(%rbp),%eax    ; i
1405: cltq
1407: lea 0x0(,%rax,4),%rdx
140f: mov -0x18(%rbp),%rax
1413: add %rdx,%rax          ; &data[i]
1416: mov (%rax),%edx        ; data[i]
1418: mov -0x30(%rbp),%rax   ; max pointer
141c: mov (%rax),%eax        ; *max
141e: cmp %eax,%edx          ; data[i] > *max?
1420: jle 143e               ; If <=, skip max update

; *max = data[i]
1422: mov -0x4(%rbp),%eax
1425: cltq
1427: lea 0x0(,%rax,4),%rdx
142f: mov -0x18(%rbp),%rax
1433: add %rdx,%rax          ; &data[i]
1436: mov (%rax),%edx        ; data[i]
1438: mov -0x30(%rbp),%rax
143c: mov %edx,(%rax)        ; *max = data[i]

; *sum += data[i]
143e: mov -0x38(%rbp),%rax   ; sum pointer
1442: mov (%rax),%edx        ; current sum
1444: mov -0x4(%rbp),%eax    ; i
1447: cltq
1449: lea 0x0(,%rax,4),%rcx
1451: mov -0x18(%rbp),%rax
1455: add %rcx,%rax          ; &data[i]
1458: mov (%rax),%eax        ; data[i]
145a: add %eax,%edx          ; sum + data[i]
145c: mov -0x38(%rbp),%rax
1460: mov %edx,(%rax)        ; *sum += data[i]

; i++
1462: addl $0x1,-0x4(%rbp)

; Loop condition: i < count
1466: mov -0x4(%rbp),%eax    ; i
1469: cmp -0x1c(%rbp),%eax   ; Compare i with count
146c: jl 13c6                ; If i < count, continue loop

1472: leave
1473: ret
```

#### Function 4: `display_report()` at address `0x1474`

**Assembly Flow (Key parts):**

```
; Print header lines using puts
1491: lea 0xba8(%rip),%rax   ; "\n========================"
149b: call puts
14a0: lea 0xbc3(%rip),%rax   ; "      DATA ANALYSIS REPORT"
14aa: call puts
14af: lea 0xbd2(%rip),%rax   ; "========================"
14b9: call puts

; Print global variable
14be: mov 0x2b68(%rip),%eax  ; Load g_total_numbers from 0x402c
14c4: mov %eax,%esi          ; Second argument for printf
14c6: lea 0xbeb(%rip),%rax   ; "Total numbers processed (global): %d\n"
14cd: mov %rax,%rdi
14d5: call printf

; Print array size
14da: mov -0x1c(%rbp),%eax   ; count
14dd: mov %eax,%esi
14ee: call printf("Array size analyzed: %d\n")

; Print "["
1502: lea 0xc28(%rip),%rax   ; "Generated Data: ["
1511: call printf

; Loop printing data values
1516: movl $0x0,-0x4(%rbp)   ; i = 0
151d: jmp 1588               ; Check condition
151f: ... printf("%3d, ", data[i]) if i < count-1
1558: ... printf("%3d", data[i]) if i == count-1
1584: addl $0x1,-0x4(%rbp)   ; i++
1588: mov -0x4(%rbp),%eax    ; i
158b: cmp -0x1c(%rbp),%eax   ; Compare i with count
158e: jl 151f                ; Loop if i < count

; Print statistics
15c2: printf("Minimum value:  %d\n", min)
15db: printf("Maximum value:  %d\n", max)
15f4: printf("Sum of values:  %d\n", sum)

; Compute and print average (floating point)
15f9: pxor %xmm0,%xmm0       ; Clear xmm0
15fd: cvtsi2sdl -0x28(%rbp),%xmm0 ; Convert sum to double
1602: pxor %xmm1,%xmm1
1606: cvtsi2sdl -0x1c(%rbp),%xmm1 ; Convert count to double
160b: divsd %xmm1,%xmm0      ; xmm0 = sum / count
1628: printf("Average value:  %.2f\n", xmm0)
```

### 3.6 Conditional Branch Analysis

#### Example 1: Min Update Decision (`generate_data`)

**Assembly:**

```asm
13e2: cmp %eax,%edx     ; Compare data[i] with *min
13e4: jge 1402           ; Jump if data[i] >= *min (skip update)
```

**C Equivalent:**

```c
if (data[i] < *min) {   // If NOT (data[i] >= *min)
    *min = data[i];     // Update minimum
}
```

**Explanation:** The `jge` (Jump if Greater or Equal) instruction checks the flags set by `cmp`. If `data[i] >= *min`, the jump is taken, skipping the min update. This is the inverse of the C condition.

#### Example 2: Input Validation Decision (`main`)

**Assembly:**

```asm
16bb: test %eax,%eax    ; Check if count is zero/negative
16bd: jg 16d5           ; Jump if count > 0 (skip default)
16bf: ...                ; Set count = 10
```

**C Equivalent:**

```c
if (count <= 0) {       // If NOT (count > 0)
    count = 10;         // Use default
}
```

### 3.7 Loop Analysis

#### Example: Data Population Loop (`generate_data`)

**Assembly:**

```asm
; Initialization
12e1: movl $0x0,-0xc(%rbp)  ; i = 0
12e8: jmp 132d               ; Jump to condition check

; Body (at 0x12ea)
12ea: call rand              ; Random number
...                         ; Compute modulo 1000
1329: addl $0x1,-0xc(%rbp)   ; i++

; Condition check
132d: mov -0xc(%rbp),%eax   ; Load i
1330: cmp -0x14(%rbp),%eax  ; Compare i with count
1333: jl 12ea               ; If i < count, continue loop
```

**C Equivalent:**

```c
for (int i = 0; i < count; i++) {
    data[i] = rand() % 1000;
}
```

**Explanation:** This is a classic "jump-to-middle" loop pattern. The initialization sets `i = 0`, then immediately jumps to the condition check. If `i < count`, execution falls through to the body at 0x12ea. After the body, `i` is incremented and the condition is rechecked.

---

## 4. Dynamic Analysis

### 4.1 Full `strace` Output Classification

```
execve("./program", ["./program"], 0x7ffd...) = 0   ← PROGRAM START
brk(NULL)                               = 0x5594452f3000   ← HEAP INIT
mmap(NULL, 8192, PROT_READ|PROT_WRITE, ...) = 0x7f...   ← ANON MAP
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT   ← LINKER CHECK
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY) = 3   ← LINKER CACHE
fstat(3, {st_mode=S_IFREG|0644, ...})   = 0           ← FILE INFO
mmap(NULL, 20219, PROT_READ, ...)       = 0x7f...     ← MAP CACHE
close(3)                                = 0           ← CLOSE CACHE
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", ...) = 3  ← OPEN LIBC
read(3, "\177ELF\2\1\1\3...", 832)     = 832         ← READ ELF HEADER
mmap(NULL, 2170256, PROT_READ, ...)     = 0x7f...     ← MAP LIBC
mmap(..., PROT_READ|PROT_EXEC, ...)     = 0x7f...     ← MAP CODE (RX)
mmap(..., PROT_READ, ...)               = 0x7f...     ← MAP DATA (R)
mmap(..., PROT_READ|PROT_WRITE, ...)    = 0x7f...     ← MAP BSS (RW)
close(3)                                = 0           ← CLOSE LIBC
mmap(NULL, 12288, PROT_READ|PROT_WRITE, ...)=0x7f...  ← TLS MAP
arch_prctl(ARCH_SET_FS, 0x7f...)        = 0           ← SET TLS
set_tid_address(0x7f...)                = 44          ← THREAD ID
set_robust_list(0x7f..., 24)            = 0           ← FUTEX LIST
rseq(0x7f..., 0x20, 0, 0x53053053)      = 0           ← RSEQ INIT
mprotect(0x7f..., 16384, PROT_READ)     = 0           ← GOT RO
mprotect(..., 4096, PROT_READ)          = 0           ← RELRO
prlimit64(0, RLIMIT_STACK, ...)         = 0           ← GET STACK LIMIT
munmap(0x7f..., 20219)                  = 0           ← UNMAP CACHE
getrandom("\x2b\x7a...", 8, GRND_NONBLOCK)=8         ← RAND SEED
brk(NULL)                               = 0x5594452f3000
brk(0x559445314000)                     = 0x559445314000  ← HEAP EXPAND (malloc)
write(1, "ELF Binary Investigation..."...) = 37        ← printf/puts
fstat(0, ...)                           = 0           ← STDIN INFO
read(0, "5\n", 4096)                    = 2           ← scanf INPUT
write(1, "Enter the number of data points "...)=46    ← printf PROMPT
write(1, "=============================..."...) = 41   ← puts
write(1, "      DATA ANALYSIS REPORT\n", 27) = 27      ← puts
write(1, "=============================..."...) = 41   ← puts
write(1, "Total numbers processed ...\n", 36) = 36     ← printf
write(1, "Array size analyzed: ...\n", 36) = 36        ← printf
write(1, "----------------------------..."...) = 41    ← puts
write(1, "Generated Data: [ 24, 200,..."...) = 42      ← printf (loop)
write(1, "----------------------------..."...) = 41    ← puts
write(1, "Minimum value:  8\n", 18)     = 18           ← printf
write(1, "Maximum value:  844\n", 20)   = 20           ← printf
write(1, "Sum of values:  1122\n", 21)  = 21           ← printf
write(1, "Average value:  224.40\n", 23) = 23          ← printf
write(1, "=============================..."...) = 41   ← puts
write(1, "Memory successfully freed..."...) = 48        ← puts
lseek(0, -1, SEEK_CUR)                 = -1 ESPIPE    ← LIBRARY CLEANUP
exit_group(0)                           = ?            ← PROGRAM EXIT
+++ exited with 0 +++
```

### 4.2 System Call Categories

| Category                 | System Calls                                                                       | Count            |
| ------------------------ | ---------------------------------------------------------------------------------- | ---------------- |
| **Program Start-up**     | `execve`, `brk`, `mmap`, `access`                                                  | 4                |
| **Dynamic Linking**      | `openat`, `read`, `close`, `mmap`, `mprotect`, `arch_prctl`, `prlimit64`, `munmap` | 15+              |
| **Thread/Local Storage** | `set_tid_address`, `set_robust_list`, `rseq`, `arch_prctl`                         | 4                |
| **Memory Management**    | `brk` (heap expand), `mmap` (anonymous), `munmap`                                  | 3                |
| **Input**                | `read` (stdin), `fstat` (stdin)                                                    | 2                |
| **Output**               | `write` (stdout - 16 times), `fstat` (stdout)                                      | 17               |
| **Security**             | `mprotect` (RELRO), `getrandom`                                                    | 2                |
| **Program Termination**  | `exit_group`, `lseek`                                                              | 2                |
| **Total**                |                                                                                    | **~49 syscalls** |

### 4.3 Key Observations

1. **`malloc()` → `brk()`:** The `malloc(20)` call in `generate_data()` triggers `brk(NULL)` to get the current heap boundary, then `brk(new_address)` to extend the heap. For small allocations, the libc allocator uses `sbrk`/`brk` rather than `mmap`.

2. **Dynamic Linking Overhead:** More than half the system calls (25+) are for dynamic linking - loading `ld-linux.so.2` and `libc.so.6`, resolving symbols, setting up GOT entries, and applying RELRO protections.

3. **`write()` for Output:** Each `printf()` or `puts()` call translates to a `write()` system call with file descriptor 1 (stdout). The program makes 16 write calls for all output, including individual calls for each line and data element.

4. **Input via `read()`:** The `scanf("%d", &count)` call results in `read(0, buffer, 4096)` reading from file descriptor 0 (stdin).

5. **`free()` is NOT visible:** The `free(dataset)` call does NOT appear as a system call because libc's memory allocator caches freed blocks in a free-list for reuse, rather than immediately returning them to the OS.

6. **Stack Canary Check:** The `mprotect` calls during start-up are partially related to setting up RELRO (making GOT entries read-only after dynamic linking).

---

## 5. GDB Debugging Analysis

### 5.1 Breakpoint Setup

```gdb
(gdb) info functions
All functions matching regular expression "":
Non-debugging symbols:
0x0000000000001000  _init
0x00000000000010e0  __cxa_finalize@plt
0x00000000000010f0  free@plt
0x0000000000001100  puts@plt
0x0000000000001110  __stack_chk_fail@plt
0x0000000000001120  printf@plt
0x0000000000001130  srand@plt
0x0000000000001140  time@plt
0x0000000000001150  malloc@plt
0x0000000000001160  __isoc99_scanf@plt
0x0000000000001170  exit@plt
0x0000000000001180  fwrite@plt
0x0000000000001190  rand@plt
0x00000000000011a0  _start
0x00000000000011d0  deregister_tm_clones
0x0000000000001200  register_tm_clones
0x0000000000001240  __do_global_dtors_aux
0x0000000000001280  frame_dummy
0x0000000000001289  generate_data         ← Function 1
0x0000000000001344  analyze_data          ← Function 2
0x0000000000001474  display_report        ← Function 3
0x000000000000163f  main                  ← Entry (after _start)

(gdb) break *0x11a0      # _start (program entry)
(gdb) break *0x163f      # main
(gdb) break *0x1289      # generate_data
(gdb) break *0x1344      # analyze_data
(gdb) break *0x1474      # display_report
```

### 5.2 Call Stack at Each Breakpoint

**At `_start` (0x11a0):**

```
#0  _start () at ...
No other frames (program just started)
```

**At `main` (0x163f):**

```
#0  main () at program.c:80
Backtrace shows _start → __libc_start_main → main
```

**At `generate_data` (0x1289):**

```
#0  generate_data (count=5) at program.c:22
#1  0x00005555555556df in main () at program.c:109
```

**At `analyze_data` (0x1344):**

```
#0  analyze_data (data=0x5555555592a0, count=5, min=0x7fffffffde5c,
                  max=0x7fffffffde58, sum=0x7fffffffde54) at program.c:39
#1  0x0000555555555701 in main () at program.c:112
```

**At `display_report` (0x1474):**

```
#0  display_report (data=0x5555555592a0, count=5, min=8, max=844, sum=1122)
    at program.c:82
#1  0x000055555555571c in main () at program.c:115
```

### 5.3 Memory Inspection

**Global Variable (`g_total_numbers`):**

```gdb
(gdb) p &g_total_numbers
$1 = (int *) 0x55555555802c <g_total_numbers>
(gdb) p g_total_numbers
$2 = 5
(gdb) x/d 0x55555555802c
0x55555555802c <g_total_numbers>:       5
(gdb) x/4xb 0x55555555802c
0x55555555802c: 0x05    0x00    0x00    0x00
```

**Local Variable on Stack (`count`):**

```gdb
(gdb) p &count
$3 = (int *) 0x7fffffffde60
(gdb) p count
$4 = 5
(gdb) info frame
Stack level 0, frame at 0x7fffffffde70:
 rip = 0x5555555556b3 in main; saved rip = 0x7ffff7c29e0a
 Arglist at 0x7fffffffde60, args:
 Locals at 0x7fffffffde60, Previous frame's sp is 0x7fffffffde70
 Saved registers:
  rbp at 0x7fffffffde60, rip at 0x7fffffffde68
```

**Dynamically Allocated Memory (Heap):**

```gdb
(gdb) p dataset
$5 = (int *) 0x5555555592a0
(gdb) x/5wd 0x5555555592a0
0x5555555592a0: 24      200     46      844
0x5555555592b0: 8
(gdb) x/10xw 0x5555555592a0
0x5555555592a0: 0x00000018    0x000000c8    0x0000002e    0x0000034c
0x5555555592b0: 0x00000008    0x00000000    0x00000000    0x00000000
0x5555555592c0: 0x00000000    0x00000000
```

### 5.4 Stack, Heap, and Global Memory Analysis

| Memory Region | Address          | Characteristics                                                                                                                                                                  |
| ------------- | ---------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Stack**     | `0x7fffffffde60` | High addresses, grows downward. Contains `count`, `min`, `max`, `sum`, `dataset` pointer, return addresses. Memory automatically managed via RSP/RBP. Lifetime = function scope. |
| **Heap**      | `0x5555555592a0` | Between .bss and stack, grows upward. Contains the 5-element integer array. Memory manually allocated/freed.                                                                     |
| **Global**    | `0x55555555802c` | In `.data` section (low address). Contains `g_total_numbers`. Lifetime = entire program execution.                                                                               |

**Key Differences:**

1. **Location**: Stack at high addresses (~0x7fff...), heap at mid addresses (~0x5555...), globals at low addresses (~0x5555...)
2. **Management**: Stack = automatic (push/pop), Heap = manual (malloc/free), Global = static
3. **Lifetime**: Stack = function scope, Heap = until free(), Global = entire program
4. **Speed**: Stack = fastest (just SP adjustment), Heap = slower (allocator overhead), Global = fixed address
5. **Size Limits**: Stack = ~8MB (ulimit), Heap = virtual memory limit, Global = determined at link time
6. **Growth Direction**: Stack = downward, Heap = upward (toward each other)

---

## 6. Security Features Analysis

### 6.1 PIE (Position-Independent Executable)

- **Status:** ✅ Enabled
- **Evidence:** ELF type is "DYN" (PIE), dynamic entry `FLAGS_1 = NOW PIE`
- **Effect:** The program can be loaded at any random base address (ASLR). Each time the program runs, addresses like `0x55555555802c` for `g_total_numbers` will be different.

### 6.2 NX (No-Execute)

- **Status:** ✅ Enabled
- **Evidence:** Program header `GNU_STACK` has flags `RW` (no `X`/execute permission)
- **Effect:** Stack and data pages are not executable, preventing shellcode injection attacks via buffer overflows.

### 6.3 RELRO (Relocation Read-Only)

- **Status:** ⚠️ Partial RELRO
- **Evidence:** `GNU_RELRO` segment covers `.init_array`, `.fini_array`, `.dynamic`, and part of `.got`. `FLAGS = BIND_NOW` indicates immediate binding.
- **Effect:** GOT entries are resolved at load time (not lazily), but some GOT entries remain writable. Full RELRO would require `-z relro -z now` linker flags to make entire GOT read-only.

### 6.4 Stack Canary

- **Status:** ✅ Present
- **Evidence:**
  - `__stack_chk_fail@plt` in PLT
  - In `main()`: `mov %fs:0x28,%rax` (load canary) at 0x164b and `sub %fs:0x28,%rdx` (check canary) at 0x1740
- **Effect:** Prevents simple stack buffer overflow attacks by detecting corruption of return addresses.

### 6.5 Symbol Stripping

- **Status:** ✅ Applied
- **Evidence:** `nm program` returns "no symbols", `objdump -t program` returns "no symbols"
- **Effect:** After `strip`, only dynamic symbols required for linking remain. Local function names (`main`, `generate_data`, etc.) are removed, making reverse engineering more difficult.

---

## 7. Summary of Findings

1. **Architecture**: x86-64, little endian, dynamically linked PIE executable
2. **Entry Point**: `0x11a0` (\_start function), not `main()` at `0x163f`
3. **Dependencies**: Only `libc.so.6` (GNU C Library)
4. **Sections Identified**: 29 sections total, with key sections `.text` (code), `.data` (globals), `.bss` (uninitialized), `.plt`/`.got` (dynamic linking), `.rodata` (constants)
5. **Functions Reconstructed**: All 4 functions (`main`, `generate_data`, `analyze_data`, `display_report`) identified and mapped from assembly to C
6. **System Calls**: ~49 syscalls across 8 categories, dominated by dynamic linking overhead
7. **Memory**: Stack (local variables), heap (malloc'd array), global (g_total_numbers) all identified and inspected
8. **Security**: PIE, NX, RELRO, and stack canaries all verified
9. **Stripping**: Reduced binary size by ~12.7%, removed all local symbols
