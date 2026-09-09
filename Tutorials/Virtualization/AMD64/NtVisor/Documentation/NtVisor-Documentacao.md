# NtVisor — Documentação Técnica

**Projeto:** NtHadouken
**Componente principal:** NtVisor (driver de kernel)

**Autor / Mantenedor:** Matheus Santos (_int2Eh)

**Categoria:** Pesquisa acadêmica em virtualização assistida por hardware

**Tema de pesquisa:** AMD SVM VMRUN — Abstração Intrínseca e Controle Arquitetural

**Status:** Não destinado a uso em produção

**Versão do documento:** 1.0

---

## Sumário

1. [Visão geral](#1-visão-geral)
2. [Estrutura de arquivos](#2-estrutura-de-arquivos)
3. [Fluxo de operação](#3-fluxo-de-operação)
4. [IOCTLs](#4-ioctls)
5. [NtVisor.c — driver de kernel](#5-ntvisorc--driver-de-kernel)
   - 5.1 [Variáveis globais](#51-variáveis-globais)
   - 5.2 [`ConvertAttrib`](#52-convertattrib)
   - 5.3 [`SVM_CHECK`](#53-svm_check)
   - 5.4 [`SVM_VMRUN`](#54-svm_vmrun)
   - 5.5 [`NtVisorDeviceControlHandle`](#55-ntvisordevicecontrolhandle)
   - 5.6 [`NtVisorCreateOrCloseHandle`](#56-ntvisorcreateorclosehandle)
   - 5.7 [`NtVisorUnload`](#57-ntvisorunload)
   - 5.8 [`DriverEntry`](#58-driverentry)
6. [svm_resources.h](#6-svm_resourcesh)
   - 6.1 [MSRs relevantes](#61-msrs-relevantes)
   - 6.2 [Códigos de status customizados](#62-códigos-de-status-customizados)
   - 6.3 [Estruturas da VMCB](#63-estruturas-da-vmcb)
   - 6.4 [Protótipos externos](#64-protótipos-externos)
7. [Guest.asm](#7-guestasm)
   - 7.1 [Dado estático](#71-dado-estático)
   - 7.2 [`GuestRestore`](#72-guestrestore)
   - 7.3 [`AsmGuestResume`](#73-asmguestresume)
   - 7.4 [`GuestPointer`](#74-guestpointer)
   - 7.5 [Rotinas de leitura de estado da CPU](#75-rotinas-de-leitura-de-estado-da-cpu)
   - 7.6 [`ReadGDTR` / `ReadIDTR` / `_sgdt`](#76-readgdtr--readidtr--_sgdt)
8. [NtVisorClient.c — cliente em user-mode](#8-ntvisorclientc--cliente-em-user-mode)
   - 8.1 [Constantes](#81-constantes)
   - 8.2 [`NtVisorOpenDevice` / `NtVisorCloseDevice`](#82-ntvisoropendevice--ntvisorclosedevice)
   - 8.3 [`NtVisorPrintError`](#83-ntvisorprinterror)
   - 8.4 [`NtVisorEnable`](#84-ntvisorenable)
   - 8.5 [`NtVisorVmrun`](#85-ntvisorvmrun)
   - 8.6 [`main`](#86-main)
9. [Pontos de atenção conhecidos](#9-pontos-de-atenção-conhecidos)
10. [Contribuição](#10-contribuição)
11. [Referências](#11-referências)

---

## 1. Visão geral

NtHadouken é um driver de kernel para Windows (WDM puro, sem framework KMDF) cujo objetivo é experimentar, de forma controlada, com a extensão de virtualização AMD-V (SVM — Secure Virtual Machine). O driver não implementa um hypervisor completo. Ele executa um único ciclo de VMRUN sobre a CPU atual, faz o guest rodar uma instrução `CPUID` seguida de `HLT`, e usa esse VM exit como ponto de prova de conceito para observar o comportamento da VMCB, o mecanismo de reentrada no guest e a comunicação de dados entre guest e host através de uma estrutura de contexto.

O projeto é dividido em três componentes:

- **NtVisor.sys** — driver de kernel (`.c` + `.h`) responsável por alocar a VMCB, habilitar SVM na CPU, montar o estado inicial do guest e executar VMRUN.
- **Guest.asm** — rotinas em assembly (MASM) que executam as instruções privilegiadas SVM (`CLGI`, `VMSAVE`, `VMRUN`, `VMLOAD`, `STGI`) e leem seletores/atributos de segmento.
- **NtVisorClient.exe** — aplicação em user-mode que abre o dispositivo exposto pelo driver e dispara os dois IOCTLs disponíveis.

A comunicação entre user-mode e kernel-mode é feita via `DeviceIoControl` com `METHOD_BUFFERED`, através de um único dispositivo (`\Device\NtVisor`) exposto pelo link simbólico `\??\NtVisorSymbolName`.

---

## 2. Estrutura de arquivos

| Arquivo | Papel |
|---|---|
| `NtVisor.c` | Lógica principal do driver: DriverEntry, IOCTLs, SVM_CHECK, SVM_VMRUN, unload |
| `svm_resources.h` | Estruturas da VMCB, união de MSRs, protótipos das rotinas ASM, constantes |
| `Guest.asm` | Implementação em MASM das rotinas privilegiadas e leitura de registradores de segmento |
| `NtVisorClient.c` | Cliente em user-mode que consome o driver via IOCTL |

---

## 3. Fluxo de operação

1. O driver é carregado (`DriverEntry`), cria o `DEVICE_OBJECT` e o link simbólico.
2. O cliente em user-mode abre o dispositivo com `CreateFileW`.
3. O cliente envia `NTVISOR_ENABLE`. O driver executa `SVM_CHECK`, que verifica suporte a SVM via CPUID e habilita `EFER.SVME`.
4. Se a checagem anterior tiver sucesso, o cliente envia `NTVISOR_VMRUN`. O driver executa `SVM_VMRUN`, que:
   - aloca a VMCB, a VM_HSAVE area, a stack do guest e o contexto (`guestCTX`);
   - zera toda a memória alocada;
   - programa `VM_HSAVE_PA` com o endereço físico da HSAVE area;
   - monta o estado inicial do guest na `_vmcb_state_save_area` (RIP, RSP, CR0–CR4, segmento de código, EFER com SVME/LME/LMA);
   - copia a VMCB montada para uma segunda VMCB (`vmcb_runtime`), que é a efetivamente usada na execução;
   - chama `AsmGuestResume`, que executa `VMSAVE` → `VMRUN` → `VMLOAD` sobre o endereço físico da VMCB runtime;
   - ao retornar do VM exit, ajusta `RIP` do guest para `nRip` (RIP após a instrução que causou o exit) e grava `0xDEADBEEF` em RCX do contexto;
   - chama `AsmGuestResume` uma segunda vez para retomar o guest, que agora executa `HLT` e efetivamente sai da VM;
   - a rotina assembly copia o ponteiro para a string "Hello from Guest Mode" (carregado pelo guest via `lea r9, Message`) para dentro do `guestCTX`;
   - o driver lê `ctx->Message` e copia a string para o buffer de saída do IRP.
5. O cliente recebe a mensagem retornada pelo kernel e a imprime no console.
6. Ao descarregar o driver, todos os buffers contíguos são liberados e o dispositivo é removido.

---

## 4. IOCTLs

Definidos tanto no driver quanto no cliente, com `FILE_DEVICE_UNKNOWN` e `METHOD_BUFFERED`:

| IOCTL | Código | Entrada | Saída | Função no driver |
|---|---|---|---|---|
| `NTVISOR_ENABLE` | `CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2080, METHOD_BUFFERED, FILE_ANY_ACCESS)` | Nenhuma | Nenhuma | Executa `SVM_CHECK` e retorna o status via `Irp->IoStatus.Status` |
| `NTVISOR_VMRUN` | `CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2081, METHOD_BUFFERED, FILE_ANY_ACCESS)` | Nenhuma | String (mensagem do guest) | Executa `SVM_VMRUN` somente se `NTVISOR_ENABLE` já tiver retornado sucesso; copia `ctx->Message` para o `SystemBuffer` do IRP |

`NTVISOR_VMRUN` depende do estado global `GStatus`, setado pela chamada anterior a `NTVISOR_ENABLE`. Se `NTVISOR_VMRUN` for chamado sem sucesso prévio de `NTVISOR_ENABLE`, o driver apenas zera `Irp->IoStatus.Information` e retorna `STATUS_INVALID_DEVICE_REQUEST`.

---

## 5. NtVisor.c — driver de kernel

### 5.1 Variáveis globais

```c
PKVMCB vmcb = NULL;
PKVMCB vmcb_runtime = NULL;
PVOID vm_hsave_area = NULL;
PVOID guest_stack = NULL;
guestCTX* ctx = NULL;
NTSTATUS GStatus = STATUS_UNSUCCESSFUL;
```

- `vmcb` — VMCB montada com os valores iniciais do guest.
- `vmcb_runtime` — cópia da `vmcb` que é efetivamente passada para `VMRUN`. A separação entre as duas existe para preservar o estado original montado enquanto a runtime é alterada pelo hardware durante a execução (EXITCODE, EXITINFO, nRip, etc.).
- `vm_hsave_area` — área reservada para o host state, exigida pelo SVM antes de qualquer `VMRUN` (4 páginas alocadas, embora a arquitetura exija apenas uma).
- `guest_stack` — página usada como pilha do guest.
- `ctx` (`guestCTX`) — estrutura de troca de dados entre guest e host, preenchida pela rotina assembly após o VM exit.
- `GStatus` — guarda o resultado de `SVM_CHECK`, usado como pré-condição para permitir `NTVISOR_VMRUN`.

### 5.2 `ConvertAttrib`

```c
UINT16 ConvertAttrib(UINT32 rawAttrib)
{
    return (UINT16)(rawAttrib & 0xFFFF);
}
```

Trunca o valor retornado por `LAR` (Load Access Rights), que vem em formato de 32 bits com o byte de acesso e flags nos bits mais baixos, para o formato de atributo de 16 bits esperado pelo campo `attrib` de `vmcb_segment_t`.

### 5.3 `SVM_CHECK`

Verifica pré-requisitos antes de qualquer tentativa de `VMRUN`:

1. `__cpuid(cpuInfo, 0x80000001)` e checagem do bit 2 de ECX. Se zero, a CPU não suporta SVM e a função retorna `SVM_NOT_SUPPORTED` sem tocar em nenhum MSR — evitar ler MSRs relacionados a SVM numa CPU sem suporte gera `#GP`.
2. Fixa afinidade da thread atual na CPU 0 com `KeSetSystemAffinityThreadEx((KAFFINITY)1)`, já que o estado de SVM (EFER, VM_CR) é por-core.
3. Lê `VM_CR` (`0xC0010114`). Se `SVMDIS` (bit 4) estiver setado, SVM foi desabilitado permanentemente pela BIOS/firmware e não pode ser reativado em software; a função retorna `SVM_SVMDIS_ERROR`.
4. Se `SVMDIS` estiver limpo, lê `EFER` (`0xC0000080`), seta o bit 12 (`SVME`) e regrava o MSR.
5. Relê `EFER.SVME` para confirmar que o bit foi de fato aceito pelo hardware. Se sim, retorna `NTVISOR_CHECK_STATUS_SUCCESS`; caso contrário, reverte a afinidade e retorna `SVM_SVME_DISABLE`.

Nota: a afinidade de thread só é revertida (`KeRevertToUserAffinityThreadEx`) nos caminhos de erro. No caminho de sucesso a thread permanece fixada na CPU 0, o que é intencional já que `SVM_VMRUN` precisa continuar executando na mesma CPU em que `EFER.SVME` foi habilitado.

### 5.4 `SVM_VMRUN`

Ponto central do driver. Sequência de alocação e montagem:

**Alocação de memória física contígua:**

- `vmcb` — `MmAllocateContiguousMemorySpecifyCache`, uma página, restrita à faixa `0x800000`–`0xFFFFFF`, cache `MmCached`.
- `vm_hsave_area` — `MmAllocateContiguousMemory`, 4 páginas, limite superior `0xFFFFFF`.
- `guest_stack` — `MmAllocateContiguousMemory`, uma página, mesma faixa.
- `ctx` (`guestCTX`) — `MmAllocateContiguousMemory`, uma página, mesma faixa.

Toda a memória é zerada com `RtlZeroMemory` logo após a alocação, para evitar lixo de kernel residual na VMCB — um campo mal inicializado na área de controle pode levar a um `#VMEXIT` inesperado ou a um comportamento indefinido do processador durante `VMRUN`.

**Programação do host save state:**

```c
__writemsr(VM_HSAVE_PA_MSR, vm_hsave_pa.QuadPart);
```

Esse MSR (`0xC0010117`) precisa apontar para uma página física válida antes de qualquer `VMRUN`; é onde o processador salva o estado do host automaticamente durante a entrada na VM.

**Montagem do control area (`vmcb->vmcb_ctrl`):**

- `GuestAsid = 1` — ASID diferente de zero é obrigatório; ASID 0 é reservado e causa `#VMEXIT(INVALID)`.
- `InterceptMisc2` e os campos `vectorInterrupt4`/`vectorInterrupt5` habilitam interceptação de instruções específicas (incluindo `VMRUN` e `CPUID`, conforme os bits setados), necessários para o guest poder executar `CPUID` de forma interceptável e para proteger o host de re-entrância indevida em `VMRUN` dentro do guest.
- `v_tpr = 0` e `interrupt_shadow = 0` — sem prioridade de interrupção virtual e sem shadow de interrupção pendente na entrada.
- `vmcb_clean = 0` — força o processador a recarregar todos os campos da VMCB do zero nesta primeira execução (nenhum campo é tratado como "limpo"/cacheado).

**Montagem do state save area (`vmcb->vmcb_ssa`):**

- `RIP = GuestPointer` — símbolo exportado pelo `.asm`, ponto de entrada do código que o guest executa (`CPUID` seguido de `LEA` e `HLT`).
- `RAX = 0`, `DR6 = 0`, `DR7 = 0` — estado inicial limpo dos registradores de debug e RAX (RAX é o leaf usado implicitamente por `CPUID` quando não setado explicitamente antes da entrada, então é zerado por previsibilidade).
- `RSP = guest_stack + PAGE_SIZE - 8` — topo da pilha do guest, alinhado a 8 bytes abaixo do fim da página.
- `CR0`, `CR2`, `CR3`, `CR4` — copiados diretamente dos registradores atuais do host via `__readcrN()`. O guest herda o mesmo espaço de paginação e o mesmo modo de operação (long mode) do host; não há isolamento de memória entre host e guest neste experimento.
- Segmento de código (`cs`): seletor e atributos lidos da CPU atual via `ReadCS`/`ReadCSAttrib`, convertidos com `ConvertAttrib`, com os seguintes ajustes de bits sobre o atributo:
  - `|= (1 << 13)` — seta o bit L (long mode) do descritor de código.
  - `&= ~(1 << 14)` — limpa o bit D/B (tamanho de operando default), coerente com um segmento de código de 64 bits.
  - `|= (1 << 15)` — seta o bit G (granularidade), com `limit = 0xFFFFFFFF` cobrindo todo o espaço de endereçamento em unidades de página.
  - `|= (1 << 11)` — marca o segmento como executável (bit de tipo).
- Segmento `tr` (task register) — programado com valores fixos (`seletor = 0x40`, `attrib = 0x8B`, `limit = 0x67`, `base = 0`) em vez de lidos da CPU atual; são valores compatíveis com uma TSS de 64 bits válida o suficiente para a entrada na VM não falhar por segmento de tarefa inválido.
- `CPL = 0` — guest roda em anel 0.
- `RFLAGS` — copiado de `__readeflags()` com o bit 8 (`TF`, trap flag) explicitamente limpo, para o guest não entrar em single-step.
- `efer` — montado a partir do EFER atual do host, com os bits `SVME` (12), `LME` (8) e `LMA` (10) forçados a 1, garantindo que o guest também rode em long mode com SVM habilitado em seu próprio EFER virtual.

**VMCB runtime e execução:**

Após montar `vmcb`, o driver aloca uma segunda VMCB (`vmcb_runtime`), zera e copia o conteúdo de `vmcb` para ela via `RtlCopyMemory`. É o endereço físico de `vmcb_runtime` que é passado para `AsmGuestResume`.

O bloco comentado no código (`__svm_clgi/__svm_vmsave/__svm_vmrun/__svm_vmload/__svm_stgi`) representa uma primeira abordagem via intrínsecos do compilador, substituída pela chamada à rotina assembly dedicada `AsmGuestResume`, que executa a mesma sequência de instruções privilegiadas manualmente e adiciona a lógica de troca de contexto com `guestCTX`.

A primeira chamada a `AsmGuestResume` executa o guest até o primeiro VM exit (a instrução `CPUID`, que está sempre interceptada pelo hardware SVM independentemente de configuração). Após o retorno:

```c
vmcb_runtime->vmcb_ssa.RIP = vmcb_runtime->vmcb_ctrl.nRip;
ctx->RCX = (UINT64)0xDEADBEEF;
```

O RIP do guest é avançado para `nRip` (fornecido pelo processador como o RIP imediatamente após a instrução que causou o exit — recurso de decode assist do SVM), e um valor sentinela é escrito em `ctx->RCX` apenas para fins de verificação/depuração do fluxo de dados entre host e guest.

A segunda chamada a `AsmGuestResume` retoma o guest a partir do RIP atualizado, que agora executa `LEA r9, Message` seguido de `HLT`. O `HLT` gera um segundo VM exit; a rotina assembly, nesse retorno, copia o ponteiro de `Message` (deixado pelo guest em R9) para `ctx->Message`.

Ao final, o driver imprime a mensagem via `KdPrint` e retorna `STATUS_SUCCESS`.

### 5.5 `NtVisorDeviceControlHandle`

Dispatch de IOCTL. Único ponto de entrada para `NTVISOR_ENABLE` e `NTVISOR_VMRUN`, conforme descrito na seção 4. Qualquer outro código de controle cai no `default`, loga via `KdPrint` e retorna `STATUS_INVALID_DEVICE_REQUEST` (valor inicial de `status`, não alterado no `default`).

### 5.6 `NtVisorCreateOrCloseHandle`

Handler de `IRP_MJ_CREATE`/`IRP_MJ_CLOSE`. Não faz nenhuma validação de acesso; apenas loga e completa o IRP com sucesso.

### 5.7 `NtVisorUnload`

Libera, em ordem, `vm_hsave_area`, `vmcb`, `guest_stack`, `ctx` e `vmcb_runtime`, cada um com a função de liberação correspondente ao alocador usado (`MmFreeContiguousMemory` para as áreas alocadas com `MmAllocateContiguousMemory`, `MmFreeContiguousMemorySpecifyCache` para as alocadas com `MmAllocateContiguousMemorySpecifyCache`). Todos os ponteiros globais são zerados ao final. Remove o link simbólico e o `DEVICE_OBJECT`.

Ponto de atenção: `NtVisorUnload` não reverte `EFER.SVME` nem a afinidade de thread fixada por `SVM_CHECK`. O estado de SVM na CPU permanece habilitado após o descarregamento do driver.

### 5.8 `DriverEntry`

Cria o dispositivo (`FILE_DEVICE_UNKNOWN`, `FILE_DEVICE_SECURE_OPEN`), cria o link simbólico, registra os três dispatch routines (`CREATE`, `CLOSE`, `DEVICE_CONTROL`) e `DriverUnload`. Em caso de falha em qualquer etapa, desfaz o que já foi criado antes de retornar o erro.

---

## 6. svm_resources.h

### 6.1 MSRs relevantes

```c
#define VMCR_ADDR       0xC0010114
#define EFER_ADDR       0xC0000080
#define VM_HSAVE_PA_MSR 0xC0010117
```

**`vmcr_msr` (VM_CR, `0xC0010114`)** — união de 64 bits mapeada com bitfields:

| Campo | Bit(s) | Significado |
|---|---|---|
| `DPD` | 0 | Debug port disable |
| `R_INIT` | 1 | Intercepta INIT |
| `DIS_A20M` | 2 | Desabilita mascaramento de A20 |
| `LOCK` | 3 | Trava os bits de VM_CR contra escrita |
| `SVMDIS` | 4 | SVM desabilitado permanentemente (via BIOS) |
| `Reserved` | 5–63 | Reservado |

**`efer_msr` (EFER, `0xC0000080`)** — união de 64 bits:

| Campo | Bit(s) | Significado |
|---|---|---|
| `Reserved` | 0–11 | Reservado (inclui LME/LMA, não modelados nesta união) |
| `SVME` | 12 | Habilita instruções SVM |
| `Reserved2` | 13–63 | Reservado |

Nota: a união `efer_msr` só modela explicitamente o bit `SVME`; os bits `LME` (8) e `LMA` (10), usados na montagem do EFER virtual do guest em `SVM_VMRUN`, são manipulados diretamente por máscara (`guest_efer |= (1ULL << 8)` etc.) sem passar por essa união.

### 6.2 Códigos de status customizados

```c
#define SVM_SVMDIS_ERROR             ((NTSTATUS)0xC0000001L)
#define SVM_SVME_DISABLE             ((NTSTATUS)0xC0000002L)
#define SVM_NOT_SUPPORTED            ((NTSTATUS)0xC0000003L)
#define NTVISOR_CHECK_STATUS_SUCCESS ((NTSTATUS)0xC0000004L)
```

Códigos arbitrários na faixa de erro/customizado do NTSTATUS, usados exclusivamente para comunicar o resultado de `SVM_CHECK` entre o driver e ele mesmo (via `GStatus`) e como valor de retorno do IOCTL `NTVISOR_ENABLE`.

### 6.3 Estruturas da VMCB

Todas empacotadas com `#pragma pack(push, 1)` para refletir exatamente o layout de memória exigido pelo hardware (a VMCB é lida e escrita diretamente pelo processador durante `VMRUN`).

**`vmcb_segment_t`** — descreve um registrador de segmento dentro da VMCB:

```c
typedef struct
{
    USHORT   seletor;
    USHORT   attrib;
    ULONG    limit;
    ULONG64  base;
} vmcb_segment_t;
```

**`_vmcb_control_area`** — área de controle da VMCB (offset 0x000–0x3FF da página). Contém, entre outros:

- Vetores de intercepção (`vectorInterrupt0`–`vectorInterrupt5`, `InterceptMisc2`) — bitmaps que definem quais eventos/instruções causam VM exit.
- `iopm_base_pa` / `msrpm_base_pa` — endereços físicos dos bitmaps de permissão de I/O e MSR (não utilizados neste driver; permanecem zerados, o que por padrão intercepta acesso conforme a configuração de intercepção geral).
- `GuestAsid` — identificador de espaço de endereço do guest, usado pelo TLB do processador para invalidação seletiva.
- `v_tpr`, `interrupt_shadow` — controle de APIC virtual e shadow de interrupção.
- `EXITCODE`, `EXITINFO1`, `EXITINFO2`, `EXITINFO` — preenchidos pelo processador após cada VM exit, descrevendo a causa e informações adicionais do exit.
- `np_enable` — habilita nested paging (não usado; paginação aninhada desativada, guest e host compartilham o mesmo espaço físico via `CR3` do host).
- `vmcb_clean` — bitmap de "campos limpos", usado pelo processador como otimização para evitar releitura de partes da VMCB não modificadas entre execuções.
- `nRip` — RIP seguinte à instrução que causou o VM exit (decode assist).
- Preenchida com reservados até fechar exatamente `0x400` bytes, validado em tempo de compilação por `static_assert(sizeof(_vmcb_control_area) == 0x400, ...)`.

**`_vmcb_state_save_area`** — área de estado salvo (offset 0x400 em diante). Contém os registradores de segmento (`es`, `cs`, `ss`, `ds`, `fs`, `gs`, `gdtr`, `ldtr`, `idtr`, `tr`), `CPL`, `efer`, os registradores de controle (`CR0`–`CR4`), `DR6`/`DR7`, `RFLAGS`, `RIP`, `RSP`, `RAX`, os MSRs de syscall (`STAR`, `LSTAR`, `CSTAR`, `SFMASK`, `KernelGsBase`), os MSRs de `SYSENTER`, `CR2`, `G_PAT`, `DBGCTL` e campos de last-branch-record. Tamanho validado em `0x2A0` bytes por `static_assert`.

**`_KVMCB` (`PKVMCB`)** — união da control area com a state save area, preenchida com reservado até completar exatamente uma página (`0x1000` bytes), conforme exigido pela arquitetura SVM (a VMCB deve ocupar uma página física alinhada). Validado por `static_assert(sizeof(_KVMCB) == 0x1000, ...)`.

**`guestCTX`** — estrutura de troca de dados entre a rotina assembly e o driver, sem relação com o formato da VMCB (é uma convenção própria do projeto, não uma estrutura definida pelo hardware):

```c
typedef struct
{
    UINT64 RAX; // 00h
    UINT64 RBX; // 08h
    UINT64 RCX; // 10h
    UINT64 RDX; // 18h
    UINT64 R8;  // 20h
    CHAR *Message; // 28h
} guestCTX;
```

Os offsets em comentário (`00h`–`28h`) são referenciados diretamente pelo `.asm` via `[r8+10h]` e `[r8+28h]`, portanto qualquer alteração na ordem ou no tamanho dos campos desta estrutura exige atualização manual e sincronizada da rotina assembly correspondente.

### 6.4 Protótipos externos

```c
void GuestPointer(void);
void GuestRestore(PVOID GuestVMCB);
extern void _sgdt(PVOID destination);

UINT16 ReadCS(void);
UINT32 ReadCSAttrib(void);
UINT16 ReadSS(void);
UINT32 ReadSSAttrib(void);
UINT16 ReadES(void);
UINT32 ReadESAttrib(void);
UINT16 ReadDS(void);
UINT32 ReadDSAttrib(void);
VOID   ReadGDTR(UINT8* buffer);
VOID   ReadIDTR(UINT8* buffer);
extern void AsmGuestResume(PVOID GuestCTX, PVOID GuestVMCB);
```

Apenas `GuestPointer`, `ReadCS`, `ReadCSAttrib` e `AsmGuestResume` são efetivamente usados por `NtVisor.c` no fluxo atual. As demais rotinas (`ReadSS`, `ReadES`, `ReadDS` e seus atributos, `ReadGDTR`, `ReadIDTR`, `_sgdt`, `GuestRestore`) estão implementadas no `.asm` mas não são chamadas — utilitários remanescentes de etapas anteriores do experimento, mantidos para uso futuro (montagem completa dos demais segmentos e das tabelas GDT/IDT do guest).

---

## 7. Guest.asm

Escrito em MASM (sintaxe Intel, x64). Contém as rotinas privilegiadas de SVM e as rotinas auxiliares de leitura de estado da CPU.

### 7.1 Dado estático

```asm
.DATA
    Message db "Hello from Guest Mode",0
.CODE
```

String usada pelo guest como prova de execução bem-sucedida dentro da VM. É carregada pelo próprio código do guest (`GuestPointer`) e seu endereço é repassado ao host através de `guestCTX.Message`.

### 7.2 `GuestRestore`

```asm
GuestRestore PROC
    clgi
    mov rax, rcx
    vmrun rax
    stgi
GuestRestore ENDP
```

Versão simplificada, sem `VMSAVE`/`VMLOAD` e sem gerenciamento de contexto — recebe o endereço físico da VMCB em RCX (convenção de chamada x64 do Windows), executa `CLGI` (desabilita interrupções globalmente antes de entrar na VM), `VMRUN` e `STGI` (reabilita interrupções após o retorno). Não é chamada pelo driver atual; corresponde a uma versão anterior/mínima de `AsmGuestResume`, mantida no código.

### 7.3 `AsmGuestResume`

Rotina efetivamente usada pelo driver. Assinatura em C: `void AsmGuestResume(PVOID GuestCTX, PVOID GuestVMCB)` — RCX recebe `GuestCTX`, RDX recebe o endereço físico da VMCB.

```asm
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
    mov qword ptr [r8+28h], r9  ; Move a mensagem para dentro de r8
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
```

Fluxo:

1. Salva todos os registradores de propósito geral que serão tocados durante a execução (incluindo os non-volatile RBX, RSI, RDI, RBP, R12–R15, exigidos pela convenção de chamada x64 do Windows, e os volatile usados internamente).
2. Copia o ponteiro de contexto (`RCX`) para `R8`, liberando RCX para uso posterior.
3. `sub rsp, 8` — ajuste de alinhamento de pilha antes de `VMRUN` (a entrada na VM não deve ocorrer com a pilha do host desalinhada).
4. Copia o endereço físico da VMCB (`RDX`) para `RAX`, registrador exigido pelas instruções `VMSAVE`/`VMRUN`/`VMLOAD` (que tomam o endereço físico da VMCB implicitamente em RAX).
5. `CLGI` → `VMSAVE rax` (salva o estado atual do host na VMCB) → `VMRUN rax` (entra na VM; a execução do host para aqui até o próximo VM exit) → `VMLOAD rax` (restaura o estado do host salvo por VMSAVE) → `STGI` (reabilita interrupções).
6. Após o retorno de `VMRUN` (ou seja, após o VM exit): `mov rcx, qword ptr [r8+10h]` lê o campo `RCX` do `guestCTX` (offset `0x10`) — comentado como "CPUID BIT", relacionado ao uso de `CPUID` pelo guest antes deste ponto, embora o valor lido não seja usado no restante da rotina (a instrução seguinte imediatamente sobrescreve o uso de RCX ao fazer `pop rcx`).
7. `mov qword ptr [r8+28h], r9` — grava o conteúdo de R9 no campo `Message` do `guestCTX` (offset `0x28`). É o mecanismo pelo qual o ponteiro carregado pelo guest via `lea r9, Message` chega até a estrutura lida pelo driver em `NtVisor.c`. Esse valor só é significativo depois do segundo VM exit (o causado por `HLT`), já que R9 só é setado pelo guest na segunda metade de sua execução.
8. `add rsp, 8` desfaz o ajuste de alinhamento e a sequência de `pop` restaura os registradores na ordem inversa da pilha.

Ponto de atenção: a leitura em `[r8+10h]` (campo `RCX` de `guestCTX`) ocorre incondicionalmente em toda chamada de `AsmGuestResume`, inclusive na primeira, antes do guest ter escrito qualquer coisa relevante em RCX — nesse ponto o valor lido é lixo ou o zero de inicialização de `RtlZeroMemory`. O valor só passa a ter significado na segunda chamada, após `ctx->RCX` ser setado para `0xDEADBEEF` em `NtVisor.c`.

### 7.4 `GuestPointer`

```asm
GuestPointer PROC
    cpuid
    lea r9, Message
    hlt
GuestPointer ENDP
```

Código executado pelo guest. É o valor atribuído a `vmcb->vmcb_ssa.RIP` antes da primeira execução. Sequência:

1. `CPUID` — instrução sempre interceptada por hardware quando a VM está em execução sob SVM (independentemente de bits de intercepção configurados), o que garante o primeiro VM exit previsível usado pelo driver para atualizar RIP via `nRip`.
2. `LEA R9, Message` — carrega o endereço da string estática no registrador R9. Só é alcançada após o driver reposicionar RIP do guest para depois da instrução `CPUID` (via `nRip`) e retomar a execução na segunda chamada de `AsmGuestResume`.
3. `HLT` — pára a execução do guest e gera o segundo VM exit, que a rotina assembly usa como ponto para extrair R9 e preencher `guestCTX.Message`.

### 7.5 Rotinas de leitura de estado da CPU

```asm
ReadCS PROC
    mov ax, cs
    ret
ReadCS ENDP

ReadCSAttrib PROC
    mov ax, cs
    lar eax, eax
    ret
ReadCSAttrib ENDP
```

`ReadCS` retorna o seletor de CS atual do host (usado diretamente como seletor de CS do guest em `SVM_VMRUN`). `ReadCSAttrib` executa `LAR` (Load Access Rights) sobre o seletor de CS, retornando em EAX os bits de acesso do descritor correspondente na GDT/LDT — usado como base para o atributo de segmento de código do guest, antes de passar por `ConvertAttrib` e pelos ajustes de bits (L, D/B, G, executável) em `NtVisor.c`.

As rotinas equivalentes para SS, ES e DS (`ReadSS`, `ReadSSAttrib`, `ReadES`, `ReadESAttrib`, `ReadDS`, `ReadDSAttrib`) seguem exatamente o mesmo padrão, mas não são chamadas pelo driver no fluxo atual — apenas CS é efetivamente configurado na state save area do guest; os demais segmentos permanecem zerados por `RtlZeroMemory`.

### 7.6 `ReadGDTR` / `ReadIDTR` / `_sgdt`

```asm
ReadGDTR PROC
    sgdt fword ptr [rcx]
    ret
ReadGDTR ENDP

ReadIDTR PROC
    sidt fword ptr [rcx]
    ret
ReadIDTR ENDP

_sgdt PROC
    sgdt fword ptr [rcx]
    ret
_sgdt ENDP
```

`ReadGDTR` e `_sgdt` são funcionalmente idênticas (ambas executam `SGDT` sobre o buffer apontado por RCX); `_sgdt` aparenta ser a primeira versão, mantida como protótipo separado em `svm_resources.h`. `ReadIDTR` executa `SIDT` sobre o mesmo padrão. Nenhuma das três é chamada pelo driver — não há, no fluxo atual, configuração de GDTR/IDTR do guest na VMCB (`gdtr` e `idtr` em `_vmcb_state_save_area` permanecem zerados).

---

## 8. NtVisorClient.c — cliente em user-mode

### 8.1 Constantes

```c
#define NTVISOR_DEVICE_PATH L"\\\\.\\NtVisorSymbolName"
#define NTVISOR_ENABLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2080, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NTVISOR_VMRUN  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2081, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NTVISOR_MESSAGE_SIZE 64
```

Os códigos de IOCTL são duplicados aqui em vez de compartilhados via header comum com o driver — qualquer mudança nos códigos definidos em `NtVisor.c` precisa ser replicada manualmente neste arquivo.

### 8.2 `NtVisorOpenDevice` / `NtVisorCloseDevice`

Encapsulam `CreateFileW` sobre `\\.\NtVisorSymbolName` (que resolve, via Win32, para o link simbólico `\??\NtVisorSymbolName` criado pelo driver) e `CloseHandle`, com log de erro via `NtVisorPrintError` em caso de falha.

### 8.3 `NtVisorPrintError`

Imprime o valor de `GetLastError()` em decimal e hexadecimal, prefixado pela operação que falhou. Único mecanismo de diagnóstico de erro do cliente.

### 8.4 `NtVisorEnable`

Envia `NTVISOR_ENABLE` sem buffers de entrada ou saída. O sucesso ou falha da chamada `DeviceIoControl` é o único indicador de resultado — o driver não retorna dados adicionais para este IOCTL, apenas o `NTSTATUS` traduzido em código de erro Win32 pelo próprio `DeviceIoControl`.

### 8.5 `NtVisorVmrun`

Valida os parâmetros de saída, zera o buffer, envia `NTVISOR_VMRUN` e recebe a mensagem do kernel no buffer de saída. Trata dois casos de falha distintos:

- `DeviceIoControl` retorna `FALSE` — falha na própria chamada.
- `DeviceIoControl` retorna `TRUE` mas `*BytesReturned == 0` — IOCTL concluído porém sem mensagem válida (situação que, dado o driver atual, não deveria ocorrer se `NTVISOR_ENABLE` teve sucesso antes).

Garante terminação nula do buffer (`Message[MessageSize - 1] = '\0'`) antes de repassar ao chamador, como proteção contra uma string não terminada vinda do kernel.

### 8.6 `main`

1. `SetConsoleOutputCP(65001)` — configura o console para UTF-8, por conta dos caracteres acentuados usados nas mensagens de log.
2. Abre o dispositivo.
3. Executa `NtVisorEnable`; aborta com código 1 em caso de falha.
4. Executa `NtVisorVmrun`; aborta com código 1 em caso de falha.
5. Imprime a mensagem retornada dentro de uma caixa ASCII formatada.
6. Fecha o dispositivo e retorna 0.

Não há tratamento para o driver não estar carregado além do erro genérico de `CreateFileW` (tipicamente `ERROR_FILE_NOT_FOUND`, reportado via `NtVisorPrintError`).

---

## 9. Pontos de atenção conhecidos

- **Sem nested paging.** `np_enable` não é setado; o guest usa o mesmo `CR3` do host, portanto não há isolamento de espaço de endereço entre guest e host neste experimento — qualquer acesso a memória do guest é, na prática, acesso direto à memória do host.
- **Sem restauração de EFER.SVME.** `SVM_CHECK` habilita SVM na CPU e nunca reverte esse estado, nem em `NtVisorUnload` nem em nenhum outro ponto do driver.
- **Afinidade de thread fixada permanentemente no caminho de sucesso.** `KeSetSystemAffinityThreadEx` em `SVM_CHECK` só é revertida nos ramos de erro.
- **Duplicação de definição de IOCTL** entre `NtVisor.c` e `NtVisorClient.c`, sem header compartilhado.
- **Leitura antecipada de `guestCTX.RCX`** em `AsmGuestResume`, antes de o guest ter escrito qualquer valor significativo nele na primeira chamada (ver seção 7.3).
- **Rotinas não utilizadas** mantidas tanto no `.h` quanto no `.asm` (`ReadSS`/`ReadES`/`ReadDS` e atributos, `ReadGDTR`, `ReadIDTR`, `_sgdt`, `GuestRestore`) — não afetam o comportamento atual, mas indicam superfície para extensão futura (montagem completa dos demais segmentos e tabelas de descritores do guest).
- **`vm_hsave_area` alocada com 4 páginas** quando a arquitetura exige apenas uma página para a host save area.

---

## 10. Referências

- AMD64 Architecture Programmer's Manual, Volume 2 — System Programming, Capítulo 15 (Secure Virtual Machine).
- AMD64 Architecture Programmer's Manual, Volume 2 — Seção 15.5.1 (Basic Operation / VMRUN).
- AMD64 Architecture Programmer's Manual, Volume 2 — Seção 15.4 (Enabling SVM).
