# NtHadouken

**Windows Internals · Kernel Development · Reverse Engineering · Low-Level Security Research**

NtHadouken is a research repository maintained by Matheus Santos. It brings together technical articles, experiments, source code, and practical notes focused on understanding Windows internals and the mechanisms beneath the operating system's abstractions.

The repository is intended for learning, experimentation, and collaborative research. Code and experiments are provided as-is and are **not intended for production use**.

## Research Areas

- Windows architecture and internals
- Processes, threads, and memory management
- System calls and kernel data structures
- Windows drivers and kernel-mode development
- WinDbg and kernel debugging
- Reverse engineering
- AMD64 architecture and hardware-assisted virtualization
- Operating-system security mechanisms and low-level experimentation

## Technical Papers

The papers are individual study notes. Some describe version-specific implementation details; always verify offsets and behavior against the Windows build being investigated.

- [Kernel Structures — EPROCESS and ETHREAD](Papers/KernelStructs.md)
- [NtGlobalFlag](Papers/NtGlobalFlag.md)
- [PEB](Papers/PEB.md)
- [Shadow Space](Papers/ShadowSpace.md)
- [TEB](Papers/TEB.md)
- [Windows IDT](Papers/Windows-IDT.md)
- [Windows KPCR](Papers/Windows-KPCR.md)
- [The nt!KiSystemCall64 System Call Handler](Papers/nt!KiSystemCall64.md)

## Tutorials and Experiments

The `Tutorials/` directory contains practical material covering Windows fundamentals, debugging, driver development, PowerShell, reverse engineering, and virtualization.

## Featured Project: NtVisor

The NtVisor experiment explores AMD Secure Virtual Machine (SVM) from a Windows kernel driver. Its documentation describes VMCB setup, guest/host state transitions, VM exits, and the limitations of the current proof of concept.

This is an experimental project, not a complete hypervisor. It does not provide guest/host memory isolation and should only be tested in a disposable virtual machine.

## Research Principles

When documenting a finding, distinguish between:

- **Observed:** directly reproduced or measured behavior.
- **Inferred:** an explanation supported by evidence but not yet independently confirmed.
- **Confirmed:** a conclusion supported by reproducible evidence and appropriate references.

Prefer primary sources, record the tested environment, preserve negative results, and avoid presenting undocumented, version-dependent implementation details as stable interfaces.

## Contributing

Contributions are welcome. Corrections, reproducible findings, improvements to explanations, and new experiments are all valuable. Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.

## References

- [Microsoft Learn — Windows drivers](https://learn.microsoft.com/windows-hardware/drivers/)
- [Microsoft Learn — WinDbg](https://learn.microsoft.com/windows-hardware/drivers/debugger/)
- [AMD64 Architecture Programmer's Manual](https://docs.amd.com/v/u/en-US/24593_3.45_APM_Vol2_PUB)
- [Intel Software Developer's Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

## Maintainer

**Matheus Santos** · [@lnt2eh](https://github.com/lnt2eh)

*Research. Build. Investigate.*
