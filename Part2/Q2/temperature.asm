; ============================================================
; Temperature Data Processor
; Reads temperature_data.txt and counts total/valid readings
; Handles LF (\n) and CRLF (\r\n) line endings
; ============================================================

section .data
    file_name   db  "temperature_data.txt", 0
    total_lbl   db  "Total readings: ", 0
    valid_lbl   db  "Valid readings: ", 0
    newline     db  10

section .bss
    fd          resq    1           ; File descriptor
    buffer      resb    4096        ; Buffer to store file contents
    total_cnt   resq    1           ; Total line counter
    valid_cnt   resq    1           ; Valid reading counter
    num_str     resb    20          ; Buffer for number conversion

section .text
    global _start

; ============================================================
; Program Entry Point
; ============================================================
_start:
    ; --------------------------------------------------------
    ; Step 1: Open file
    ; sys_open: rax=2, rdi=filename, rsi=flags (0=O_RDONLY)
    ; --------------------------------------------------------
    mov     rax, 2                  ; sys_open system call
    mov     rdi, file_name          ; File name to open
    mov     rsi, 0                  ; O_RDONLY - read only
    syscall                         ; Call kernel
    
    ; Error handling: Check if file opened successfully
    cmp     rax, 0                  ; Compare return value with 0
    jl      exit_error              ; If negative, file open failed
    
    ; Store file descriptor
    mov     [fd], rax               ; Save file descriptor

    ; --------------------------------------------------------
    ; Step 2: Read file into memory buffer
    ; sys_read: rax=0, rdi=fd, rsi=buffer, rdx=size
    ; --------------------------------------------------------
    mov     rdi, [fd]               ; File descriptor
    mov     rax, 0                  ; sys_read system call
    mov     rsi, buffer             ; Buffer to store data
    mov     rdx, 4096               ; Maximum bytes to read
    syscall                         ; Call kernel
    mov     r12, rax                ; Store bytes read in r12

    ; --------------------------------------------------------
    ; Step 3: Initialize counters
    ; r13 = current index position in buffer
    ; r14 = total line counter (including empty lines)
    ; r15 = valid line counter (non-empty lines)
    ; rbx = flag indicating current line has content (0=empty, 1=has content)
    ; --------------------------------------------------------
    xor     r13, r13                ; index = 0
    xor     r14, r14                ; total lines = 0
    xor     r15, r15                ; valid lines = 0
    xor     rbx, rbx                ; content flag = 0 (line empty so far)

    ; --------------------------------------------------------
    ; Step 4: Traverse buffer line by line
    ; Algorithm:
    ; - Scan each character
    ; - If non-newline char found -> mark line as non-empty (rbx=1)
    ; - If \n found -> process line, update counters, reset flag
    ; - If \r found -> skip (handles Windows CRLF)
    ; --------------------------------------------------------

.loop:
    ; Check if we've reached end of file
    cmp     r13, r12                ; Compare index with bytes read
    jge     .done                   ; If done, handle final line
    
    ; Get current character
    mov     al, [buffer + r13]      ; Load character at current index
    
    ; Check for newline (LF - \n)
    cmp     al, 10                  ; Compare with newline character
    je      .handle_newline         ; If newline, process line end
    
    ; Check for carriage return (CR - \r) for Windows line endings
    cmp     al, 13                  ; Compare with carriage return
    je      .skip_char              ; Skip CR character
    
    ; Regular character found - line has content
    mov     rbx, 1                  ; Set flag: line has content

.skip_char:
    inc     r13                     ; Move to next character
    jmp     .loop                   ; Continue scanning

.handle_newline:
    ; End of line reached - update counters
    inc     r14                     ; Increment total line counter
    add     r15, rbx                ; Add flag (0 or 1) to valid counter
    
    xor     rbx, rbx                ; Reset content flag for next line
    inc     r13                     ; Move to next character
    jmp     .loop                   ; Continue scanning

.done:
    ; Handle last line if file doesn't end with newline
    cmp     rbx, 1                  ; Check if last line had content
    jne     .print_results          ; If not, skip
    inc     r14                     ; Increment total lines
    inc     r15                     ; Increment valid lines

    ; --------------------------------------------------------
    ; Step 5: Display results
    ; --------------------------------------------------------
.print_results:
    ; Print "Total readings: "
    mov     rax, 1                  ; sys_write
    mov     rdi, 1                  ; stdout
    mov     rsi, total_lbl          ; Label pointer
    mov     rdx, 16                 ; Label length
    syscall                         ; Print label
    
    ; Print total count (r14)
    mov     rax, r14                ; Move total to rax
    call    print_number            ; Convert and print
    
    ; Print "Valid readings: "
    mov     rax, 1                  ; sys_write
    mov     rdi, 1                  ; stdout
    mov     rsi, valid_lbl          ; Label pointer
    mov     rdx, 16                 ; Label length
    syscall                         ; Print label
    
    ; Print valid count (r15)
    mov     rax, r15                ; Move valid to rax
    call    print_number            ; Convert and print

    ; --------------------------------------------------------
    ; Step 6: Close file and exit
    ; --------------------------------------------------------
    mov     rax, 3                  ; sys_close
    mov     rdi, [fd]               ; File descriptor
    syscall                         ; Close file
    
    ; Normal exit
    mov     rax, 60                 ; sys_exit
    xor     rdi, rdi                ; Exit code 0
    syscall                         ; Exit

exit_error:
    ; File open failed - exit with error code
    mov     rax, 60                 ; sys_exit
    mov     rdi, 1                  ; Exit code 1 (error)
    syscall                         ; Exit

; ============================================================
; Helper Function: print_number
; Converts number in RAX to string and prints with newline
; ============================================================
print_number:
    ; Point to end of buffer
    mov     rcx, num_str            ; Buffer pointer
    add     rcx, 19                 ; Move to last character
    mov     byte [rcx], 10          ; Add newline at end
    
    mov     rbx, 10                 ; Divisor for decimal conversion

.num_loop:
    xor     rdx, rdx                ; Clear remainder
    div     rbx                     ; Divide RAX by 10
    add     dl, '0'                 ; Convert remainder to ASCII
    dec     rcx                     ; Move pointer left
    mov     [rcx], dl               ; Store digit
    test    rax, rax                ; Check if quotient is zero
    jnz     .num_loop               ; If not zero, continue
    
    ; Print the number
    mov     rax, 1                  ; sys_write
    mov     rdi, 1                  ; stdout
    mov     rsi, rcx                ; Start of number
    mov     rdx, num_str            ; Calculate length
    add     rdx, 20                 ; End of buffer
    sub     rdx, rcx                ; Length = end - start
    syscall                         ; Print number
    
    ret                             ; Return to caller