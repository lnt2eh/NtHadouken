# How the nt!KiSystemCall64 System Call Handler Works in Windows

This article explores the internals of the Windows operating system, specifically the `nt!KiSystemCall64` system call handler. It explains how system calls are essential mechanisms that allow the operating system to request services from the kernel, enabling operations that cannot be performed directly from user mode, such as creating files on a disk.

The article examines the distinction between user mode and kernel mode, showing how system calls bridge communication between these two modes. Through a detailed analysis and disassembly of the `KiSystemCall64` handler, we examine the transition from user mode to kernel mode, the role of the `swapgs` instruction, and the use of Model-Specific Registers (MSRs) in this process. The study also introduces the `KUSER_SHARED_DATA` structure and highlights its role in reducing the need for transitions. These findings contribute to a deeper understanding of Windows internals and provide an introductory guide to this subject.

When I started studying Windows, one of the things I was curious about was how the system handled system calls. Before discussing the handler, let's define a few basic concepts.

## What Are System Calls?

System calls are mechanisms provided by the operating system to request services that cannot be performed directly by user-mode code. For example, creating a file on a disk. When a user clicks "Create a new text file" in Windows, the operating system uses a system call to ask the kernel to perform the operation. This separation helps protect the operating system from activities that could cause damage if users were allowed to perform privileged operations directly. Speaking of protection, let's look at the processor protection rings, which define privilege levels.

## User Mode and Kernel Mode

When we talk about user mode and kernel mode, we are referring to CPU privilege levels. On x86/x64, user-mode applications normally execute at CPL 3, while Windows kernel code executes at CPL 0. Therefore, it is not correct to say that the entire operating system runs in ring 3: Windows includes both user-mode components and privileged kernel-mode components. This separation limits which instructions and memory regions each context can access.

On Windows x64, the `SYSCALL` instruction performs a controlled transition from CPL 3 to CPL 0 using MSRs configured by the system, including `IA32_LSTAR`, which provides the kernel entry address. Unlike an `INT` instruction, `SYSCALL` does not dispatch execution through an IDT entry; it uses an architecture-specific mechanism. The kernel entry code validates and handles the request and, at the appropriate point, returns to user mode.

Figure 1 illustrates the flow of a Windows system call, starting in user mode. The process begins with a call to a Windows API function, which in turn uses `ntdll.dll` to transition to kernel mode through a system instruction (syscall). In this example, the Windows API function `CreateFile` is used as a reference; internally, it maps to the `NtCreateFile` function in `ntdll.dll`, which invokes the kernel system-call path.

<p align="center">
  <img src="https://media.invisioncic.com/u323382/monthly_2025_01/FluxogramaSyscall.png.f6a09c83c66d0416f0318d385dc6a3bc.png" />
</p>

<p align="center">
  Figure 1 - CreateFile() flowchart*
</p>

## The Windows System Call Handler

The `nt!KiSystemCall64` routine is the Windows system call handler. It is responsible for handling entry into kernel mode when a system call is performed.

When a system call is made on a 64-bit system, the `syscall` instruction is executed. Let's inspect the `NtCreateFile` stub in WinDbg to understand the process.

In Figure 2, the `NtCreateFile` function is shown in assembly, allowing us to follow the execution flow. First, the argument located in `RCX` is moved to `R10`, matching the calling convention expected by the kernel. Next, the service index in the System Service Descriptor Table (SSDT) is loaded into the `EAX` register. For `NtCreateFile`, the index shown in this example is `55h`. The kernel processes this index during the transition to kernel mode, which occurs through the syscall instruction.

Windows has two instructions that can be used to enter the system-call path: `syscall`, represented by opcode `0f05`, and `int 2Eh`, represented by opcode `cd2e`. They use different entry mechanisms: `syscall` reaches the Windows `nt!KiSystemCall64` path, while `int 2Eh` reaches `nt!KiSystemCall`. Before these instructions execute, the syscall stub performs a test to select the appropriate path, as shown in Figure 2.

<p align="center">
  <img src="https://media.invisioncic.com/u323382/monthly_2025_01/NtCreateFileAsm.png.075ba58ad3edf04d2c78e52f163801fd.png" />
</p>

<p align="center">
  Figure 2 - Code excerpt from the NtCreateFile function in ntdll.dll*
</p>

When the `syscall` instruction executes in user mode, the value stored in the `IA32_LSTAR` MSR is loaded into `RIP`, transferring execution to the kernel. This value is an address known as the kernel RIP entry point for system calls. In other words, it points to the system call entry routine, commonly identified in the analyzed build as `nt!KiSystemCall64`.

The `nt!KiSystemCall64` routine is the 64-bit system-call entry handler. When the `syscall` instruction executes, control transfers to the kernel routine whose address is specified by an MSR, which can be accessed through the `rdmsr` (read) and `wrmsr` (write) instructions.

## KUSER_SHARED_DATA

The `KUSER_SHARED_DATA` structure is used to share information from the kernel with user-mode processes, avoiding the need for a user-mode-to-kernel-mode transition for every piece of shared information. You can find more details about this structure in Microsoft's documentation for `KUSER_SHARED_DATA`.

On Windows x64, the user-mode view of `KUSER_SHARED_DATA` is normally mapped at `0x7FFE0000`; its kernel alias is `0xFFFFF78000000000`. The `SystemCall` field, at offset `0x308` in modern Windows x64 versions, is consulted by the `ntdll` syscall stub. When the relevant bit is clear (normally observed as zero), the stub follows the `SYSCALL` path. When it is set, the stub may select the legacy `INT 2Eh` path. Therefore, this field does not indicate whether the system is x86 or x64: it selects the transition mechanism used by the stub on an x64 system. The layout and use of these fields depend on the Windows version; verify them using symbols and disassembly for the build being analyzed.

The value of this field should not be interpreted as an indicator of the system architecture. On Windows x64, it acts as a selector for the transition path used by the system-call stub. Its meaning should be considered together with the `ntdll` disassembly and the specific Windows version, since internal structures and their fields can change between builds.

When the `syscall` instruction executes, the value of `IA32_LSTAR` is used as the kernel entry address in `RIP`. This is the system-call entry point. After that, kernel mode executes the `nt!KiSystemCall64` handler.

## Disassembling the System Call Handler

When disassembling the `KiSystemCall64` handler, we can see that the first instruction executed is `swapgs`, as shown in Figure 3.

<p align="center">
  <img src="https://media.invisioncic.com/u323382/monthly_2025_01/Swapgs.png.fb737f51ccfdec79cd8d7ebe34d03b3a.png" />
</p>

<p align="center">
  Figure 3 - The swapgs instruction at the beginning of the system call handler*
</p>

There are actually two different system-call handlers, one with and one without the "Shadow" keyword. The word "Shadow" comes from the Kernel Virtual Address Shadow feature, which was introduced to mitigate the Meltdown vulnerability.

The `swapgs` instruction is a privileged instruction used to exchange the current GS base value with the value stored in the `C0000102H` MSR (`IA32_KERNEL_GS_BASE`). In other words, the GS base value is associated with the value contained in the `IA32_GS_BASE` MSR. On Windows x64 systems, the values are used as follows:

- `IA32_KERNEL_GS_BASE` — Pointer to the current Processor Control Region (PCR), specifically the kernel processor control region (KPCR).
- `IA32_GS_BASE` — Pointer to the current thread's Thread Environment Block (TEB).

Thus, in x64 long mode, the GS segment points to the current thread's TEB in user mode, while in kernel mode it points to the current processor's PCR.

The next instruction, `mov qword ptr gs:[10h],rsp`, saves the user-mode stack pointer. Saving the user-mode stack pointer (`RSP`) during the transition from user mode to kernel mode ensures that, when control is returned to user mode, program execution can continue correctly with the user thread's stack context preserved.

## Conclusion

In this article, we explored the role of system calls in Windows, focusing on the system-call handler `nt!KiSystemCall64`. System calls are essential for allowing user-mode components to request critical services from the kernel, such as creating files. We also examined how the transition between user mode and kernel mode uses the `swapgs` instruction and how the `KUSER_SHARED_DATA` structure provides shared information to user-mode processes. Finally, we discussed how the system preserves execution context so that the program can continue correctly when control returns to user mode.
