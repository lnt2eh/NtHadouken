/*
 * ============================================================================
 *
 *                         Projeto: NtHadouken
 *
 *                    Cliente de Pesquisa em Kernel Windows
 *
 *  Autor / Mantenedor : Matheus Santos (_int2Eh)
 *  Objetivo           : Interface em user-mode para o NtVisor
 *  Pesquisa           : AMD SVM VMRUN: Abstração Intrínseca e
 *                        Controle Arquitetural
 *
 *  Aviso              : Não destinado ao uso em produção
 *
 * ============================================================================
 */

#include <Windows.h>
#include <stdio.h>

/*
 * O link simbólico é criado pelo driver através de:
 *
 *     \??\NtVisorSymbolName
 *
 * O Win32 expõe esse link através do namespace \\.\.
 */
#define NTVISOR_DEVICE_PATH L"\\\\.\\NtVisorSymbolName"

/*
 * METHOD_BUFFERED é utilizado pelo driver para transferir dados através
 * do SystemBuffer associado ao IRP.
 */
#define NTVISOR_ENABLE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2080, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define NTVISOR_VMRUN \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2081, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define NTVISOR_MESSAGE_SIZE 64

/* ------------------------------------------------------------------------ *
 *                               Protótipos                                  *
 * ------------------------------------------------------------------------ */

/**
 * Abre o dispositivo NtVisor exposto pelo driver.
 *
 * @return Um handle válido ou INVALID_HANDLE_VALUE em caso de falha.
 */
static HANDLE NtVisorOpenDevice(void);

/**
 * Solicita ao driver a verificação do ambiente SVM.
 *
 * Esta operação não retorna uma mensagem. O resultado é representado
 * pelo sucesso ou falha da chamada DeviceIoControl.
 *
 * @param DeviceHandle Handle retornado por NtVisorOpenDevice.
 *
 * @return TRUE quando o IOCTL é concluído com sucesso.
 */
static BOOL NtVisorEnable(
    _In_ HANDLE DeviceHandle);

/**
 * Solicita ao driver a execução do experimento VMRUN.
 *
 * A mensagem produzida pelo kernel é copiada para o buffer de saída
 * após a conclusão de SVM_VMRUN.
 *
 * @param DeviceHandle Handle retornado por NtVisorOpenDevice.
 * @param Message Buffer de saída que recebe a mensagem do kernel.
 * @param MessageSize Tamanho do buffer de saída em bytes.
 * @param BytesReturned Recebe a quantidade de bytes retornados pelo driver.
 *
 * @return TRUE quando o IOCTL é concluído e uma mensagem válida é retornada.
 */
static BOOL NtVisorVmrun(
    _In_ HANDLE DeviceHandle,
    _Out_writes_bytes_(MessageSize) CHAR* Message,
    _In_ DWORD MessageSize,
    _Out_ DWORD* BytesReturned);

/**
 * Exibe o último erro Win32 associado a uma operação.
 *
 * @param Operation Descrição da operação que falhou.
 */
static void NtVisorPrintError(
    _In_ const char* Operation);

/**
 * Fecha um handle do dispositivo NtVisor.
 *
 * @param DeviceHandle Handle retornado por NtVisorOpenDevice.
 */
static void NtVisorCloseDevice(
    _In_ HANDLE DeviceHandle);

/* ------------------------------------------------------------------------ *
 *                            Tratamento de erros                            *
 * ------------------------------------------------------------------------ */

static void NtVisorPrintError(
    _In_ const char* Operation)
{
    DWORD error = GetLastError();

    printf(
        "[-] %s falhou: %lu (0x%08lX)\n",
        Operation,
        error,
        error
    );
}

/* ------------------------------------------------------------------------ *
 *                            Gerenciamento do dispositivo                  *
 * ------------------------------------------------------------------------ */

static HANDLE NtVisorOpenDevice(void)
{
    HANDLE deviceHandle;

    printf("[*] Abrindo NtVisor...\n");

    deviceHandle = CreateFileW(
        NTVISOR_DEVICE_PATH,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        NtVisorPrintError("CreateFileW");
        return INVALID_HANDLE_VALUE;
    }

    printf("[+] Dispositivo NtVisor aberto com sucesso.\n");

    return deviceHandle;
}

static void NtVisorCloseDevice(
    _In_ HANDLE DeviceHandle)
{
    if (DeviceHandle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    if (!CloseHandle(DeviceHandle))
    {
        NtVisorPrintError("CloseHandle");
        return;
    }

    printf("[*] Dispositivo NtVisor fechado.\n");
}

/* ------------------------------------------------------------------------ *
 *                              Manipuladores de IOCTL                       *
 * ------------------------------------------------------------------------ */

static BOOL NtVisorEnable(
    _In_ HANDLE DeviceHandle)
{
    DWORD bytesReturned = 0;

    printf("[*] Solicitando verificação do ambiente SVM...\n");

    /*
     * NTVISOR_ENABLE executa apenas a verificação do ambiente SVM.
     *
     * Nenhum buffer de saída é necessário, pois este IOCTL não retorna
     * a mensagem produzida pelo experimento. A mensagem é retornada
     * exclusivamente pelo NTVISOR_VMRUN.
     */
    if (!DeviceIoControl(
        DeviceHandle,
        NTVISOR_ENABLE,
        NULL,
        0,
        NULL,
        0,
        &bytesReturned,
        NULL))
    {
        NtVisorPrintError("DeviceIoControl(NTVISOR_ENABLE)");
        return FALSE;
    }

    printf("[+] IOCTL de verificação SVM concluído com sucesso.\n");

    return TRUE;
}

static BOOL NtVisorVmrun(
    _In_ HANDLE DeviceHandle,
    _Out_writes_bytes_(MessageSize) CHAR* Message,
    _In_ DWORD MessageSize,
    _Out_ DWORD* BytesReturned)
{
    if (Message == NULL ||
        BytesReturned == NULL ||
        MessageSize == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        NtVisorPrintError("NtVisorVmrun");
        return FALSE;
    }

    ZeroMemory(Message, MessageSize);
    *BytesReturned = 0;

    printf("[*] Solicitando execução do VMRUN...\n");

    /*
     * NTVISOR_VMRUN executa o experimento de virtualização e retorna
     * a mensagem produzida pelo kernel através de METHOD_BUFFERED.
     */
    if (!DeviceIoControl(
        DeviceHandle,
        NTVISOR_VMRUN,
        NULL,
        0,
        Message,
        MessageSize,
        BytesReturned,
        NULL))
    {
        NtVisorPrintError("DeviceIoControl(NTVISOR_VMRUN)");
        return FALSE;
    }

    printf("[+] IOCTL de VMRUN concluído com sucesso.\n");

    /*
     * Um IOCTL concluído com sucesso, mas sem dados retornados,
     * não fornece uma mensagem válida para exibição.
     */
    if (*BytesReturned == 0)
    {
        printf("[-] O VMRUN foi concluído sem retornar uma mensagem.\n");
        return FALSE;
    }

    /*
     * Garante que o buffer esteja terminado com NULL antes de ser
     * utilizado pelo printf. O kernel deve retornar uma string terminada,
     * mas o cliente mantém o último byte reservado para terminação.
     */
    Message[MessageSize - 1] = '\0';

    return TRUE;
}

/* ------------------------------------------------------------------------ *
 *                              Ponto de entrada                             *
 * ------------------------------------------------------------------------ */

int main(void)
{
    SetConsoleOutputCP(65001);
    HANDLE deviceHandle = INVALID_HANDLE_VALUE;

    CHAR message[NTVISOR_MESSAGE_SIZE] = { 0 };
    DWORD messageBytesReturned = 0;

    printf("\n");

    printf("███╗   ██╗████████╗██╗  ██╗ █████╗ ██████╗  ██████╗ ██╗   ██╗██╗  ██╗███████╗███╗   ██╗\n");
    printf("████╗  ██║╚══██╔══╝██║  ██║██╔══██╗██╔══██╗██╔═══██╗██║   ██║██║ ██╔╝██╔════╝████╗  ██║\n");
    printf("██╔██╗ ██║   ██║   ███████║███████║██║  ██║██║   ██║██║   ██║█████╔╝ █████╗  ██╔██╗ ██║\n");
    printf("██║╚██╗██║   ██║   ██╔══██║██╔══██║██║  ██║██║   ██║██║   ██║██╔═██╗ ██╔══╝  ██║╚██╗██║\n");
    printf("██║ ╚████║   ██║   ██║  ██║██║  ██║██████╔╝╚██████╔╝╚██████╔╝██║  ██╗███████╗██║ ╚████║\n");
    printf("╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝  ╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝\n");

    printf("\n");
    printf("                         Windows NT Internals Research\n");
    printf("                         AMD SVM / VMRUN Experiment\n");
    printf("\n");

    printf("[*] Cliente NtVisor em user-mode\n");

    deviceHandle = NtVisorOpenDevice();

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    /*
     * Executa a verificação do ambiente SVM antes de solicitar o VMRUN.
     *
     * NTVISOR_ENABLE não retorna dados de mensagem. Ele apenas solicita
     * ao driver que valide e prepare o ambiente SVM.
     */
    if (!NtVisorEnable(deviceHandle))
    {
        NtVisorCloseDevice(deviceHandle);
        return 1;
    }

    /*
     * Executa o experimento VMRUN.
     *
     * A mensagem é retornada somente após o kernel concluir SVM_VMRUN
     * e copiar ctx->Message para o buffer de saída do IOCTL.
     */
    if (!NtVisorVmrun(
        deviceHandle,
        message,
        sizeof(message),
        &messageBytesReturned))
    {
        NtVisorCloseDevice(deviceHandle);
        return 1;
    }

    printf("\n");
    printf("+----------------------------------------------------+\n");
    printf("|                                                    |\n");
    printf("|              %-24s              |\n", message);
    printf("|                                                    |\n");
    printf("+----------------------------------------------------+\n");
    printf("\n");

    NtVisorCloseDevice(deviceHandle);

    return 0;
}