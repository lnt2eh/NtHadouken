# O Funcionamento do Manipulador de Syscall nt!KiSystemCall64 no Windows

Este artigo explora o funcionamento interno do sistema operacional Windows, especificamente o manipulador de syscall `nt!KiSystemCall64`. Nele, explico como chamadas do sistema são mecanismos cruciais que concedem ao sistema operacional a capacidade de solicitar serviços do kernel, permitindo operações que o próprio sistema operacional não pode executar diretamente, como a criação de arquivos no disco rígido.

O artigo investiga a distinção entre Modo Usuário (user mode) e Modo Kernel (kernel mode), ilustrando como as syscalls (abreviação de System Calls ou chamadas de sistema) preenchem a lacuna de comunicação entre esses dois modos. Por meio da análise detalhada e desmontagem do manipulador `KiSystemCall64`, o artigo examina a transição do modo de usuário para o modo kernel, o papel da instrução `swapgs` e o uso de registradores específicos de modelo (Model Specific Registers - MSRs) no processo. O estudo ainda fornece informações sobre a estrutura `KUSER_SHARED_DATA`, destacando sua importância na redução das mudanças de contexto. As descobertas contribuem para uma compreensão mais profunda dos componentes internos do Windows, oferecendo um guia básico para iniciantes neste campo.

Quando comecei minha jornada de estudos sobre o Windows, uma das curiosidades que eu tinha era saber como o sistema manipulava e lidava com as syscalls. Antes de falar do manipulador, vamos definir alguns conceitos básicos.

## O que são as chamadas de sistema?

As chamadas de sistema são um mecanismo fornecido pelo sistema operacional para a realização de algum serviço que o sistema operacional por si só não pode realizar. Por exemplo, criar um arquivo no disco rígido. Quando um usuário clica em “Criar um novo arquivo de texto” no Windows, por trás, o que acontece é que o SO usa uma syscall para pedir ao kernel que realize o serviço. De certa forma, isso existe para proteger o SO de atividades que poderiam causar danos se o usuário viesse a operar manualmente. Falando em proteção, vamos abordar os anéis (rings) de proteção do processador, que definem o modo usuário (ring 0) e o modo kernel (ring 3).

## Modo Usuário e Modo Kernel

Quando falamos em modo usuário e modo kernel, estamos nos referindo aos níveis de privilégio da CPU. Em x86/x64, aplicações em user mode normalmente executam em CPL 3, enquanto o código do kernel do Windows executa em CPL 0. Portanto, não é correto dizer que o sistema operacional inteiro atua em ring 3: o Windows inclui componentes em user mode e componentes privilegiados em kernel mode. A separação limita quais instruções e regiões de memória podem ser acessadas por cada contexto.

No Windows x64, a instrução `SYSCALL` realiza a transição controlada de CPL 3 para CPL 0 usando os MSRs configurados pelo sistema, incluindo `IA32_LSTAR`, que fornece o endereço de entrada no kernel. Diferentemente de uma instrução `INT`, `SYSCALL` não despacha a execução por uma entrada da IDT; ela usa um mecanismo específico da arquitetura. O código de entrada do kernel valida e trata a solicitação e, ao final do caminho apropriado, retorna ao modo usuário.

A figura 1 ilustra o fluxo de uma chamada de sistema no Windows, iniciando na camada de modo usuário. O processo começa com a chamada a uma função da API do Windows, que, por sua vez, utiliza a `ntdll.dll` para realizar a transição para o modo kernel por meio de uma instrução de sistema (syscall). Nesse exemplo, a função `CreateFile` da API do Windows é usada como referência, sendo internamente mapeada para a função `NtCreateFile` da `ntdll.dll`, responsável por invocar o interrupt handler no modo kernel.

<p align="center">
  <img src="https://media.invisioncic.com/u323382/monthly_2025_01/FluxogramaSyscall.png.f6a09c83c66d0416f0318d385dc6a3bc.png" />
</p>

<p align="center">
  Figura 1 - Fluxograma da CreateFile()*
</p>

## O manipulador de syscall do Windows

O `nt!KiSystemCall64` é o manipulador de syscall do Windows. Ele é responsável pela entrada no modo kernel quando uma syscall é realizada.

Quando uma chamada de sistema é realizada em um sistema de 64-bits, a instrução “syscall” é executada. Vamos olhar no WinDBG o stub da `NtCreateFile` para fins de entendimento.

Na figura 2, a função `NtCreateFile` é destrinchada em linguagem assembly, permitindo a compreensão detalhada do fluxo de execução. Inicialmente, o argumento localizado em `RCX` é movido para `R10`, alinhando-se à convenção de chamada esperada pelo kernel. Em seguida, o índice correspondente ao serviço na Tabela de Serviços do Sistema (SSDT) é carregado no registrador `EAX`. No caso da `NtCreateFile`, o índice utilizado é `55h`. Esse índice será processado pelo kernel durante a transição para o modo kernel, que ocorre por meio da instrução syscall. O Windows possui duas instruções que podem ser utilizadas para executar a syscall: a instrução `syscall`, representada pelo opcode `0f05` e a instrução `int 2Eh`, que possui o opcode `cd2e`. Ambas são utilizadas para causar a interrupção e alcançar o manipulador de syscall do Windows, o `nt!KiSystemCall64`, no caso da instrução `syscall` ou o `nt!KiSystemCall`, se a instrução for a `int 2Eh`. Antes da execução dessas instruções, um teste é realizado pelo próprio código da syscall para verificar se o sistema é x64 ou x86 (falarei mais deste teste em breve), como podemos ver na figura 2.

<p align="center">
  <img src="https://media.invisioncic.com/u323382/monthly_2025_01/NtCreateFileAsm.png.075ba58ad3edf04d2c78e52f163801fd.png" />
</p>

<p align="center">
  Figura 2 - Trecho de código da função NtCreateFile na ntdll.dll*
</p>

Quando a instrução syscall é executada no modo usuário, o valor armazenado no registrador MSR `IA32_LSTAR` é carregado no registrador `RIP`, transferindo a execução para o kernel. Este valor é um endereço conhecido como o ponto de entrada do Kernel RIP para syscalls. Em outras palavras, ele é o endereço do manipulador de syscall, `nt!KiSystemCall64`.

O `nt!KiSystemCall64` é o manipulador Kernel RIP Syscall em modo longo (64 bits). Quando a instrução syscall é executada, o código salta para a rotina do modo kernel cujo endereço é apontado por um MSR, acessado por meio das instruções `rdmsr` (leitura) e `wrmsr` (escrita).

## KUSER_SHARED_DATA

A estrutura `KUSER_SHARED_DATA` é uma estrutura utilizada para que o kernel compartilhe informações com os processos do lado do user-mode, evitando assim uma troca de contexto de user-mode para kernel-mode constante. Você pode procurar ver mais detalhes sobre essa estrutura na documentação da Microsoft em `KUSER_SHARED_DATA`.

O sistema acessa a estrutura `KUSER_SHARED_DATA` que possui um endereço fixo mapeado para os processos no modo usuário `0xfffff78000000000`. Na instrução `TEST`, um deslocamento com offset de `0x308` alcança o campo “SystemCall” da estrutura e o código testa se o valor dele é 1. Se for, significa que o sistema é baseado em x64 e ele a próxima instrução executada será a syscall. Caso contrário, ele pula para o endereço `0x00007ffe7756db55` que possui a instrução `INT 2Eh`. Ou seja, em sistemas baseados em x86, a instrução para gerar a interrupção é a `INT 2Eh`. Já na arquitetura x64, a instrução é a `SYSCALL`.

Ainda sobre a estrutura, de acordo com a documentação da Microsoft, o campo `SystemCall` é, em x64, inicializado para um valor diferente de zero se o sistema operar com uma exibição alterada do mecanismo de chamada de serviço do sistema.

Em x64, o membro `SystemCall` da estrutura `KUSER_SHARED_DATA` indica o mecanismo de chamadas de sistema em uso e é configurado com um valor diferente de zero, refletindo que o sistema está utilizando o modelo baseado em instruções otimizadas, como syscall. Em sistemas x86, esse membro permanece como zero, indicando que o modelo tradicional de chamadas de sistema, como `int 2Eh` ou `sysenter`, está em uso.

Quando a instrução syscall é executada, o valor de `IA32_LSTAR` é recuperado e colocado no registrador `RIP` do kernel. Chamamos isso de Kernel RIP Syscall. Após isso, o modo kernel passa a executar o manipulador de syscall `nt!KiSystemCall64`.

## Desmontagem do manipulador de syscall

Desmontando o manipulador `KiSystemCall64`, vemos que a primeira instrução a ser executada é `swapgs`, como mostra a figura 3.

<p align="center">
  <img src="https://media.invisioncic.com/u323382/monthly_2025_01/Swapgs.png.fb737f51ccfdec79cd8d7ebe34d03b3a.png" />
</p>

<p align="center">
  Figura 3 - Instrução swapgs no início do manipulador de syscall*
</p>

Na verdade, existem dois manipuladores syscall diferentes, com e sem a palavra-chave ‘Shadow’. A palavra “Shadow” vem do recurso chamado de Kernel Virtual Address Shadow, que visa corrigir o bug Meltdown.

A `swapgs` é uma instrução privilegiada usada para trocar o valor atual do registrador base GS pelo valor residente no endereço MSR `C0000102H` (`IA32_KERNEL_GS_BASE`). Explico: o valor do registro base GS é igual ao valor contido no MSR `IA32_GS_BASE`. Em sistemas Windows x64, os valores são:

- `IA32_KERNEL_GS_BASE` — Ponteiro para Processor Control Region (PCR) atual, especificamente a região de controle do processador do kernel (KPCR)
- `IA32_GS_BASE` — Ponteiro para thread de execução atual da TEB

Assim, no modo longo x64, o segmento GS sempre aponta para o thread atual TEB no modo usuário, enquanto no modo kernel aponta para o PCR do processador atual.

A próxima instrução `mov qword ptr gs:[10h],rsp` salva o ponteiro da pilha do modo usuário. O objetivo de salvar o ponteiro da pilha de modo usuário (`RSP`) durante a transição do modo usuário para o modo kernel é garantir que, quando o controle for devolvido ao modo usuário, a execução do programa possa continuar de maneira correta, mantendo o contexto da thread do usuário intacto.

## Conclusão

Neste artigo, exploramos o papel das chamadas de sistema no Windows, com foco no manipulador de chamadas de sistema (syscalls) `nt!KiSystemCall64`. As syscalls são fundamentais para que o sistema operacional se comunique com o kernel e execute tarefas críticas que não podem ser realizadas diretamente em modo usuário, como criar arquivos, por exemplo. Também vimos como a transição entre os modos usuário e kernel ocorre de forma eficiente com a instrução `swapgs` e o uso da estrutura `KUSER_SHARED_DATA`, que permite uma comunicação rápida e eficaz entre o kernel e os processos. Discutimos como o sistema mantém o contexto da execução do programa, permitindo que a execução continue corretamente quando o controle retorna ao modo usuário.
