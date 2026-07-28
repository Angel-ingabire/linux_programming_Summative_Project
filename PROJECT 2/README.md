# Project 2: Assembly-Based Text File Analysis

## Overview

This x86 Assembly program reads a text file (`sensor_readings.txt`) containing sensor measurements (one per line) and computes statistics about the data including total records and valid (non-empty) records.

## Files

- `sensor_analysis.asm` - Main assembly source code
- `sensor_readings.txt` - Sample sensor data file
- `README.md` - This file

## Build Instructions

### Linux (32-bit) with NASM

```bash
nasm -f elf32 -o sensor_analysis.o sensor_analysis.asm
ld -m elf_i386 -o sensor_analysis sensor_analysis.o
./sensor_analysis
```

### Linux (64-bit) with NASM

```bash
nasm -f elf64 -o sensor_analysis.o sensor_analysis.asm
ld -o sensor_analysis sensor_analysis.o
./sensor_analysis
```

## Sample Input

The `sensor_readings.txt` file contains:

```
23.5
24.1
[empty line]
22.8
23.9
[empty line]
25.0
24.7
23.3
[empty line]
22.1
26.2
```

## Expected Output

```
Total records: 12
Valid records: 9
```

## Features

- Handles both Unix (LF) and Windows (CRLF) line endings
- Error handling for file open/read failures
- Detailed comments explaining file operations and counting logic
- Character-by-character traversal of file data
- Counts total lines and valid (non-empty) lines
