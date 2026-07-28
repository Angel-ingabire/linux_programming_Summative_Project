; =============================================================================
; Project 2: Assembly-Based Text File Analysis
; =============================================================================
; Program: sensor_analysis.asm
; Description: Reads a text file (sensor_readings.txt) containing sensor
;              measurements (one per line), traverses character by character,
;              and counts:
;                 - Total number of lines (total records)
;                 - Number of non-empty lines (valid records)
;
; Handles both Unix (LF - \n) and Windows (CRLF - \r\n) line endings.
; Includes error handling for file open/read failures.
;
; Build Instructions (Linux/NASM):
;   nasm -f elf32 -o sensor_analysis.o sensor_analysis.asm
;   ld -m elf_i386 -o sensor_analysis sensor_analysis.o
;   ./sensor_analysis
;
; For 64-bit build:
;   nasm -f elf64 -o sensor_analysis.o sensor_analysis.asm
;   ld -o sensor_analysis sensor_analysis.o
;   ./sensor_analysis
; =============================================================================

section .data
    ; ----- File and Program Information -----
    filename db "sensor_readings.txt", 0   ; Input file name (null-terminated)
    
    ; ----- File Access Mode (O_RDONLY = 0) -----
    ; For sys_open, the second argument (flags) = 0 means read-only
    mode dd 0
    
    ; ----- Output Format Strings -----
    total_msg   db "Total records: ", 0     ; Message prefix for total count
    valid_msg   db "Valid records: ", 0     ; Message prefix for valid count
    newline     db 10, 0                    ; Newline character (LF)
    
    ; ----- Error Messages -----
    err_open    db "Error: Unable to open file 'sensor_readings.txt'", 10, 0
    err_read    db "Error: Failed to read file contents", 10, 0
    err_empty   db "Error: File is empty", 10, 0

section .bss
    ; ----- Buffers and Storage -----
    file_buffer resb 4096                  ; 4KB buffer for file content
    bytes_read  resd 1                     ; Number of bytes actually read from file
    file_descriptor resd 1                 ; File descriptor returned by sys_open
    
    ; ----- Counters -----
    total_lines   resd 1                   ; Counter for total lines in file
    valid_lines   resd 1                   ; Counter for non-empty lines
    current_line_empty resd 1              ; Flag: 1 if current line has no data, 0 otherwise
    i             resd 1                   ; Loop index for traversing buffer

section .text
    global _start                          ; Entry point for the program

; =============================================================================
; MAIN PROGRAM ENTRY POINT
; =============================================================================
_start:
    ; ---------------------------------------------------------------
    ; Step 1: Open the file for reading
    ; ---------------------------------------------------------------
    ; sys_open syscall (int 0x80):
    ;   eax = 5  (sys_open)
    ;   ebx = pointer to filename string
    ;   ecx = flags (0 = O_RDONLY)
    ;   Returns: file descriptor in eax (or negative error code)
    ; ---------------------------------------------------------------
    mov eax, 5                              ; sys_open syscall number
    mov ebx, filename                       ; Pointer to filename
    mov ecx, [mode]                         ; O_RDONLY (0)
    int 0x80                                ; Invoke kernel
    
    ; Check if file opened successfully (return value >= 0)
    test eax, eax                           ; Check if eax is negative (error)
    js .open_error                          ; Jump if sign flag set (error occurred)
    
    mov [file_descriptor], eax              ; Store file descriptor
    
    ; ---------------------------------------------------------------
    ; Step 2: Read file contents into buffer
    ; ---------------------------------------------------------------
    ; sys_read syscall (int 0x80):
    ;   eax = 3  (sys_read)
    ;   ebx = file descriptor
    ;   ecx = pointer to buffer
    ;   edx = buffer size
    ;   Returns: number of bytes read in eax
    ; ---------------------------------------------------------------
    mov eax, 3                              ; sys_read syscall number
    mov ebx, [file_descriptor]              ; File descriptor from open
    mov ecx, file_buffer                    ; Destination buffer
    mov edx, 4096                           ; Buffer size (4KB)
    int 0x80                                ; Invoke kernel
    
    ; Check if read succeeded (return value >= 0)
    test eax, eax                           ; Check for error
    js .read_error                          ; Jump if sign flag set (error)
    
    ; Check if file is empty (0 bytes read)
    cmp eax, 0
    je .empty_error
    
    mov [bytes_read], eax                   ; Store number of bytes read
    
    ; ---------------------------------------------------------------
    ; Step 3: Close the file (no longer needed)
    ; ---------------------------------------------------------------
    ; sys_close syscall (int 0x80):
    ;   eax = 6  (sys_close)
    ;   ebx = file descriptor
    ; ---------------------------------------------------------------
    mov eax, 6                              ; sys_close syscall number
    mov ebx, [file_descriptor]              ; File descriptor
    int 0x80                                ; Invoke kernel
    
    ; ---------------------------------------------------------------
    ; Step 4: Process/analyze the file data
    ; ---------------------------------------------------------------
    call count_records                      ; Call subroutine to count lines
    
    ; ---------------------------------------------------------------
    ; Step 5: Display results
    ; ---------------------------------------------------------------
    call print_results                      ; Call subroutine to print results
    
    ; ---------------------------------------------------------------
    ; Step 6: Exit program
    ; ---------------------------------------------------------------
    ; sys_exit syscall (int 0x80):
    ;   eax = 1  (sys_exit)
    ;   ebx = exit code (0 = success)
    ; ---------------------------------------------------------------
    mov eax, 1                              ; sys_exit syscall number
    xor ebx, ebx                            ; Exit code 0 (success)
    int 0x80                                ; Invoke kernel

; =============================================================================
; ERROR HANDLING SECTION
; =============================================================================
; These routines handle various error conditions and display appropriate
; error messages before terminating the program with a non-zero exit code.
; =============================================================================

; ----- File Open Error Handler -----
.open_error:
    ; Display error message using sys_write
    mov eax, 4                              ; sys_write syscall number
    mov ebx, 1                              ; File descriptor: stdout (1)
    mov ecx, err_open                       ; Pointer to error message
    mov edx, 52                             ; Message length
    int 0x80                                ; Invoke kernel
    mov ebx, 1                              ; Exit code 1 (error)
    jmp .exit_with_code                     ; Jump to exit

; ----- File Read Error Handler -----
.read_error:
    mov eax, 4                              ; sys_write syscall number
    mov ebx, 1                              ; stdout
    mov ecx, err_read                       ; Pointer to error message
    mov edx, 36                             ; Message length
    int 0x80                                ; Invoke kernel
    mov ebx, 2                              ; Exit code 2 (read error)
    jmp .close_and_exit                     ; Try to close file before exit

; ----- Empty File Error Handler -----
.empty_error:
    mov eax, 4                              ; sys_write syscall number
    mov ebx, 1                              ; stdout
    mov ecx, err_empty                      ; Pointer to error message
    mov edx, 24                             ; Message length
    int 0x80                                ; Invoke kernel
    mov ebx, 3                              ; Exit code 3 (empty file)
    ; Attempt to close the file descriptor
    mov eax, 6                              ; sys_close
    mov ebx, [file_descriptor]
    int 0x80
    jmp .exit_with_code

; ----- Close file on read error and exit -----
.close_and_exit:
    mov eax, 6                              ; sys_close
    mov ebx, [file_descriptor]
    int 0x80

; ----- Exit with specific error code -----
.exit_with_code:
    mov eax, 1                              ; sys_exit
    int 0x80

; =============================================================================
; SUBROUTINE: count_records
; =============================================================================
; This subroutine traverses the file buffer character by character to count:
;   1. total_lines  - Incremented for every line terminator encountered
;   2. valid_lines  - Incremented only when a line contains at least one
;                     non-newline/non-carriage-return character
;
; Line Ending Detection Logic:
;   - Unix (LF):    \n (ASCII 10) - single character line terminator
;   - Windows (CRLF): \r\n (ASCII 13, 10) - two-character line terminator
;
; Algorithm:
;   1. Initialize i = 0 (buffer index)
;   2. Set current_line_empty = 1 (assume line is empty initially)
;   3. For each character in the buffer:
;      a. If character == '\r' (CR), skip it (it's part of CRLF)
;      b. If character == '\n' (LF):
;            - Increment total_lines
;            - If current_line_empty == 0, increment valid_lines
;            - Reset current_line_empty = 1 for next line
;         Else (regular data character):
;            - Set current_line_empty = 0 (line has data)
;   4. Handle the last line if the file doesn't end with a newline
; =============================================================================
count_records:
    push ebp                               ; Save base pointer
    mov ebp, esp                           ; Set up stack frame
    
    ; Initialize counters to zero
    mov dword [total_lines], 0
    mov dword [valid_lines], 0
    mov dword [i], 0
    
    ; Initialize: assume current line is empty until proven otherwise
    mov dword [current_line_empty], 1
    
.loop_start:
    ; Check if we've processed all bytes in the buffer
    mov eax, [i]                           ; Load current index
    cmp eax, [bytes_read]                  ; Compare with total bytes read
    jge .loop_end                          ; If index >= bytes_read, we're done
    
    ; Load the current character from buffer
    mov eax, [i]                           ; Get current index
    xor ecx, ecx                           ; Clear ecx
    mov cl, [file_buffer + eax]            ; Load character at buffer[i]
    
    ; ---------------------------------------------------------------
    ; Character Analysis:
    ; We check for three cases:
    ; 1. Carriage Return (\r = 0x0D) - part of Windows CRLF
    ; 2. Line Feed (\n = 0x0A) - line terminator (Unix or Windows)
    ; 3. Regular character - indicates non-empty line
    ; ---------------------------------------------------------------
    
    ; Check if character is Carriage Return (\r)
    cmp cl, 0x0D                           ; Compare with ASCII CR
    je .handle_cr                          ; If CR, handle specially
    
    ; Check if character is Line Feed (\n)
    cmp cl, 0x0A                           ; Compare with ASCII LF
    je .handle_lf                          ; If LF, count as line end
    
    ; If we reach here, it's a regular data character.
    ; This means the current line contains data, so it's not empty.
    mov dword [current_line_empty], 0      ; Mark line as non-empty
    
    ; Move to next character
    inc dword [i]                          ; Increment index
    jmp .loop_start                        ; Continue loop
    
; ----- Handle Carriage Return (\r) -----
; In Windows line endings (CRLF), \r is followed by \n.
; We simply skip over \r and let the next iteration handle \n as the
; actual line terminator. This approach properly handles both:
;   - Windows: \r\n (we skip \r, \n triggers line count)
;   - Bare \r (rare): would be counted if not followed by \n
; ---------------------------------------------------------------
.handle_cr:
    ; Skip the carriage return character
    inc dword [i]                          ; Move past \r
    
    ; Check if there are more bytes to process
    mov eax, [i]
    cmp eax, [bytes_read]
    jge .loop_end                          ; If end of buffer, we're done
    
    ; Check if next character is \n (complete CRLF pair)
    xor ecx, ecx
    mov cl, [file_buffer + eax]
    cmp cl, 0x0A                           ; Is it \n?
    jne .loop_start                        ; If not, go back to main loop
    
    ; If it IS \n, fall through to .handle_lf to count the line
    ; (We don't increment i again here; we'll process \n normally)
    
; ----- Handle Line Feed (\n) -----
; A Line Feed marks the end of a line. We:
;   - Increment the total line counter
;   - Check if the line was non-empty, and if so increment valid counter
;   - Reset the empty flag for the next line
; ---------------------------------------------------------------
.handle_lf:
    inc dword [total_lines]                ; One more line found
    
    ; Check if the current line had any data
    cmp dword [current_line_empty], 0      ; Was the line non-empty?
    jne .skip_valid                        ; If empty, skip valid increment
    inc dword [valid_lines]                ; Otherwise, count as valid record
    
.skip_valid:
    ; Reset for next line: assume it's empty until we see data
    mov dword [current_line_empty], 1
    
    inc dword [i]                          ; Move past \n
    jmp .loop_start                        ; Continue processing

; ----- End of Buffer Processing -----
; After processing all characters, check if the last line in the file
; didn't end with a newline (no trailing \n). In that case, we still
; need to count it as a line.
; ---------------------------------------------------------------
.loop_end:
    ; Check if the buffer ends without a trailing newline.
    ; If current_line_empty == 0, there's an unterminated final line.
    cmp dword [current_line_empty], 0
    je .count_final_line                   ; If data exists, count it
    jmp .done                              ; Otherwise, we're done

.count_final_line:
    inc dword [total_lines]                ; Count the final line
    inc dword [valid_lines]                ; It has data, so it's valid

.done:
    pop ebp                                ; Restore base pointer
    ret                                    ; Return to caller

; =============================================================================
; SUBROUTINE: print_results
; =============================================================================
; Displays the analysis results to the console in the format:
;   Total records: X
;   Valid records: Y
;
; Uses a helper to convert binary integers to ASCII strings for display.
; The sys_write syscall is used to output each part of the message.
; =============================================================================
print_results:
    push ebp                               ; Save base pointer
    mov ebp, esp                           ; Set up stack frame
    
    ; ----- Print "Total records: " -----
    mov eax, 4                             ; sys_write
    mov ebx, 1                             ; stdout
    mov ecx, total_msg                     ; Message string
    mov edx, 15                            ; Length of "Total records: "
    int 0x80
    
    ; ----- Print total_lines value -----
    push dword [total_lines]               ; Push value to convert
    call print_number                      ; Call conversion/print helper
    add esp, 4                             ; Clean up stack
    
    ; ----- Print newline -----
    mov eax, 4                             ; sys_write
    mov ebx, 1                             ; stdout
    mov ecx, newline                       ; Newline string
    mov edx, 1                             ; Length 1
    int 0x80
    
    ; ----- Print "Valid records: " -----
    mov eax, 4                             ; sys_write
    mov ebx, 1                             ; stdout
    mov ecx, valid_msg                     ; Message string
    mov edx, 15                            ; Length of "Valid records: "
    int 0x80
    
    ; ----- Print valid_lines value -----
    push dword [valid_lines]               ; Push value to convert
    call print_number                      ; Call conversion/print helper
    add esp, 4                             ; Clean up stack
    
    ; ----- Print final newline -----
    mov eax, 4                             ; sys_write
    mov ebx, 1                             ; stdout
    mov ecx, newline                       ; Newline string
    mov edx, 1                             ; Length 1
    int 0x80
    
    pop ebp                                ; Restore base pointer
    ret                                    ; Return to caller

; =============================================================================
; SUBROUTINE: print_number
; =============================================================================
; Converts an unsigned integer to an ASCII string and prints it.
; Input:  [esp+4] = integer value to print
; Output: Printed to stdout via sys_write
;
; Algorithm: Convert number to string in reverse order by repeatedly
; dividing by 10 and storing remainders as ASCII digits. Then reverse
; the string and print it.
; =============================================================================
print_number:
    push ebp                               ; Save base pointer
    mov ebp, esp                           ; Set up stack frame
    sub esp, 16                            ; Allocate local space for digit storage
    
    mov eax, [ebp+8]                       ; Load the number to print
    mov ecx, 10                            ; Divisor (base 10)
    lea edi, [ebp-4]                       ; Point to local buffer
    mov dword [edi], 0                     ; Initialize digit count = 0
    
.convert_loop:
    xor edx, edx                           ; Clear edx for division
    div ecx                                ; edx:eax / 10 => eax=quotient, edx=remainder
    
    add dl, '0'                            ; Convert remainder to ASCII character
    dec edi                                ; Move buffer pointer backward
    mov [edi], dl                          ; Store digit character
    
    inc dword [ebp-4]                      ; Increment digit count
    test eax, eax                          ; Is quotient zero?
    jnz .convert_loop                      ; If not, continue dividing
    
    ; Now print the string
    mov eax, 4                             ; sys_write
    mov ebx, 1                             ; stdout
    mov ecx, edi                           ; Pointer to the first digit
    mov edx, [ebp-4]                       ; Number of digits
    int 0x80
    
    mov esp, ebp                           ; Restore stack pointer
    pop ebp                                ; Restore base pointer
    ret                                    ; Return to caller

