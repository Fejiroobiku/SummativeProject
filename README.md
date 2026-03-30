# Digital Systems and Low-Level Programming - Summative Project

## Project Overview

This project consists of five tasks covering C programming, x86 Assembly, Python C Extensions, Multithreading, and Network Programming.

---

## Table of Contents

1. [Question 1: Array Processor](#question-1-array-processor)  
2. [Question 2: Temperature Data Processor](#question-2-temperature-data-processor)  
3. [Question 3: Vibration Analysis](#question-3-vibration-analysis)  
4. [Question 4: Airport Baggage System](#question-4-airport-baggage-system)  
5. [Question 5: Digital Library Platform](#question-5-digital-library-platform)  
6. [System Requirements](#system-requirements)  
7. [Compilation Instructions](#compilation-instructions)

---

## Question 1: Array Processor

**Description**: C program demonstrating dynamic memory, global variables, and functions. Includes ELF, strace, and gdb analysis.

**Files**: `Q1/array_processor.c`

**Features**:
- `malloc()` / `free()`
- Global `operation_count`
- Functions: `init_array()`, `find_max()`, `calc_sum()`

**Sample Output**:
=== Array Processing Results ===
Array initialized with values: 5 10 15 20 25 30
Maximum value: 30
Sum of values: 105
Average value: 17.50
Total operations: 19
text**Analysis Tools**: `readelf`, `objdump`, `strace`, `gdb`

---

## Question 2: Temperature Data Processor (x86 Assembly)

**Description**: Pure x86-64 assembly program that processes temperature readings and handles LF/CRLF line endings using direct system calls.

**Files**: `Q2/temperature.asm`, `Q2/temperature_data.txt`

**Compilation**:
```bash
nasm -f elf64 temperature.asm && ld temperature.o -o temperature
Sample Output:
textTotal readings: 5
Valid readings: 4

Question 3: Vibration Analysis Python C Extension
Description: High-performance Python C extension for vibration data analysis (peak-to-peak, RMS, std_dev, etc.).
Files: Q3/vibration_analysis.c, Q3/setup.py, Q3/test_vibration.py
Build:
Bashcd Q3 && python3 setup.py build_ext --inplace

Question 4: Airport Baggage Handling System (Multithreading)
Description: Multithreaded producer-consumer simulation using POSIX threads, mutexes, and condition variables.
Files: Q4/baggage_system.c
Compilation:
Bashgcc -pthread -o baggage Q4/baggage_system.c -Wall

Question 5: Digital Library Reservation Platform (Client-Server)
Description: TCP client-server application with multi-client support, authentication, and mutex-protected resources.
Files: Q5/library_server.c, Q5/library_client.c
Compilation:
Bashgcc -pthread -o library_server Q5/library_server.c -Wall
gcc -o library_client Q5/library_client.c -Wall

System Requirements

Linux (Ubuntu 20.04+ or WSL2)
build-essential, nasm, python3-dev, gdb, valgrind, strace


Compilation Instructions
Bash# Question 1
gcc -Wall -O0 -fno-inline -o array_processor Q1/array_processor.c

# Question 2
cd Q2 && nasm -f elf64 temperature.asm && ld temperature.o -o temperature && cd ..

# Question 3
cd Q3 && python3 setup.py build_ext --inplace && cd ..

# Question 4
gcc -pthread -o baggage Q4/baggage_system.c -Wall

# Question 5
gcc -pthread -o library_server Q5/library_server.c -Wall
gcc -o library_client Q5/library_client.c -Wall