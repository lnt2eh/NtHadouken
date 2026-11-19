# NtGlobalFlag

<p><img src="https://github.com/lnt2eh/NtHadouken/blob/main/assets/NtGlobalFlag.png"/></p>

NtGlobalFlag is a field in the Windows Process Environment Block (PEB). It contains diagnostic flags, including heap validation and checking options that can be enabled in debugging scenarios. Malware has also used these flags as one of several signals to identify debugging environments and alter its behavior. Understanding NtGlobalFlag is one of the topics worth exploring when studying Windows Internals.

## Introduction

During debugging, the system may expose flags such as:

- `FLG_HEAP_VALIDATE_PARAMETERS`
- `FLG_HEAP_ENABLE_TAIL_CHECK`
- `FLG_HEAP_ENABLE_FREE_CHECK`

These flags enable additional heap validation and checking behavior. The bit mask `0x70` is commonly associated with these flags in the NtGlobalFlag field. The following educational example uses Microsoft compiler intrinsics, so compile it with Microsoft Visual Studio.

```c
// x64 example

DWORD pNtGlobalFlag = NULL;
PPEB pPeb = (PPEB)__readgsqword(0x60);
pNtGlobalFlag = *(PDWORD)((PBYTE)pPeb + 0xBC);

if ((pNtGlobalFlag & 0x70) != 0)
{
    printf("Debugger-related heap flags detected\n");
}
```

### Location

The NtGlobalFlag offset in the PEB is `0x68` on x86 and `0xBC` on x64.

## PEB Structure

| Offset (x86) | Offset (x64) | Definition | Versions |
|:---:|:---:|---|---|
| 0x3C | 0x70 | ULONG TlsExpansionCounter | all |
| | 0x74 | UCHAR Padding2 [4] | 6.3 and higher |
| 0x40 | 0x78 | PVOID TlsBitmap | all |
| 0x44 | 0x80 | ULONG TlsBitmapBits [2] | all |
| 0x4C | 0x88 | PVOID ReadOnlySharedMemoryBase | all |
| 0x50 | 0x90 | PVOID ReadOnlySharedMemoryHeap | 3.10 to 5.2 |
| 0x50 | 0x90 | PVOID HotpatchInformation | 6.0 to 6.2 |
| 0x50 | 0x90 | PVOID SparePvoid0 | 6.3 to 1607 |
| 0x50 | 0x90 | PVOID SharedData | 1703 and higher |
| 0x54 | 0x98 | PVOID *ReadOnlyStaticServerData | all |
| 0x58 | 0xA0 | PVOID AnsiCodePageData | all |
| 0x5C | 0xA8 | PVOID OemCodePageData | all |
| 0x60 | 0xB0 | PVOID UnicodeCaseTableData | all |
| 0x64 | 0xB8 | ULONG NumberOfProcessors | 3.51 and higher |
| 0x68 | 0xBC | ULONG NtGlobalFlag | 3.51 and higher |
| 0x68 | | ULONG NtGlobalFlag | 3.10 to 3.50 |
| 0x70 | 0xC0 | LARGE_INTEGER CriticalSectionTimeout | all |

Source: [PEB Structure](https://geoffchappellmirror.github.io/studies/windows/km/ntoskrnl/inc/api/pebteb/peb/index.htm)

---
