# Demo Video Script: Project 1 - ELF Binary Investigation

**Duration:** ~10-15 minutes
**Format:** Screen recording with voiceover

---

## Section 1: Introduction (1 minute)

**[SCREEN: Show terminal with WSL/Ubuntu]**

**Script:**
"Hello, my name is [Your Name]. Today I'll be presenting Project 1: Investigating an ELF Executable. I'll cover the C program I developed, demonstrate compilation and stripping, perform static analysis using readelf and objdump, dynamic analysis using strace, and finally debugging with gdb. Let's begin."

---

## Section 2: Source Code Walkthrough (2 minutes)

**[SCREEN: Open program.c in VSCode/editor - highlight parts as you talk]**

**Script:**
"Let me walk through the source code of `program.c`. This program generates random numbers, analyzes them, and displays a report."

**Navigate to:**

1. **Global variable** (line 15):
   - "Here's our global variable `int g_total_numbers = 0`. This tracks how many data points were processed. It's stored in the `.data` section because it's initialized to zero."

2. **Function 1 - `generate_data()`** (lines 18-35):
   - "This is our first user-defined function. It does three things:"
   - "First, it dynamically allocates memory using `malloc()` - [highlight malloc(line 20)]"
   - "Then it uses a `for` loop to fill the array with random numbers using `rand()` [highlight loop lines 29-31]"
   - "Finally, it sets the global variable and returns the pointer."

3. **Function 2 - `analyze_data()`** (lines 38-74):
   - "The second function finds the minimum, maximum, and sum. Note the `if` statements for decision-making [highlight lines 63 and 71] - these are the conditional branches we'll see in the assembly."

4. **Function 3 - `display_report()`** (lines 77-118):
   - "The third function prints a formatted report. It uses `printf()` from the standard library [highlight line 92], and another `for` loop to print all the values [highlight loop lines 99-111]."

5. **`main()`** (lines 121-145):
   - "In main, we seed the random generator with `srand(time(NULL))`, read user input with `scanf`, validate with an `if` statement [highlight line 135], call all three functions, and finally `free()` the allocated memory."

---

## Section 3: Compilation and Stripping (1 minute)

**[SCREEN: Terminal - type commands with visible output]**

**Script:**
"Now let's compile the program."

**Type:**

```bash
gcc -Wall -O0 -fno-inline -o program program.c
```

"Notice:"

- `-Wall` enables all warnings - no warnings, clean compile
- `-O0` means no optimization - preserves our function structure
- `-fno-inline` keeps functions separate for analysis

**Check file:**

```bash
ls -la program
# Shows 16576 bytes
file program
# Shows: ELF 64-bit LSB pie executable, x86-64
```

**Now strip:**

```bash
strip program
ls -la program
# Shows 14472 bytes (12.7% smaller!)
```

"Now we have a stripped executable. Let's verify:"

```bash
nm program
# Shows: no symbols
```

"This confirms all local symbols like main, generate_data, etc. have been removed."

---

## Section 4: Static Analysis with readelf (3 minutes)

**[SCREEN: Terminal - run commands and explain output]**

**Script:**
"Let's analyze the ELF header using `readelf`."

```bash
readelf -h program
```

**[Point to each field as you explain]**

1. **Class: ELF64** - "This is a 64-bit executable targeting x86-64 architecture."
2. **Entry point: 0x11a0** - "This is NOT main(). This is `_start`, the true entry point provided by the C runtime. It initializes the runtime and eventually calls main()."
3. **Type: DYN (PIE)** - "Position-Independent Executable. This enables ASLR for security - the program can be loaded at any random memory address."

**Now sections:**

```bash
readelf -S program
```

**[Walk through key sections]**

1. **`.text` at offset 0x11a0** - "This contains all the executable code. Size 0x5b2 = 1458 bytes. This is where main(), generate_data(), analyze_data(), display_report() live."
2. **`.data` at offset 0x4000** - "Initialized global data. Our `g_total_numbers = 0` is stored here - that's 4 bytes of the 16-byte section."
3. **`.bss` at offset 0x4020** - "Uninitialized data. Note it says NOBITS - it takes no space in the file, only in memory. Zero-initialized at load time."
4. **`.plt` at offset 0x1020** - "The Procedure Linkage Table. These are stub functions for calling dynamically linked library functions like printf, malloc, etc."
5. **`.got` at offset 0x3f68** - "The Global Offset Table. Contains addresses of global symbols resolved by the dynamic linker at runtime."

**Dynamic linking:**

```bash
readelf -l program
readelf -d program
```

"Let's check if it's statically or dynamically linked..."

```bash
readelf -d program | head -5
# Shows: NEEDED libc.so.6
```

"It's dynamically linked - depends only on `libc.so.6`. The INTERP segment points to `/lib64/ld-linux-x86-64.so.2`, the dynamic linker."

---

## Section 5: Function Reconstruction with objdump (4 minutes)

**[SCREEN: Terminal - use objdump and explain]**

**Script:**
"Now let's reconstruct our C functions from the assembly code."

```bash
objdump -d program
```

**Main function (0x163f):**
"I'll start with `main` at address 0x163f. Let me walk through the key parts..."

```asm
; [Point to these lines as you explain]
165a: lea 0xb47(%rip),%rax    ; Load string address
1661: mov %rax,%rdi           ; String as first argument
1664: call 1100 <puts@plt>    ; Call puts() via PLT
```

"This is our first `puts()` call printing the banner. The program uses the PLT to call puts, which then jumps through the GOT to find the actual function in libc."

```asm
169d: lea -0x20(%rbp),%rax    ; &count (stack address)
16a1: mov %rax,%rsi           ; Second argument for scanf
16a4: lea 0xb7b(%rip),%rax   ; "%d" format string
16ae: mov $0x0,%eax           ; For variadic function
16b3: call 1160 <__isoc99_scanf@plt>
```

"This is the scanf call reading user input. Note how it passes `&count` as the second argument - the address is on the stack at `rbp-0x20`."

```asm
; Conditional branch for input validation
16bb: test %eax,%eax          ; Check count
16bd: jg 16d5                ; Jump if count > 0
16bf: lea 0xb6a(%rip),%rax   ; "Invalid input" string
16c9: call 1100 <puts@plt>   ; Print message
16ce: movl $0xa,-0x20(%rbp)  ; count = 10
```

"This is our `if (count <= 0)` statement. The `test` instruction checks if count is zero or negative. `jg` jumps to skip the error handling if count > 0. Notice the inverse logic - the jump condition is the opposite of the C condition."

```asm
16da: call 1289              ; Call generate_data
16df: mov %rax,-0x10(%rbp)   ; Save pointer
16fc: call 1344              ; Call analyze_data
1717: call 1474              ; Call display_report
1723: call 10f0 <free@plt>   ; Free memory
```

"Here we see the calls to our three user-defined functions followed by free(). The return value from generate_data is saved to the stack."

**Generate_data (0x1289):**

```asm
; malloc(count * 4) - multiply by 4 for int size
1298: mov -0x14(%rbp),%eax   ; count
129d: shl $0x2,%rax          ; count * 4
12a4: call 1150 <malloc@plt>

; Check if malloc returned NULL
12ad: cmpq $0x0,-0x8(%rbp)
12b2: jne 12e1              ; If not NULL, continue
```

"The malloc call: shift left by 2 multiplies count by 4 (sizeof int). Then we check if the return value is NULL. If it IS NULL, we jump to error handling. Otherwise, we proceed to the loop."

**[Show the loop structure]**

```asm
; Loop initialization
12e1: movl $0x0,-0xc(%rbp)   ; i = 0
12e8: jmp 132d               ; Jump to condition

; Loop body - data[i] = rand() % 1000
12ea: call 1190 <rand@plt>   ; Get random number
... [mod 1000 computation] ...
1329: addl $0x1,-0xc(%rbp)   ; i++

; Condition check - jump to body if i < count
132d: mov -0xc(%rbp),%eax
1330: cmp -0x14(%rbp),%eax
1333: jl 12ea                ; Loop if i < count
```

"This is a classic 'jump-to-middle' loop pattern. Initialize i=0, jump to condition check. The body computes `rand() % 1000`, stores it, increments i, then the condition checks `if i < count` to continue."

**Analyze_data (0x1344) - Conditional branches:**

```asm
; data[i] < *min check
13e2: cmp %eax,%edx          ; data[i] vs *min
13e4: jge 1402               ; Skip update if data[i] >= *min

; data[i] > *max check
141e: cmp %eax,%edx          ; data[i] vs *max
1420: jle 143e               ; Skip update if data[i] <= *max
```

"Notice the pattern: both conditions use the INVERSE jump instruction. In C, we check `if (data[i] < *min)` but the assembly checks `if NOT (data[i] >= *min)` and jumps over the update. This is standard compiler optimization - it makes straight-line code the common path."

---

## Section 6: Dynamic Analysis with strace (2 minutes)

**[SCREEN: Run strace and analyze output]**

**Script:**
"Let's see what system calls the program makes during execution."

```bash
echo "5" | strace -o strace_output.txt ./program
cat strace_output.txt
```

**[Scroll through and categorize]**

**Program Start-up & Dynamic Linking:**
"Let me highlight the key system calls..."

```bash
execve("./program", ...)     # Launch the program
openat(..., "libc.so.6")     # Open shared library
mmap(..., PROT_READ|PROT_EXEC) # Map executable code from libc
```

"These 20+ syscalls are ALL for dynamic linking. The kernel's loader reads the ELF header, finds the interpreter ld-linux.so.2, which loads libc.so.6, resolves all symbols, and sets up the GOT."

**Memory Allocation:**

```bash
brk(NULL)                    # Get current heap position
brk(0x559445314000)          # Expand heap for malloc
```

"When our program calls malloc, the C library's allocator uses the `brk` system call to expand the heap. The program break moves upward to reserve memory."

**Input/Output:**

```bash
read(0, "5\n", 4096)         # Read user input (scanf)
write(1, "Total numbers...", 36)  # Write output (printf)
```

"Every printf/puts call corresponds to a `write` syscall to file descriptor 1 (stdout). The scanf results in a `read` from file descriptor 0 (stdin). We see 16 write calls for all our output."

**Important observation about free():**
"Notice there is NO syscall for `free()`. The C library's memory allocator caches freed blocks internally rather than returning memory to the OS immediately, so free() doesn't trigger a syscall."

**Program Termination:**

```bash
exit_group(0)                # Exit with status 0
```

---

## Section 7: Debugging with GDB (2 minutes)

**[SCREEN: GDB session]**

**Script:**
"Finally, let me demonstrate debugging with GDB."

```bash
gdb ./program
```

**Setting breakpoints:**

```gdb
(gdb) break *0x11a0          # _start (entry point)
(gdb) break *0x163f          # main
(gdb) break *0x1289          # generate_data
(gdb) break *0x1344          # analyze_data
(gdb) break *0x1474          # display_report
```

**Run and inspect:**

```gdb
(gdb) run
Starting program: /root/Cprogramming/.../program
```

**[Run through breakpoints]**

**At generate_data breakpoint:**

```gdb
(gdb) continue
(gdb) bt                     # Backtrace
#0  generate_data (count=5) at 0x1289
#1  main () at 0x16df

(gdb) info registers         # Show all registers
(gdb) p $rdi                 # First argument = count = 5
```

**Memory inspection:**

```gdb
# Global variable
(gdb) p g_total_numbers
$1 = 0
(gdb) p &g_total_numbers
$2 = (int *) 0x55555555802c

# After malloc - inspect heap
(gdb) x/5wd 0x5555555592a0
0x5555555592a0: 24      200     46      844
0x5555555592b0: 8

# Local variables on stack
(gdb) p &count
$3 = (int *) 0x7fffffffde60
(gdb) p &i
$4 = (int *) 0x7fffffffde5c
```

**Explain memory regions:**
"Notice the three memory regions:"

1. **Stack** at `0x7fffffff...` - "High addresses. Our local variables count, i, min, max, sum live here."
2. **Heap** at `0x5555555592a0` - "Dynamic memory from malloc. The 5 integers are stored here."
3. **Global** at `0x55555555802c` - "Our global variable g_total_numbers is at a lower fixed address."

**Stack canary verification:**

```gdb
(gdb) disassemble main
```

"At address 0x164b, we see `mov %fs:0x28,%rax` - this loads the stack canary from thread-local storage. At 0x1740, we see `sub %fs:0x28,%rdx` - this checks if the canary was corrupted, protecting against buffer overflow attacks."

---

## Section 8: Conclusion (30 seconds)

**[SCREEN: Show all files in the project directory]**

**Script:**
"To summarize what I've demonstrated today:"

1. "A C program satisfying all structural requirements - 3 functions, global variable, loops, decisions, dynamic memory, and library calls."
2. "Compilation and stripping - reducing the binary by 12.7%."
3. "Static analysis with readelf and objdump - identified the ELF header, all sections, dynamic linking, and reconstructed all 4 functions from assembly."
4. "Dynamic analysis with strace - classified ~49 system calls across startup, linking, memory, I/O, and termination."
5. "Debugging with GDB - set breakpoints at entry point, main, and user functions; inspected the call stack, global variables, heap memory, and stack variables, and explained the three memory regions."

"Thank you for watching. All files including the source code, stripped executable, analysis report, and strace output are available in the project directory."

**[END]**
