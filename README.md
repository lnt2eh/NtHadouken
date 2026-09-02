# NtHadouken

<p align="center">
  <img src="https://github.com/lnt2eh/NtHadouken/blob/main/assets/UpdateLogo.png" />
</p>

## About

NtHadouken is a personal research repository focused on Windows Internals, low-level development, reverse engineering and Windows kernel development.

The repository contains technical papers, experiments, source code, tools and references created or collected during my studies.

The goal is simple: document what I am learning and build a structured knowledge base around the internal components of the Windows operating system.

Topics covered throughout the repository include:

* Windows Internals
* Kernel Development
* Windows Drivers
* Debugging
* Reverse Engineering
* Windows Architecture
* Memory Management
* Thread and Process Internals
* Low-level Security Research

This repository is constantly evolving as new research, experiments and documentation are added.

---

## Technical Papers

The `Papers` directory contains technical articles focused on specific Windows internals and low-level mechanisms.

### Available Papers

* [`nt!KiSystemCall64`](Papers/nt!KiSystemCall64.md)
* [`KPCR - Kernel Processor Control Region`](Papers/Windows-KPCR.md)
* [`IDT - Interrupt Descriptor Table`](Papers/Windows-IDT.md)
* [`Kernel Structures`](Papers/KernelStructs.md)
* [`NtGlobalFlag`](Papers/NtGlobalFlag.md)
* [`PEB - Process Environment Block`](Papers/PEB.md)
* [`TEB - Thread Environment Block`](Papers/TEB.md)
* [`Shadow Space`](Papers/ShadowSpace.md)

## Resources

### Articles and Blogs

* [Mark Russinovich Blog](https://learn.microsoft.com/en-us/archive/blogs/markrussinovich/)
* [Matt Graeber](https://www.exploit-monday.com/)
* [James Forshaw](https://tyranidslair.blogspot.com/)
* [AdSecurity](https://adsecurity.org/)
* [CERT/CC](https://www.kb.cert.org/vuls/)
* [The Old New Thing](https://devblogs.microsoft.com/oldnewthing/)
* [Hexacorn](http://www.hexacorn.com/blog/)
* [Didier Stevens](https://blog.didierstevens.com/)
* [Oddvar Moe](https://oddvarmoe.no/)
* [Windows Kernel Explorer](https://github.com/wke/wke)

### Communities

* [r/WindowsInternals](https://www.reddit.com/r/WindowsInternals/)
* [OSR](https://www.osr.com/)
* [Windows Hacking - ired.team](https://www.ired.team/)

### Books

* *Windows Internals, Part 1* — Pavel Yosifovich, Alex Ionescu, Mark Russinovich, David Solomon
* *Windows Internals, Part 2* — Andrea Allievi, Mark Russinovich, Alex Ionescu, David Solomon
* *Windows Kernel Programming* — Pavel Yosifovich

### Technical References

* [Microsoft Windows Driver Documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/)
* [Windows Internals Book](https://windows-internals.com/)
* [Sysinternals](https://learn.microsoft.com/en-us/sysinternals/)
* [Windows Developer Center](https://developer.microsoft.com/en-us/windows)

### Tools

* [Sysinternals](https://learn.microsoft.com/en-us/sysinternals/)
* [Windows Kernel Explorer](https://github.com/wke/wke)
* [x64dbg](https://x64dbg.com/)
* [WinDbg](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/)

---

## Repository Structure

```text
NtHadouken/
│
├── README.md
│
├── assets/
│   └── NtHadouken.png
│
├── Papers/
│   ├── A-Trip-to-ntKiSystemCall64.md
│   ├── KPCR.md
│   ├── ShadowSpace.md
│   ├── TEB.md
│   └── PEB.md
│
├── docs/
│   ├── index.md
│   └── how-to-contribute.md
│
├── Tutorials/
│   │
│   ├── Beginner/
│   │
│   ├── PowerShell/
│   │   └── CursoPowerShell.md
│   │
│   ├── Drivers/
│   │   ├── Setting-up-environment-for-creating-Windows-drivers.md
│   │   └── ...
│   │
│   ├── Debugging/
│   │   ├── Set-up-KDNET-network-kernel-debugging-automatically.md
│   │   ├── Analyzing-BSOD-with-WinDBG.md
│   │   ├── Extracting-Syscalls.md
│   │   └── Debugging-Kernel-Drivers.md
│   │
│   ├── Windows-Internals/
│   │   ├── Thread-Scheduling.md
│   │   └── Understanding-Paging.md
│   │
│   ├── Reverse-Engineering/
│   │   ├── Reversing-NtDLL.md
│   │   └── Hooking-SSDT.md
│   │
│   ├── Virtualization/
│   │
│   └── Misc/
│       └── Writing-Kernel-Exploits.md
│
├── src/
│   │
│   ├── drivers/
│   │   ├── basic/
│   │   │   ├── HelloWorldDriver/
│   │   │   └── SimpleDeviceDriver/
│   │   │
│   │   ├── security/
│   │   │   ├── KernelKeylogger/
│   │   │   └── ProcessHider/
│   │   │
│   │   ├── debugging/
│   │   │   ├── KernelDebugger/
│   │   │   └── BSODTrigger/
│   │   │
│   │   └── memory/
│   │       ├── KernelMemoryScanner/
│   │       └── VirtualMemoryMonitor/
│   │
│   └── tools/
│
├── CONTRIBUTING.md
└── LICENSE
```

---

## Roadmap

* [x] Publish technical papers related to Windows Internals
* [x] Build a collection of technical references
* [x] Improve repository organization
* [ ] Add additional Windows Internals papers
* [ ] Expand the driver development section
* [ ] Add debugging and reverse engineering experiments
* [ ] Develop small tools for research and debugging
* [ ] Improve the project documentation

---

## Contributions

Contributions, corrections and technical discussions are welcome.

If you find an error in one of the papers or have suggestions for improving the documentation, feel free to open an issue or submit a pull request.

---

## Maintainer

**Matheus Santos**

GitHub: [@lnt2Eh](https://github.com/lnt2Eh)

---

This repository is maintained as a personal research and learning project. Content may change as the research evolves and previous material may be updated when new information becomes available.
