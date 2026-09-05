.DATA
    Message db "Hello from Guest Mode",0

.CODE

PUBLIC AsmGuestResume
PUBLIC ReadCS
PUBLIC ReadCSAttrib
PUBLIC ReadSS
PUBLIC ReadSSAttrib
PUBLIC ReadES
PUBLIC ReadESAttrib
PUBLIC ReadDS
PUBLIC ReadDSAttrib
PUBLIC ReadGDTR
PUBLIC ReadIDTR
PUBLIC GuestPointer
PUBLIC GuestRestore

GuestRestore PROC
    clgi
    mov rax, rcx
    vmrun rax
    stgi

GuestRestore ENDP

AsmGuestResume PROC
    ; RCX = GuestContext
    ; RDX = GuestVMCB (físico)
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r12
    push r13
    push r14
    push r15
    push r8
    mov r8, rcx
    sub rsp, 8  

    mov rax, rdx   
    clgi
    vmsave rax
    vmrun rax 
    vmload rax
    stgi

    push rcx
    mov rcx, qword ptr [r8+10h] ;; CPUID BIT
    mov qword ptr [r8+28h], r9 ; Move a mensagem para dentro de r8
    pop rcx

    add rsp, 8
    pop r8
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret
AsmGuestResume ENDP

GuestPointer PROC
    cpuid
    lea r9, Message
    hlt

GuestPointer ENDP

_sgdt PROC
    sgdt fword ptr [rcx]
    ret
_sgdt ENDP

ReadCS PROC
    mov ax, cs
    ret
ReadCS ENDP

ReadSS PROC
    mov ax, ss
    ret
ReadSS ENDP

ReadES PROC
    mov ax, es
    ret
ReadES ENDP

ReadDS PROC
    mov ax, ds
    ret
ReadDS ENDP

ReadCSAttrib PROC
    mov ax, cs
    lar eax, eax
    ret
ReadCSAttrib ENDP

ReadSSAttrib PROC
    mov ax, ss
    lar eax, eax
    ret
ReadSSAttrib ENDP

ReadESAttrib PROC
    mov ax, es
    lar eax, eax
    ret
ReadESAttrib ENDP

ReadDSAttrib PROC
    mov ax, ds
    lar eax, eax
    ret
ReadDSAttrib ENDP

ReadGDTR PROC
    sgdt fword ptr [rcx]
    ret
ReadGDTR ENDP

ReadIDTR PROC
    sidt fword ptr [rcx]
    ret
ReadIDTR ENDP

END