# CS2 Skin Changer — Versão Educativa Simplificada

## 📚 Propósito Educativo

Este projeto foi **simplificado e documentado** para fins educacionais. Toda a lógica complexa de facas, luvas e injeção de código foi removida, mantendo apenas o essencial: **aplicação de skins em armas**.

O código está **completamente documentado em português** com explicações detalhadas sobre:
- Como funciona a memória do Windows
- Como o Source 2 gerencia entidades e atributos
- Como evitar crashes (Access Violation 0xC0000005)
- Boas práticas de programação em C++

---

## 🎯 O Que Este Projeto Faz

Aplica skins customizadas em armas do CS2 **sem modificar o jogo**. Funciona via:
- `ReadProcessMemory`: lê a memória do cs2.exe
- `WriteProcessMemory`: escreve valores nos offsets corretos
- **Não injeta DLL**, **não modifica código do jogo**

### Funcionalidades Mantidas ✅
- ✅ Skins de armas (AK-47, AWP, M4A4, etc.)
- ✅ Music Kits
- ✅ Validação de estado (evita crashes)
- ✅ Cache de memória (otimização)
- ✅ Interface gráfica (ImGui)

### Funcionalidades Removidas ❌
- ❌ Facas (modelos customizados)
- ❌ Luvas
- ❌ Shellcode/injeção de código
- ❌ AcceptInput (troca de subclasse)

---

## 🚀 Como Compilar e Usar

### Pré-requisitos
- Visual Studio 2022 (ou superior) com suporte a C++17
- Windows 10/11
- CS2 instalado e rodando

### Passos para Compilar
1. Abra o arquivo `ext-cs2-skin-changer.sln` no Visual Studio.
2. Selecione a configuração `Release` e plataforma `x64`.
3. Clique em "Build" > "Build Solution".
4. O executável será gerado em `x64/Release/ext-cs2-skin-changer.exe`.

### Como Usar
1. Execute o CS2.
2. Execute o `ext-cs2-skin-changer.exe` como administrador.
3. Selecione uma skin no menu e aplique.
4. Certifique-se de que o CS2 está em uma partida para ver as mudanças.

**Nota:** Este projeto é para fins educacionais. Use por sua conta e risco.

---

## 🏗️ Arquitetura do Projeto

### Estrutura de Arquivos

```
ext-cs2-skin-changer/
├── src/
│   ├── main.cpp                    # Loop principal (DOCUMENTADO)
│   ├── StateSync.hpp/cpp           # Gerenciamento de estado (SIMPLIFICADO)
│   ├── menu.h                      # Interface gráfica (ImGui)
│   ├── config.h                    # Salvar/carregar configurações
│   └── SDK/
│       ├── CEconItemAttributeManager.h  # Atributos customizados (DOCUMENTADO)
│       └── weapon/
│           └── C_EconEntity.h      # Funções de armas (SIMPLIFICADO)
├── ext/
│   ├── mem.h                       # Classe Memory (DOCUMENTADO)
│   ├── offsets.h                   # Offsets do CS2 (a2x dumper)
│   └── skindb.h                    # Banco de dados de skins
└── README.md                       # Este arquivo
```

---

## 🧠 Como Funciona (Explicação Técnica)

### 1. Conexão ao Processo

```cpp
// Abre um handle para o cs2.exe com permissões completas
HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
```

**O que isso faz:**
- `OpenProcess`: API do Windows que retorna um "handle" (identificador) do processo
- `PROCESS_ALL_ACCESS`: permissões para ler, escrever e criar threads
- `processId`: PID do cs2.exe (obtido via `CreateToolhelp32Snapshot`)

---

### 2. Leitura de Memória

```cpp
// Lê o ponteiro do jogador local
uintptr_t localPlayer = mem.Read<uintptr_t>(client_base + Offsets::dwLocalPlayerPawn);
```

**O que isso faz:**
- `ReadProcessMemory`: API do Windows que copia bytes da memória do cs2.exe
- `client_base`: endereço base da client.dll (obtido via `GetModuleBase`)
- `Offsets::dwLocalPlayerPawn`: offset do ponteiro do jogador (dumper a2x)

---

### 3. Aplicação de Skins

```cpp
// Escreve o ID da skin no offset correto
mem.Write<uint32_t>(weapon + Offsets::m_nFallbackPaintKit, skinID);
```

**O que isso faz:**
- `WriteProcessMemory`: API do Windows que escreve bytes na memória do cs2.exe
- `m_nFallbackPaintKit`: offset do ID da skin dentro da estrutura C_CSWeaponBase
- `skinID`: ID numérico da skin (ex: 38 = AK-47 Fire Serpent)

**Estrutura da Arma no CS2:**
```
C_CSWeaponBase (arma física)
  ├─ m_AttributeManager
  │   └─ m_Item (C_EconItemView)
  │       ├─ m_iItemDefinitionIndex: ID da arma (7=AK-47, 9=AWP)
  │       ├─ m_nFallbackPaintKit: ID da skin ← ESCREVEMOS AQUI
  │       ├─ m_nFallbackSeed: Seed do padrão
  │       └─ m_flFallbackWear: Desgaste (0.0-1.0)
  └─ m_pGameSceneNode
      └─ m_modelState
          └─ m_MeshGroupMask: Bitmask de meshes visíveis
```

---

### 4. Atributos Customizados

Algumas skins precisam de atributos extras (Pattern, Wear). Criamos um array de `CEconItemAttribute` na memória do CS2:

```cpp
// 1. Aloca memória no cs2.exe
uintptr_t block = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT, PAGE_READWRITE);

// 2. Escreve os atributos no bloco
CEconItemAttribute attr;
attr.defIndex = 6;  // Paint
attr.value = 38.0f; // Fire Serpent
WriteProcessMemory(hProcess, block, &attr, sizeof(attr), NULL);

// 3. Atualiza o ponteiro m_Attributes para apontar para nosso bloco
CPtrGameVector vec;
vec.size = 1;
vec.ptr = block;
WriteProcessMemory(hProcess, item + Offsets::m_AttributeList, &vec, sizeof(vec), NULL);
```

**Por que isso funciona:**
- O CS2 lê o ponteiro `m_Attributes` e processa os atributos
- Não modificamos código do jogo, apenas dados na memória

---

## 🛡️ Prevenção de Crashes

### Problema: Access Violation (0xC0000005)

Ocorre quando tentamos ler/escrever em memória inválida:
- Ponteiro NULL (0x0)
- Memória não alocada (MEM_FREE)
- Memória protegida (PAGE_NOACCESS, PAGE_GUARD)

### Solução: Validação com VirtualQueryEx

```cpp
bool IsReadableCommitted(uintptr_t address) {
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQueryEx(hProcess, address, &mbi, sizeof(mbi));
    
    // Verifica se está alocada (MEM_COMMIT) e readable
    return (mbi.State == MEM_COMMIT) && 
           (mbi.Protect & PAGE_READABLE);
}
```

**O que isso faz:**
- `VirtualQueryEx`: consulta informações sobre uma região de memória
- `mbi.State`: MEM_COMMIT (alocada) ou MEM_FREE (livre)
- `mbi.Protect`: PAGE_READWRITE, PAGE_READONLY, PAGE_NOACCESS, etc.

---

### Problema: Escrita em Memória Read-Only

Alguns offsets estão em páginas protegidas (PAGE_READONLY). Escrever diretamente causa crash.

### Solução: VirtualProtectEx

```cpp
// 1. Salva a proteção original
DWORD oldProtect;
VirtualProtectEx(hProcess, address, size, PAGE_EXECUTE_READWRITE, &oldProtect);

// 2. Escreve o valor
WriteProcessMemory(hProcess, address, &value, size, NULL);

// 3. Restaura a proteção original (segurança)
VirtualProtectEx(hProcess, address, size, oldProtect, &oldProtect);
```

**Por que restaurar a proteção:**
- Segurança: evita que outros processos escrevam nessa região
- Estabilidade: o CS2 pode verificar a proteção em alguns casos

---

### Problema: Crash Durante Transição Lobby→Partida

Durante a transição, o pawn do jogador é destruído e recriado. Escrever em armas durante essa janela causa crash.

### Solução: Guards de Estado

```cpp
// m_lifeState: 0=vivo, 1=morrendo, 2=morto
uint8_t lifeState = mem.Read<uint8_t>(localPlayer + Offsets::m_lifeState);
if (lifeState != 0) return; // Skip se não estiver vivo

// m_nSimulationTick: 0 = entidade recém-criada (não simulada ainda)
int32_t simTick = mem.Read<int32_t>(localPlayer + Offsets::m_nSimulationTick);
if (simTick <= 0) return; // Skip se não foi simulada
```

**Por que isso funciona:**
- `simTick > 0`: garante que a entidade foi simulada pelo menos uma vez
- Armas só existem após a primeira simulação do pawn

---

## 🔧 Offsets e Dumpers

### O Que São Offsets?

Offsets são **deslocamentos** (em bytes) dentro de uma estrutura de memória.

Exemplo:
```cpp
struct Player {
    int health;        // offset 0x0
    int armor;         // offset 0x4
    Vector3 position;  // offset 0x8
};

// Para ler a vida do jogador:
int health = mem.Read<int>(playerAddress + 0x0);

// Para ler a posição:
Vector3 pos = mem.Read<Vector3>(playerAddress + 0x8);
```

### Por Que Mudam a Cada Update?

Quando a Valve atualiza o CS2:
- Adiciona novos campos nas estruturas
- Reordena campos existentes
- Muda o tamanho de estruturas

Isso faz os offsets mudarem. Exemplo:
```cpp
// Antes do update:
struct Player {
    int health;    // offset 0x0
    int armor;     // offset 0x4
};

// Depois do update (adicionaram um campo):
struct Player {
    int health;    // offset 0x0
    int teamID;    // offset 0x4 ← NOVO CAMPO
    int armor;     // offset 0x8 ← MUDOU DE 0x4 PARA 0x8
};
```

### Dumper a2x

O [a2x/cs2-dumper](https://github.com/a2x/cs2-dumper) é um projeto que:
1. Analisa o client.dll do CS2
2. Extrai os offsets automaticamente
3. Publica em JSON/C++ via GitHub Actions
4. Atualiza a cada patch do CS2

**Como usar:**
```cpp
#include "ext/a2x/client_dll.hpp"

// Offsets atualizados automaticamente:
uintptr_t localPlayer = mem.Read<uintptr_t>(
    client_base + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn
);
```

---

## 🎓 Conceitos de Programação

### 1. Latch Idempotente (StateController)

**Problema:** Aplicar a skin 200 vezes por segundo (loop a 200 FPS) é desperdício e pode causar instabilidade.

**Solução:** Latch que garante aplicação **apenas uma vez** por transição:

```cpp
class StateController {
    std::atomic<bool> appliedOnce_{false};
    
public:
    bool tryLatch() {
        bool expected = false;
        // CAS (Compare-And-Swap): atômico, thread-safe
        return appliedOnce_.compare_exchange_strong(expected, true);
    }
    
    void resetLatch() {
        appliedOnce_.store(false);
    }
};

// Uso:
static StateController controller;

if (activeWeapon != lastActiveWeapon) {
    controller.resetLatch(); // Permite nova aplicação
}

if (controller.tryLatch()) {
    ApplySkin(weapon, skin); // Executado apenas UMA vez
}
```

**Como funciona:**
- `compare_exchange_strong`: operação atômica (lock-free)
- Se `appliedOnce_` for `false`, seta para `true` e retorna `true`
- Se `appliedOnce_` já for `true`, retorna `false` (skip)

---

### 2. Cache de Blocos Alocados

**Problema:** Alocar/liberar memória a cada frame causa:
- Fragmentação de memória
- Lentidão (VirtualAllocEx é lento)
- Race conditions (alloc durante frame render)

**Solução:** Cache que reutiliza blocos:

```cpp
std::unordered_map<uintptr_t, uintptr_t> allocCache_;
// Chave: endereço do item
// Valor: endereço do bloco alocado

void Create(uintptr_t item, SkinInfo_t skin) {
    uintptr_t block = 0;
    
    // Tenta reutilizar bloco existente
    auto it = allocCache_.find(item);
    if (it != allocCache_.end()) {
        block = it->second;
    } else {
        // Primeira vez: aloca novo bloco
        block = mem.Allocate(0, size);
        allocCache_[item] = block;
    }
    
    // Escreve os atributos no bloco (sobrescreve)
    mem.Write<CEconItemAttribute>(block, attr);
}
```

**Benefícios:**
- Aloca apenas uma vez por item
- Sobrescreve o bloco nas atualizações seguintes
- Sem alloc/free a cada frame

---

### 3. Structured Exception Handling (SEH)

**Problema:** Crashes param o programa. Queremos capturar e continuar.

**Solução:** SEH do Windows:

```cpp
void RunTickSEH() {
    __try {
        RunTick(); // Código que pode crashar
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        DWORD code = GetExceptionCode();
        printf("[CRASH] 0x%08X\n", code);
        Sleep(1000); // Aguarda antes de tentar novamente
    }
}
```

**Códigos de exceção comuns:**
- `0xC0000005`: Access Violation (ponteiro inválido)
- `0xC0000094`: Integer Divide by Zero
- `0xC00000FD`: Stack Overflow

---

## 📖 Recursos para Aprender Mais

### APIs do Windows
- [ReadProcessMemory](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory)
- [WriteProcessMemory](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-writeprocessmemory)
- [VirtualQueryEx](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualqueryex)
- [VirtualProtectEx](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotectex)

### Source 2 Engine
- [Valve Developer Community](https://developer.valvesoftware.com/wiki/Source_2)
- [CS2 Dumper (a2x)](https://github.com/a2x/cs2-dumper)

### C++ Avançado
- [std::atomic](https://en.cppreference.com/w/cpp/atomic/atomic)
- [Memory Order](https://en.cppreference.com/w/cpp/atomic/memory_order)
- [Structured Exception Handling](https://learn.microsoft.com/en-us/cpp/cpp/structured-exception-handling-c-cpp)

---

## ⚠️ Avisos Legais

Este projeto é **apenas para fins educacionais**. Uso em servidores oficiais do CS2 pode resultar em:
- VAC Ban (Valve Anti-Cheat)
- Banimento permanente da conta Steam

**Use apenas em:**
- Servidores locais (offline)
- Servidores privados com permissão
- Ambiente de desenvolvimento/aprendizado

---

## 🤝 Contribuindo

Este é um projeto educativo. Contribuições são bem-vindas:
- Melhorias na documentação
- Correções de bugs
- Exemplos adicionais
- Traduções

**Não serão aceitas:**
- Funcionalidades de bypass de anti-cheat
- Código ofuscado/malicioso
- Funcionalidades que violem ToS da Valve

---

## 📝 Licença

Este projeto é fornecido "como está", sem garantias. Use por sua conta e risco.

---

## 🙏 Créditos

- **a2x**: Dumper de offsets do CS2
- **ocornut**: ImGui (interface gráfica)
- **nlohmann**: JSON library
- **Comunidade CS2**: Pesquisa e documentação

---

**Bons estudos! 📚**
