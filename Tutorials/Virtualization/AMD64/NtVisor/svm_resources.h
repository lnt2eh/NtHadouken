#ifndef SVM_RESOURCES_H
#define SVM_RESOURCES_H

/* ======================================================================== *
 *                                                                          *
 *                      Project: NtHadouken                                 *
 *                                                                          *
 *                     Header: svm_resources.h                              *
 *                                                                          *
 *   Definições relacionadas à extensão AMD-V (SVM): estruturas da VMCB,    *
 *   endereços/índices de MSR, campos de intercepção, layout de save area   *
 *   de guest/host e constantes auxiliares usadas pelo hypervisor.          *
 *                                                                          *
 *   Autor / Mantenedor : Matheus Santos (_int2Eh)                          *
 *   Propósito          : Apresentação e pesquisa acadêmica                 *
 *   Referência         : AMD64 Architecture Programmer's Manual, Vol. 2    *
 *                         (SVM Architecture Reference Manual, doc. 24593)  *
 *   Aviso              : Não destinado a uso em produção                   *
 *                                                                          *
 * ======================================================================== */

#include <ntddk.h>

#define BIT(n) (1ULL << n)

 // MSRS bits

	 // VM_CR
typedef union
{
	UINT64 Value;
	struct
	{
		UINT64 DPD : 1;
		UINT64 R_INIT : 1;
		UINT64 DIS_A20M : 1;
		UINT64 LOCK : 1;
		UINT64 SVMDIS : 1;
		UINT64 Reserved : 59;
	}vmcr_s;
}vmcr_msr;

//EFER
typedef union
{
	UINT64 Value;
	struct
	{
		UINT64 Reserved : 12;
		UINT64 SVME : 1;
		UINT64 Reserved2 : 51;
	}efer_s;
}efer_msr;

// MSRS address

#define VMCR_ADDR 0xC0010114
#define EFER_ADDR 0xC0000080
#define VM_HSAVE_PA_MSR 0xC0010117

// SVM ERROS
#define SVM_SVMDIS_ERROR ((NTSTATUS)0xC0000001L)
#define SVM_SVME_DISABLE ((NTSTATUS)0xC0000002L)
#define SVM_NOT_SUPPORTED ((NTSTATUS)0xC0000003L)

//NtVisor Status
#define NTVISOR_CHECK_STATUS_SUCCESS ((NTSTATUS)0xC0000004L)


// VMCB
#pragma pack(push, 1)

typedef struct
{
	USHORT   seletor;
	USHORT   attrib;
	ULONG    limit;
	ULONG64  base;
} vmcb_segment_t;


typedef struct
{
	UINT16   vectorInterrupt0;
	UINT16   vectorInterrupt1;
	UINT16   vectorInterrupt2;
	UINT16   vectorInterrupt3;
	UINT32   vectorInterrupt4;
	UINT32   vectorInterrupt5;
	UINT32   InterceptMisc2;
	UINT8    Reserverd1[0x03c - 0x014];
	UINT16   pause_fth;
	UINT16   pause_fc;
	UINT64   iopm_base_pa;
	UINT64   msrpm_base_pa;
	UINT64   tsc_offset;
	UINT32   GuestAsid;
	UINT32   UPDATE_IRR;
	UINT64   v_tpr;
	UINT64   interrupt_shadow;
	UINT64   EXITCODE;
	UINT64   EXITINFO1;
	UINT64   EXITINFO2;
	UINT64   EXITINFO;
	UINT64   np_enable;
	UINT64   avic_apic_bar;
	UINT64   ghcb;
	UINT64   eventinj;
	UINT64   n_cr3;
	UINT64   lbr_virtualization_enable;
	UINT64   vmcb_clean;
	UINT64   nRip;
	UINT8    NumOfBytesFetched;
	UINT8    requested_irr[15];
	UINT64   avic_apic_backing_page;
	UINT64   Reserved2;
	UINT64   avic_logical_table;
	UINT64   avic_physical_max_index;
	UINT64   Reserved3;
	UINT64   VMSA;
	UINT8    Reserved4[0x3e0 - 0x110];
	UINT8    Reserved7[0x400 - 0x3e0];
} _vmcb_control_area;

static_assert(sizeof(_vmcb_control_area) == 0x400,
	"VMCB_CONTROL_AREA Size Mismatch");


typedef struct
{
	vmcb_segment_t es;
	vmcb_segment_t cs;
	vmcb_segment_t ss;
	vmcb_segment_t ds;
	vmcb_segment_t fs;
	vmcb_segment_t gs;
	vmcb_segment_t gdtr;
	vmcb_segment_t ldtr;
	vmcb_segment_t idtr;
	vmcb_segment_t tr;
	UINT8    Reserved[0x0cb - 0x0a0];
	UINT8    CPL;
	UINT32   Reserved2;
	UINT64   efer;
	UINT8    Reserved3[0x148 - 0x0d8];
	UINT64   CR4;
	UINT64   CR3;
	UINT64   CR0;
	UINT64   DR7;
	UINT64   DR6;
	UINT64   RFLAGS;
	UINT64   RIP;
	UINT8    Reserved4[0x1d8 - 0x180];
	UINT64   RSP;
	UINT8    Reserved5[0x1f8 - 0x1e0];
	UINT64   RAX;
	UINT64   STAR;
	UINT64   LSTAR;
	UINT64   CSTAR;
	UINT64   SFMASK;
	UINT64   KernelGsBase;
	UINT64   SYSENTER_CS;
	UINT64   SYSENTER_ESP;
	UINT64   SYSENTER_EIP;
	UINT64   CR2;
	UINT8    Reserved6[0x268 - 0x248];
	UINT64   G_PAT;
	UINT64   DBGCTL;
	UINT64   BR_FROM;
	UINT64   BR_TO;
	UINT64   LASTEXCPFROM;
	UINT64   LASTEXCPTO;
	UINT64   IC_IBS_EXTD_CTL;
} _vmcb_state_save_area;

static_assert(sizeof(_vmcb_state_save_area) == 0x2a0,
	"VMCB_STATE_SAVE_AREA Size Mismatch");


typedef struct
{
	_vmcb_control_area vmcb_ctrl;
	_vmcb_state_save_area vmcb_ssa;
	UINT8 Reserved1[0x1000 - sizeof(_vmcb_control_area) - sizeof(_vmcb_state_save_area)];
} _KVMCB, * PKVMCB;

static_assert(sizeof(_KVMCB) == 0x1000,
	"VMCB Size Mismatch");


typedef struct
{
	UINT64 RAX; // 00h
	UINT64 RBX; // 08h
	UINT64 RCX; // 10h
	UINT64 RDX; // 18
	UINT64 R8; // 20h
	CHAR *Message; //28h
} guestCTX;

#pragma pack(pop)

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

#endif