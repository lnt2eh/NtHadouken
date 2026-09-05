# Pesquisa: por que evitar `__svm_vmrun` e usar trampoline em assembly manual

**Objetivo:** reunir argumentos técnicos, sustentados em fonte primária (AMD APM / SVM Manual) e em documentação da Microsoft, para justificar — no AstarothVisor — o uso de um trampoline MASM próprio no lugar do intrínseco `__svm_vmrun` do MSVC.

---

## 1. O que o intrínseco realmente faz

Assinatura oficial (Microsoft Learn, header `<intrin.h>`):

```c
void __svm_vmrun(size_t VmcbPhysicalAddress);
```

Pontos-chave da documentação da Microsoft:
- Recebe **um único parâmetro**: o endereço físico da VMCB.
- **Retorna `void`** — não devolve nenhuma informação sobre a razão do VMEXIT nem sinaliza que uma troca de contexto ocorreu.
- A própria documentação diz que a função usa "uma quantidade mínima de informação da VMCB" para iniciar a execução do guest, e remete o desenvolvedor à AMD APM Vol. 2 para qualquer coisa além disso.
- Não existe nenhuma variante que receba ponteiro para struct de GPRs, nem hook de pré/pós execução.

**Conclusão do passo 1:** o intrínseco é um wrapper fino de *uma única instrução*. Ele não tenta resolver nenhum dos problemas de troca de contexto — isso fica inteiramente a cargo de quem chama.

---

## 2. O que a especificação da AMD exige ao redor de VMRUN

Fontes: AMD64 SVM Architecture Reference Manual (doc. 24593) + confirmação cruzada com Project Zero ("An EPYC escape") e o devlog *Mini-SVM*.

Fatos centrais:

1. **RAX** deve conter o endereço físico da VMCB antes de `VMRUN`.
2. `VMRUN` salva automaticamente um subconjunto do estado do host na área apontada pela MSR `VM_HSAVE_PA` (configurada previamente pelo hypervisor) — mas essa área cobre principalmente estado de sistema (segmentos, CR0/CR3/CR4, EFER, IDTR/GDTR), não os registradores de propósito geral.
3. **No #VMEXIT, o hardware só restaura automaticamente RIP, RSP e RAX** para os valores do host. Todos os demais GPRs (RBX, RCX, RDX, RSI, RDI, RBP, R8–R15) permanecem com os valores que o **guest** deixou neles.
4. O GIF (Global Interrupt Flag) precisa ser controlado manualmente com `clgi` antes e `stgi` depois de `VMRUN` — sem isso, uma interrupção pode chegar num momento em que o estado host/guest está inconsistente.
5. Um contexto completo de troca também depende de `VMSAVE`/`VMLOAD` para registradores de sistema adicionais (FS, GS, LDTR, STAR, LSTAR, etc.), que não fazem parte do save automático de `VMRUN`.

**Conclusão do passo 2:** a especificação deixa claro, por design, que **GPRs (exceto RAX) são compartilhados entre host e guest através de VMRUN** e que salvá-los/restaurá-los é responsabilidade explícita do software.

---

## 3. O gap entre o intrínseco e o ciclo de vida real do VMEXIT

Aqui está o argumento mais forte, e ele nasce direto do cruzamento dos passos 1 e 2:

- A ABI x64 do Windows define RBX, RBP, RDI, RSI, R12–R15 como **non-volatile** (o compilador assume que sobrevivem intactos a uma chamada de função) e RCX, RDX, R8–R11 como **volatile**.
- `__svm_vmrun` é, para o compilador, **uma chamada de função comum**. O MSVC pode perfeitamente manter valores "vivos" do host em registradores non-volatile através dessa chamada, confiando que eles vão sobreviver — exatamente como sobreviveriam a qualquer outra call.
- Mas, na prática, depois que `VMRUN` retorna (isto é, depois de um #VMEXIT), **esses registradores não contêm mais valores do host — contêm o que o guest executou por último**.
- Resultado: o compilador pode gerar código que lê um registrador non-volatile logo após `__svm_vmrun` esperando o valor do host, e na verdade lê lixo do guest. Isso é silencioso — não há warning, não há erro de compilação — e se manifesta como corrupção de stack, `#UD`, `VMEXIT_INVALID` ou triple fault, dependendo de qual registrador foi pisado e como o código seguinte o usa.
- Não há como inserir `clgi`/`stgi` *precisamente* ao redor da instrução `VMRUN` gerada pelo intrínseco — o compilador decide o resto do agendamento de instruções.

**Conclusão do passo 3:** o intrínseco não viola a especificação da AMD por si só, mas viola a suposição implícita da ABI do Windows sobre quais registradores sobrevivem a uma "chamada de função" — e é exatamente esse descompasso ABI-vs-hardware que se manifesta como os bugs `#UD`/`VMEXIT_INVALID`/triple fault já registrados no desenvolvimento do AstarothVisor.

---

## 4. Confirmação em implementações reais (não apenas teoria)

Múltiplas fontes independentes de desenvolvimento de hypervisor SVM descrevem o mesmo padrão obrigatório, sempre em assembly puro, nunca via intrínseco:

- **Mini-SVM** (devlog público): descreve explicitamente que RBX–R15 são compartilhados entre HV e VM, e que sem salvá-los manualmente "o HV começaria a usar os registradores da VM ao sair e simplesmente travaria". A sequência usada é `push` de todos os GPRs do host → carregar registradores do guest → `clgi; vmrun; stgi` → salvar registradores do guest de volta na struct → `pop` dos registradores do host.
- **SimpleSvm** (tandasat, hypervisor educacional de referência para Windows/AMD): implementa a transição inteira em `x64.asm`, com convenção de retorno própria via registradores (RBX/RCX/EDX:EAX) para sincronizar com o código C que o chamou — desenho só possível porque o trampoline é escrito à mão, não gerado por um intrínseco opaco.
- **Google Project Zero** ("An EPYC escape", análise de uma vulnerabilidade real em KVM/SVM): confirma que só RIP, RSP e RAX voltam automaticamente ao host no #VMEXIT, e que uma troca de contexto completa exige `VMSAVE`/`VMLOAD` manuais — o mesmo achado do passo 2, agora validado em um hypervisor de produção (KVM).

**Conclusão do passo 4:** nenhum hypervisor SVM sério para Windows usa `__svm_vmrun` como ponto de entrada/saída principal — todos escrevem o trampoline em assembly, justamente pelo motivo do passo 3.

---

## 5. Cruzamento com os bugs já registrados no AstarothVisor

Isso já está documentado no seu progresso:

- `VMRUN` gerando `#UD` — consistente com estado de CPU/afinidade incorreto antes da execução, mas também com corrupção de registrador de controle se algo pisou em CR-relacionados via GPR sujo.
- `VMEXIT_INVALID` — consistente com inconsistência de estado GPR/segmento no momento do exit.
- Triple fault por TR/TSS malconfigurado — categoria adjacente de "estado que precisa ser preservado manualmente e não foi".
- Bug em que **qualquer VMRUN após o primeiro** derrubava o host, com causa raiz na rotina de guest-resume em assembly — este é exatamente o tipo de bug que a análise do passo 3 prevê: se o *primeiro* VMRUN funciona por coincidência de quais registradores o compilador decidiu manter vivos, e o *segundo* falha porque o agendamento de registradores mudou (ex: código adicional entre as duas chamadas), isso bate com "funciona às vezes, para aleatoriamente" — o sintoma que você reportou sobre a instabilidade atual do SVM.

---

## 6. Tabela comparativa (argumento consolidado)

| Critério | `__svm_vmrun` (intrínseco MSVC) | Trampoline manual em MASM |
|---|---|---|
| Controle sobre GPRs host/guest | Nenhum — compilador decide o que fica vivo em qual registrador | Total — você decide exatamente o que é salvo, quando e onde |
| Bracket `clgi`/`stgi` | Não garantido ao redor da instrução exata | Garantido, instrução a instrução |
| Comportamento no #VMEXIT | Tratado como retorno de função comum pela ABI x64 — falso, pois GPRs non-volatile podem ter sido sobrescritos pelo guest | Tratado explicitamente como fronteira de contexto — nenhum registrador é assumido como preservado |
| Reprodutibilidade | Depende de decisões de alocação de registrador do compilador (pode mudar entre builds/otimizações) | Determinístico, mesmo comportamento em qualquer build |
| Precedente em hypervisors reais | Não encontrado nenhum projeto de referência (SimpleSvm, mini-svm, KVM) usando o intrínseco como entry point | Padrão universal nos projetos analisados |
| Debugabilidade | Difícil — bug se manifesta longe da causa raiz | Mais fácil — transição de contexto é explícita e visível no disassembly |
| Portabilidade MSVC/Clang | Ambos suportam o intrínseco de forma similar, mas isso não resolve o problema de ABI acima | Independente de toolchain, você controla a convenção |

---

## Fontes consultadas

- Microsoft Learn — `__svm_vmrun` intrinsic reference (`learn.microsoft.com/en-us/cpp/intrinsics/svm-vmrun`)
- AMD64 Architecture Programmer's Manual Vol. 2 / SVM Architecture Reference Manual (doc. 24593)
- Google Project Zero — "An EPYC escape: Case-study of a KVM breakout" (projectzero.google, 2021)
- Mini-SVM devlog — "Hello World, but it's a hypervisor instead" (varko.xyz)
- SimpleSvm (tandasat/SimpleSvm) — código-fonte `x64.asm`
- Rolen Louie — "Building a Type-2 Hypervisor from Scratch" (vmmcall.org)

---

## Próximo passo sugerido

Com esse material, dá pra escrever a seção de justificativa técnica do AstarothVisor (comentário de cabeçalho no trampoline, ou um doc interno) citando especificamente o passo 3 como motivo central — é o argumento mais defensável tecnicamente porque não depende de opinião, só do confronto ABI x64 vs. comportamento documentado de hardware do VMRUN.
