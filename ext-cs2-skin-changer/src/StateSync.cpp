// ═════════════════════════════════════════════════════════════════════════════
// StateSync.cpp — Implementação do StateController (Simplificado)
// ═════════════════════════════════════════════════════════════════════════════
//
// Este arquivo contém a implementação PURA da classe StateController.
//
// IMPORTANTE:
// - NÃO inclui nenhum header do SDK (mem.h, skindb.h, C_EconEntity.h, etc.)
// - Esses headers definem variáveis globais e funções sem 'inline' diretamente
//   no corpo do header, o que causa LNK2005 (multiply defined symbols) se
//   incluídos em mais de uma translation unit (.cpp)
//
// VERSÃO SIMPLIFICADA:
// - Removida toda a lógica de facas, shellcode, e AcceptInput
// - Mantido apenas o latch idempotente para evitar escritas redundantes
// - Toda a implementação está no header (StateSync.hpp) como inline
// - Este arquivo existe apenas para manter a estrutura do projeto
//
// ═════════════════════════════════════════════════════════════════════════════

#include "StateSync.hpp"

// Arquivo vazio — toda a implementação está no header como inline.
// Mantido para compatibilidade com o sistema de build (.vcxproj).
