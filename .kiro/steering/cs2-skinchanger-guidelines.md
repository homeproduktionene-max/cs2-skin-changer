# CS2 Skin Changer — Diretrizes de Desenvolvimento

Este projeto é um skin changer externo para CS2 (Source 2) que opera via `ReadProcessMemory` / `WriteProcessMemory` a partir de um processo separado. As diretrizes abaixo devem ser seguidas em **todas** as conversas sobre este projeto.

---

## 1. Análise de Ciclo de Vida de Entidade Antes de Qualquer Mudança

Antes de propor qualquer função que altere modelo, subclasse ou estado de uma entidade, descrever como o Source 2 gerencia o ciclo de vida dessa entidade no contexto da operação.

**Perguntas obrigatórias antes de escrever código:**
- A entidade está no inventário do jogador ou no mundo (world model)?
- O `m_hOwnerEntity` é válido? Se não, a entidade foi dropada — não tocar em `m_hModel`.
- O `m_hModel` zerado numa entidade sem viewmodel ativo causa invisibilidade permanente.
- `ForceKnifeModel` só deve ser chamado quando `m_hOwnerEntity != 0xFFFFFFFF`.

**Regra:** Nunca zerar `m_hModel` de uma entidade que pode estar no chão.

---

## 2. Depuração por Árvore de Falhas

Quando algo crashar ou não funcionar, sempre listar as causas prováveis antes de propor código. Estrutura obrigatória:

1. **Alinhamento de Stack (RSP)** — verificar se `(RSP - 8) % 16 == 0` no momento do `CALL`. `sub rsp, 0x48` é correto; `sub rsp, 0x40` causa crash SSE.
2. **Ponteiro de VTable corrompido** — verificar se o endereço da função (ex: `AcceptInput`) foi resolvido após `mem.Connect()`, não antes.
3. **Race Condition no thread de renderização** — `CreateRemoteThread` durante o frame render pode corromper o VTable. Usar `WaitForSingleObject` sempre.
4. **Atributos não aplicados** — `CEconItemAttributeManager::Create` retorna sem fazer nada se `preAttributes.size > 0`. Sempre chamar `Remove` antes de `Create`.
5. **Globals inicializados antes de `main()`** — `Memory mem` e `Sigs::*` são globais. Se conectarem ao CS2 antes de `main()`, crasham se o jogo não estiver aberto.

---

## 3. Shellcode em Três Estágios

Todo shellcode de injeção deve ser estruturado em três estágios separados:

**Estágio 1 — Preparação de Dados:**
- Alocar um único bloco contíguo (nunca múltiplos `VirtualAllocEx` separados).
- Layout do bloco: `[inputName\0][subclassName\0][padding][variant_t][shellcode]`.
- `variant_t` para `ChangeSubclass`: `pszVal = ptr para subclassName`, `fieldType = 2` (FIELD_CSTRING, não FIELD_STRING=4).

**Estágio 2 — Invocação (Shellcode puro):**
- Convenção x64 Windows: RCX, RDX, R8, R9 + shadow space `0x20` + args extras na stack.
- `variant_t` é passado **por valor** na stack (não por ponteiro): `[rsp+0x20]` = pszVal, `[rsp+0x28]` = fieldType, `[rsp+0x30]` = outputID.
- `sub rsp, 0x48` → `CALL` → `add rsp, 0x48` → `xor eax, eax` → `ret`.
- Prova: entrada RSP % 16 == 0 → sub 0x48 → RSP % 16 == 8 → CALL push 8 → RSP % 16 == 0 ✓

**Estágio 3 — Sincronização:**
- Sempre `WaitForSingleObject(hThread, 1000)` antes de qualquer operação que dependa do resultado.
- `mem.Free(block)` só após `CloseHandle(hThread)`.

---

## 4. Contexto de Patching — Offsets Críticos

Quando a Valve atualizar o CS2, os offsets mudam. Offsets atualmente em uso:

| Campo | Offset | Arquivo |
|---|---|---|
| `m_pGameSceneNode` | `client_dll schema` | `offsets.h` |
| `m_modelState` | `CSkeletonInstance schema` | `offsets.h` |
| `m_hModel` | `0xA0` dentro de `m_modelState` | `offsets.h` |
| `m_ModelName` | `0xA8` dentro de `m_modelState` | `offsets.h` |
| `m_nSubclassID` | `weapon + 0x380` | `C_EconEntity.h` |
| `m_bDirtyHierarchy` | `node + 0x104` | `C_EconEntity.h` |
| `m_hOwnerEntity` | `client_dll schema` | `offsets.h` |

**Ao receber novos offsets:** perguntar explicitamente quais funções usam o offset alterado antes de aplicar.

---

## 5. Regras de Inicialização

- `Memory mem` é um global com construtor vazio — `mem.Connect(L"cs2.exe")` deve ser chamado no início de `main()` com loop de retry.
- `Sigs::RegenerateWeaponSkins` e `Sigs::AcceptInput` são `inline uintptr_t = 0` — resolvidos via `FindRegenerateWeaponSkins()` e `FindAcceptInput()` **após** `mem.Connect()`.
- Nunca chamar `mem.SigScan`, `mem.Read` ou `mem.Write` fora de `main()` durante inicialização estática.

---

## 6. Paths de Modelos de Faca

Formato correto para CS2: `"models/weapons/v_knife_<nome>.vmdl_c"`

Exemplos corretos:
- Butterfly: `models/weapons/v_knife_butterfly.vmdl_c`
- Karambit: `models/weapons/v_knife_karam.vmdl_c`
- M9 Bayonet: `models/weapons/v_knife_m9_bay.vmdl_c`

**Formato errado (não usar):** `phase2/weapons/models/knife/.../weapon_knife_xxx_ag2.vmdl`

---

## 7. Bugs Conhecidos e Correções Aplicadas

| Bug | Causa | Correção |
|---|---|---|
| Faca invisível no chão | `ForceKnifeModel` zerava `m_hModel` de entidade dropada | Checar `m_hOwnerEntity != 0xFFFFFFFF` antes de aplicar |
| Skins de armas somem após 10s | `Create` retornava se `preAttributes.size > 0` | `Create` chama `Remove` antes de alocar |
| `UpdateWeapons` apagava skins | `Remove` chamado em armas com skin configurada | `UpdateWeapons` só remove armas sem skin no `skinManager` |
| Crash ao abrir o painel | `Memory` construtor lançava exceção antes de `main()` | Construtor vazio + `Connect()` explícito em `main()` |
| Crash com shellcode | Stack desalinhado (`sub rsp, 0x38`) + `variant_t` por ponteiro | `sub rsp, 0x48` + `variant_t` por valor + `FIELD_CSTRING=2` |
| Modelo não mudava (nome certo, visual errado) | `AcceptInput` encontrado mas nunca chamado | Shellcode chama `AcceptInput("ChangeSubclass", ...)` |
| `HardResetKnifeSlot` chamado todo frame | `notApplied` sempre true por race condition | Marcar `m_iItemIDHigh = -1` antes das chamadas pesadas |
| `client`, `entitylist`, `pActiveHud` sempre 0 | Globais estáticos em headers chamavam `mem` antes de `main()` | Convertidos para lazy init com `static uintptr_t = 0` dentro de funções inline |
