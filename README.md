# NtHadouken

**Windows internals · Kernel development · Reverse engineering · Low-level security research**

NtHadouken is a personal research repository maintained by Matheus Santos. It documents experiments, technical notes and practical work focused on understanding Windows internals and the mechanisms beneath the operating system's user-facing abstractions.

The repository is a learning and research workspace. Code and experiments are provided as-is and are **not intended for production use**.

## Research areas

- Windows architecture and internals
- Processes, threads and memory management
- System calls and kernel data structures
- Windows drivers and kernel-mode development
- WinDbg and kernel debugging
- Reverse engineering
- AMD64 architecture and hardware-assisted virtualization
- Security mechanisms and low-level experimentation

## Technical papers

The papers below are individual study notes. Some are introductory and may contain version-specific observations; consult primary documentation before relying on implementation details.

| Topic | Paper |
|---|---|
| System-call path | [nt!KiSystemCall64](Papers/nt!KiSystemCall64.md) |
| Processor control region | [Windows KPCR](Papers/Windows-KPCR.md) |
| Interrupt handling | [Interrupt Descriptor Table](Papers/Windows-IDT.md) |
| Kernel data structures | [Kernel Structures](Papers/KernelStructs.md) |
| Process environment | [PEB](Papers/PEB.md) |
| Thread environment | [TEB](Papers/TEB.md) |
| Process instrumentation | [NtGlobalFlag](Papers/NtGlobalFlag.md) |
| x64 calling convention | [Shadow Space](Papers/ShadowSpace.md) |

## Featured research: AMD SVM / NtVisor

The [NtVisor technical documentation](Tutorials/Virtualization/AMD64/NtVisor/Documentation/NtVisor-Documentacao.md) describes an experimental Windows kernel driver that explores AMD Secure Virtual Machine (SVM), VMCB setup, VM exits and guest/host state transitions.

This is a proof of concept, not a complete hypervisor. The documentation records known limitations, including the absence of nested paging and the lack of guest/host memory isolation. Do not run kernel experiments on a machine containing data you cannot afford to lose.

## Tutorials and experiments

The `Tutorials/` and `src/` directories contain practical material, including debugging, drivers, PowerShell, reverse engineering and virtualization research.

## Working principles

Research notes should distinguish between:

- **Observed:** directly reproduced or measured behavior.
- **Inferred:** an explanation supported by observations but not yet independently established.
- **Confirmed:** a conclusion supported by reproducible evidence and appropriate references.

Prefer primary sources, record the tested environment, preserve negative results and avoid presenting undocumented, version-dependent implementation details as stable interfaces.

## References

- [Microsoft Learn — Windows drivers](https://learn.microsoft.com/windows-hardware/drivers/)
- [Microsoft Learn — WinDbg](https://learn.microsoft.com/windows-hardware/drivers/debugger/)
- [Windows Internals](https://windows-internals.com/)
- [AMD64 Architecture Programmer's Manual](https://docs.amd.com/v/u/en-US/24593_3.45_APM_Vol2_PUB)
- [Intel Software Developer's Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

## Contributing

Corrections, reproducible findings and technical discussion are welcome. Please include references, tested environment, reproduction steps and known limitations when proposing technical changes.

## Maintainer

**Matheus Santos** · [@lnt2eh](https://github.com/lnt2eh)

---

*Research. Build. Investigate.*
