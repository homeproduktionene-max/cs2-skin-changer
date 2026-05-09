# Troubleshooting — Skins Não Aparecem

## ✅ Correções Aplicadas

### 1. Reordenação da Lógica de Aplicação
- Agora verifica se há skin **ANTES** de marcar como aplicado
- Evita marcar armas sem skin como "aplicadas"
- Garante que skins recém-selecionadas sejam aplicadas imediatamente

### 2. Debug Logging Adicionado
- `[SKIN] Aplicando skin X em arma defIndex=Y` — quando uma skin é escrita
- `[UPDATE] Chamando UpdateWeapons` — quando o CS2 é notificado

---

## 🧪 Como Testar

1. **Compile o projeto** (Release x64)
2. **Execute o CS2** e entre em um servidor offline/bot
3. **Execute o skin changer**
4. **Pressione INSERT** para abrir o menu
5. **Selecione uma arma** (ex: AK-47)
6. **Selecione uma skin**
7. **Observe o console** do skin changer:
   - Deve aparecer `[SKIN] Aplicando skin...`
   - Deve aparecer `[UPDATE] Chamando UpdateWeapons...`
8. **Pegue a arma no jogo** e verifique se a skin aparece

---

## 🔍 Diagnóstico

### Se o console mostrar:

#### ✅ `[SKIN] Aplicando skin...` + `[UPDATE] Chamando UpdateWeapons...`
**Significado:** O skin changer está funcionando corretamente.

**Se a skin ainda não aparecer:**
- Os **offsets estão desatualizados**
- O CS2 foi atualizado e mudou a estrutura de memória
- **Solução:** Atualize os offsets usando o [a2x/cs2-dumper](https://github.com/a2x/cs2-dumper)

#### ❌ `RegenerateWeaponSkins not found`
**Significado:** A assinatura da função interna do CS2 está desatualizada.

**Solução:**
1. Abra `ext/sigs.h`
2. Atualize os patterns de `FindRegenerateWeaponSkins()`
3. Consulte o [a2x/cs2-dumper](https://github.com/a2x/cs2-dumper) para patterns atualizados

#### ❌ Nenhuma mensagem aparece
**Significado:** O skin changer não está detectando a seleção de skin.

**Possíveis causas:**
1. `skinManager->AddSkin()` não está sendo chamado
2. `RequestForceUpdate()` não está funcionando
3. O loop principal está travado

**Solução:**
1. Verifique se o menu está respondendo (consegue clicar nas skins?)
2. Adicione um print em `menu.h` dentro do callback de clique da skin
3. Verifique se o console mostra `[UPDATE]` a cada ~5 segundos (timer de força bruta)

---

## 🛠️ Offsets Críticos

Se as skins não aparecem, verifique se estes offsets estão corretos em `ext/offsets.h`:

```cpp
// Offsets críticos para skins:
m_nFallbackPaintKit      // ID da skin
m_AttributeManager       // Gerenciador de atributos
m_Item                   // C_EconItemView
m_iItemDefinitionIndex   // ID da arma (7=AK-47, 9=AWP)
m_iItemIDHigh            // Flag de aplicação
```

### Como Atualizar Offsets:

1. Acesse https://github.com/a2x/cs2-dumper
2. Baixe o arquivo `offsets.json` mais recente
3. Substitua os valores em `ext/offsets.h`
4. Recompile o projeto

---

## 📊 Fluxo de Aplicação de Skin

```
Usuário clica na skin
    ↓
skinManager->AddSkin()
    ↓
RequestForceUpdate() (seta ForceUpdate = true)
    ↓
RunTick() detecta ForceUpdate
    ↓
Zera m_iItemIDHigh de todas as armas
    ↓
Loop de armas:
    - Lê defIndex da arma
    - Busca skin no skinManager
    - Escreve m_nFallbackPaintKit
    - Escreve atributos (Paint, Seed, Wear)
    - Marca m_iItemIDHigh = -1
    ↓
UpdateWeapons() chama RegenerateWeaponSkins
    ↓
CS2 recarrega os modelos das armas
    ↓
Skin aparece no jogo ✅
```

---

## 🐛 Problemas Conhecidos

### 1. Skins Desaparecem Após Morte
**Causa:** O CS2 reseta os atributos ao respawnar.

**Solução:** O timer de força bruta (a cada 5s) reaplica automaticamente.

### 2. Skins Não Aparecem em Armas Dropadas
**Causa:** Armas no chão não têm `m_hOwnerEntity` válido.

**Solução:** Isso é esperado. Skins só aparecem em armas equipadas.

### 3. Algumas Skins Não Funcionam
**Causa:** Skins que requerem modelos 3D customizados (ex: M4A4 Howl).

**Solução:** Verifique se `m_MeshGroupMask` está sendo escrito corretamente.

---

## 📞 Suporte

Se nenhuma das soluções acima funcionar:

1. **Capture o output do console** (copie todas as mensagens)
2. **Anote a versão do CS2** (Menu Principal → Configurações → Sobre)
3. **Verifique se os offsets estão atualizados** (compare com a2x/cs2-dumper)
4. **Teste com uma skin simples** (ex: AK-47 | Redline) antes de testar skins complexas

---

## ✅ Checklist de Verificação

- [ ] Console mostra `[SKIN] Aplicando skin...`
- [ ] Console mostra `[UPDATE] Chamando UpdateWeapons...`
- [ ] Console NÃO mostra `RegenerateWeaponSkins not found`
- [ ] Offsets estão atualizados (comparar com a2x/cs2-dumper)
- [ ] CS2 está rodando (não crashou)
- [ ] Estou em um servidor offline/bot (não VAC)
- [ ] A arma está equipada (não no chão)
- [ ] Aguardei pelo menos 5 segundos após selecionar a skin

Se todos os itens acima estão ✅ e a skin ainda não aparece, o problema é nos offsets ou na assinatura de `RegenerateWeaponSkins`.
