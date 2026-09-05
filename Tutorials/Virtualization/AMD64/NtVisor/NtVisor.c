/* ======================================================================== *
 *                                                                          *
 *                      Project: NtHadouken                                 *
 *                                                                          *
 *                  Windows Kernel Research Driver                          *
 *                                                                          *
 *   Author / Maintainer : Matheus Santos (_int2Eh)                         *
 *   Purpose             : Academic Research and Experimentation            *
 *   Research            : AMD SVM VMRUN: Intrinsic Abstraction and         *
 *                         Architectural Control                            *
 *   Notice              : Not intended for production use                  *
 *                                                                          *
 * ======================================================================== */

#include <ntddk.h>
#include <intrin.h>
#include "svm_resources.h"

 //Memory Allocation
PKVMCB vmcb = NULL;
PKVMCB vmcb_runtime = NULL; 
PVOID vm_hsave_area = NULL;
PVOID guest_stack = NULL;
guestCTX* ctx = NULL;

// Maskings
#define BIT_SVME BIT(12)

// IOCTL
#define NTVISOR_ENABLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2080, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NTVISOR_VMRUN CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2081, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Global Function and Variables
UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\NtVisor");
UNICODE_STRING SymbolicName = RTL_CONSTANT_STRING(L"\\??\\NtVisorSymbolName");
NTSTATUS GStatus = STATUS_UNSUCCESSFUL; // GStatus(Global Status) é utilizada para definir se a checagem do ambiente para SVM obteve sucesso.

UINT16 ConvertAttrib(UINT32 rawAttrib)
{
	return (UINT16)(rawAttrib & 0xFFFF);
}

// Protótipos de funções
NTSTATUS NtVisorCreateOrCloseHandle(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp);

NTSTATUS NtVisorDeviceControlHandle(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp);

// SVM FUNCTIONS
NTSTATUS SVM_VMRUN(void)
{
	/*
	 * Initial SVM configuring.
	 *
	 * Reference:
	 * AMD64 Architecture Programmer's Manual,
	 * Volume 2 - 15.5.1 Basic Operation.
	 */

	PHYSICAL_ADDRESS LowestAcceptableAddress;
	LowestAcceptableAddress.QuadPart = 0x0000000000800000;

	PHYSICAL_ADDRESS HighestAcceptableAddress;
	HighestAcceptableAddress.QuadPart = 0x0000000000FFFFFF;

	PHYSICAL_ADDRESS vmcbPa;
	PHYSICAL_ADDRESS vm_hsave_pa;
	PHYSICAL_ADDRESS vmcbRuntimePa;

	vmcb = (PKVMCB)MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, LowestAcceptableAddress, HighestAcceptableAddress, LowestAcceptableAddress, MmCached);
	if (vmcb == NULL)
	{
		KdPrint(("Não foi possível alocar memória para o VMCB.\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	vm_hsave_area = MmAllocateContiguousMemory(PAGE_SIZE * 4, HighestAcceptableAddress);
	if (vm_hsave_area == NULL)
	{
		KdPrint(("Não foi possível alocar memória para o VM_HSAVE_PA.\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	guest_stack = MmAllocateContiguousMemory(PAGE_SIZE, HighestAcceptableAddress);
	if (guest_stack == NULL)
	{
		KdPrint(("STACK GUEST FAILED\n"));
		return STATUS_UNSUCCESSFUL;
	}

	ctx = (guestCTX*)MmAllocateContiguousMemory(PAGE_SIZE, HighestAcceptableAddress);
	if (ctx == NULL)
	{
		KdPrint(("Non Allocated\n"));
		return STATUS_UNSUCCESSFUL;
	}
	RtlZeroMemory(ctx, PAGE_SIZE);

	// Importante zerar a área de memória alocada para evitar lixo e possívelmente crash após VMRUN.
	RtlZeroMemory(vmcb, PAGE_SIZE);
	RtlZeroMemory(vm_hsave_area, PAGE_SIZE);

	vmcbPa = MmGetPhysicalAddress((PVOID)vmcb);
	vm_hsave_pa = MmGetPhysicalAddress(vm_hsave_area);
	RtlZeroMemory(guest_stack, PAGE_SIZE);

	__writemsr(VM_HSAVE_PA_MSR, vm_hsave_pa.QuadPart);

	KdPrint(("VM_HSAVE: %llx\n", vm_hsave_area));

	// Configuração básica do VMCB
	vmcb->vmcb_ctrl.GuestAsid = 1;
	vmcb->vmcb_ctrl.InterceptMisc2 = (1 << 0) | (1 << 1);
	vmcb->vmcb_ctrl.vectorInterrupt5 = (1 << 3) | (1 << 18) | (1 << 24);
	vmcb->vmcb_ctrl.vectorInterrupt4 = (1 << 3);

	vmcb->vmcb_ctrl.v_tpr = 0;
	vmcb->vmcb_ctrl.interrupt_shadow = 0;

	vmcb->vmcb_ssa.RIP = (UINT64)GuestPointer;
	vmcb->vmcb_ssa.RAX = 0;
	vmcb->vmcb_ssa.DR6 = 0;
	vmcb->vmcb_ssa.DR7 = 0;
	vmcb->vmcb_ctrl.vmcb_clean = 0;

	vmcb->vmcb_ssa.RSP = (ULONG64)guest_stack + PAGE_SIZE - 8;
	vmcb->vmcb_ssa.CR0 = __readcr0();
	vmcb->vmcb_ssa.CR2 = __readcr2();
	vmcb->vmcb_ssa.CR3 = __readcr3();
	vmcb->vmcb_ssa.CR4 = __readcr4();

	UINT32 rawCsAttrib = ReadCSAttrib();
	UINT16 csAttrib = ConvertAttrib(rawCsAttrib);
	csAttrib |= (1 << 13);
	csAttrib &= ~(1 << 14);
	csAttrib |= (1 << 15);
	csAttrib |= (1 << 11);

	vmcb->vmcb_ssa.cs.seletor = ReadCS();
	vmcb->vmcb_ssa.cs.attrib = csAttrib;
	vmcb->vmcb_ssa.cs.limit = 0xFFFFFFFF;
	vmcb->vmcb_ssa.cs.base = 0;

	vmcb->vmcb_ssa.tr.seletor = 0x40;
	vmcb->vmcb_ssa.tr.attrib = 0x8B;
	vmcb->vmcb_ssa.tr.limit = 0x67;
	vmcb->vmcb_ssa.tr.base = 0;

	vmcb->vmcb_ssa.CPL = 0;
	vmcb->vmcb_ssa.RFLAGS = __readeflags() & ~0x100ULL;

	ULONG64 guest_efer = __readmsr(EFER_ADDR);
	guest_efer |= (1ULL << 12);  // SVME
	guest_efer |= (1ULL << 8);   // LME
	guest_efer |= (1ULL << 10);  // LMA
	vmcb->vmcb_ssa.efer = guest_efer;

	vmcb_runtime = (PKVMCB)MmAllocateContiguousMemorySpecifyCache(
		PAGE_SIZE,
		LowestAcceptableAddress,
		HighestAcceptableAddress,
		LowestAcceptableAddress,
		MmCached);

	if (vmcb_runtime == NULL)
	{
		KdPrint(("Não foi possível alocar o VMCB runtime.\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(vmcb_runtime, PAGE_SIZE);

	RtlCopyMemory(
		vmcb_runtime,
		vmcb,
		PAGE_SIZE);

	vmcbRuntimePa = MmGetPhysicalAddress((PVOID)vmcb_runtime);

	KdPrint(("NtVisor: entering VMRUN\n"));

	/*
	__svm_clgi();
	__svm_vmsave((SIZE_T)vmcbRuntimePa.QuadPart);
	__svm_vmrun((SIZE_T)vmcbRuntimePa.QuadPart);
	__svm_vmload((SIZE_T)vmcbRuntimePa.QuadPart);
	__svm_stgi();

	KdPrint(("NtVisor: returned from VMRUN\n"));
	KdPrint(("NtVisor: EXITCODE = 0x%I64X\n",
		vmcb_runtime->vmcb_ctrl.EXITCODE));
	*/
	
	AsmGuestResume((PVOID)ctx, (PVOID)vmcbRuntimePa.QuadPart);
	KdPrint(("NtVisor: returned from VMRUN\n"));
	KdPrint(("NtVisor: EXITCODE = 0x%I64X\n",
		vmcb_runtime->vmcb_ctrl.EXITCODE));

	vmcb_runtime->vmcb_ssa.RIP = vmcb_runtime->vmcb_ctrl.nRip;
	ctx->RCX = (UINT64)0xDEADBEEF;

	AsmGuestResume((PVOID)ctx, (PVOID)vmcbRuntimePa.QuadPart);

	KdPrint(("NtVisor: returned from VMRUN\n"));
	KdPrint(("NtVisor: EXITCODE = 0x%I64X\n",
		vmcb_runtime->vmcb_ctrl.EXITCODE));

	KdPrint(("Mensagem From GuestMode: %s\n", ctx->Message));

	return STATUS_SUCCESS;
}

NTSTATUS SVM_CHECK(void)
{
	/*
	 * Initial SVM environment check.
	 *
	 * Reference:
	 * AMD64 Architecture Programmer's Manual,
	 * Volume 2 - Section 15.4: Enabling SVM.
	 */

	 // Checagem de suporte a SVM via CPUID antes de tocar em qualquer MSR relacionado.
	// Ler VMCR/EFER numa CPU sem suporte gera #GP.
	int cpuInfo[4] = { 0 };
	__cpuid(cpuInfo, 0x80000001);
	if (!(cpuInfo[2] & (1 << 2))) // ECX bit 2 = SVM
	{
		KdPrint(("NtVisor: CPU nao suporta SVM (CPUID 8000_0001h.ECX[2]=0)\n"));
		return SVM_NOT_SUPPORTED;
	}

	KAFFINITY oldAffinity = KeSetSystemAffinityThreadEx((KAFFINITY)1);

	// Estrutura relacionadas aos MSRs
	vmcr_msr vmcr = { 0 };
	vmcr.Value = (UINT64)__readmsr(VMCR_ADDR);

	efer_msr efer = { 0 };
	efer.Value = (UINT64)__readmsr(EFER_ADDR);

	if (vmcr.vmcr_s.SVMDIS == 0)
	{
		/*
		 * Enable SVM support in EFER.
		 *
		 * Sets EFER.SVME (bit 12) to 1, allowing the processor to execute
		 * SVM instructions (VMRUN, VMLOAD, VMSAVE, CLGI, STGI). All other
		 * bits are preserved as read from the register.
		 *
		 * Reference:
		 * AMD64 Architecture Programmer's Manual,
		 * Volume 2 - Section 15.4: Enabling SVM.
		 */

		KdPrint(("SVM Enabled\n"));
		efer.Value |= BIT_SVME;
		__writemsr(EFER_ADDR, efer.Value); // Reescreve EFER com o bit 12 ativo.

		if (efer.efer_s.SVME == 1)
		{
			return NTVISOR_CHECK_STATUS_SUCCESS;
		}

		else
		{
			KeRevertToUserAffinityThreadEx(oldAffinity);
			return SVM_SVME_DISABLE;
		}
	}

	else
	{
		KdPrint(("SVMDIS DISABLED\n"));
		KeRevertToUserAffinityThreadEx(oldAffinity);
		return SVM_SVMDIS_ERROR;
	}
}

// NtVisor Unload function
VOID NtVisorUnload(
	_In_ PDRIVER_OBJECT DriverObject)
{
	// Start Function NtVisorUnload

	UNREFERENCED_PARAMETER(DriverObject); // Necessário para evitar erros de compilação
	if (vm_hsave_area)  MmFreeContiguousMemory(vm_hsave_area);

	if (vmcb != NULL)
	{
		MmFreeContiguousMemorySpecifyCache((PVOID)vmcb, PAGE_SIZE, MmCached);
	}

	if (guest_stack != NULL)
	{
		MmFreeContiguousMemory(guest_stack);
	}

	if (ctx) MmFreeContiguousMemory(ctx);

	if (vmcb_runtime)
	{
		MmFreeContiguousMemorySpecifyCache(
			(PVOID)vmcb_runtime,
			PAGE_SIZE,
			MmCached);
	}
	vmcb_runtime = NULL;

	vmcb = NULL;
	vm_hsave_area = NULL;
	guest_stack = NULL;
	ctx = NULL;

	IoDeleteSymbolicLink(&SymbolicName);
	IoDeleteDevice(DriverObject->DeviceObject);
	KdPrint(("NtVisor Unloaded\n"));
}

// NtVisor Device Control function
NTSTATUS NtVisorDeviceControlHandle(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject); // Necessário para evitar erros de compilação
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST; // Definir status de retorno

	PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);
	switch (stackLocation->Parameters.DeviceIoControl.IoControlCode)
	{
		case NTVISOR_ENABLE:
		{
			GStatus = SVM_CHECK();
			status = GStatus;
			break;
		}
		case NTVISOR_VMRUN:
		{
			if(GStatus == NTVISOR_CHECK_STATUS_SUCCESS)
			{
				SVM_VMRUN();
				status = STATUS_SUCCESS;
				Irp->IoStatus.Information = (ULONG)strlen(ctx->Message) + 1;
				RtlCopyMemory(
					Irp->AssociatedIrp.SystemBuffer,
					ctx->Message,
					strlen(ctx->Message) + 1
				);
				break;
			}

			Irp->IoStatus.Information = 0;
			break;
		}
		default:
			KdPrint(("NtVisor: Operation Not SUPP\n"));
			break;
	}

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// Create Or Close Device function
NTSTATUS NtVisorCreateOrCloseHandle(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject); // Necessário para evitar erros de compilação
	PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);
	switch (stackLocation->MajorFunction)
	{
	case IRP_MJ_CREATE:
		KdPrint(("Dispositivo Aberto\n"));
		break;
	case IRP_MJ_CLOSE:
		KdPrint(("Dispositivo Fechado\n"));
		break;
	}

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


// DriverEntry Function
NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath); // Necessário para evitar erros de compilação

	// Start Function DriverEntry
	KdPrint(("NtVisor Loaded\n"));

	// Variables
	NTSTATUS status = 0;

	//Device
	status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DriverObject->DeviceObject);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Erro ao criar dispositivo. Verifique: 0x%08X\n", status));
		return status;
	}

	status = IoCreateSymbolicLink(&SymbolicName, &DeviceName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(DriverObject->DeviceObject);
		KdPrint(("Erro ao criar nome simbolico. Verifique: 0x%08X\n", status));
		return status;
	}

	// Driver Dispatch Routines
	DriverObject->MajorFunction[IRP_MJ_CREATE] = NtVisorCreateOrCloseHandle;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = NtVisorCreateOrCloseHandle;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = NtVisorDeviceControlHandle;
	DriverObject->DriverUnload = NtVisorUnload;
	return STATUS_SUCCESS;
}