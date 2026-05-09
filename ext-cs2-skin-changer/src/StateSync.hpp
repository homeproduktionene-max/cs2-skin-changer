// ═════════════════════════════════════════════════════════════════════════════
// StateSync.hpp — Utilitários de sincronização de estado
// ═════════════════════════════════════════════════════════════════════════════
//
// Mantido para compatibilidade com o sistema de build (.vcxproj).
// O controle de "já aplicado" é feito diretamente pelo itemIDHigh == -1
// no loop de armas do RunTick, e a detecção de troca de arma dispara
// RequestForceUpdate() via SyncWithProcess().
//
// ═════════════════════════════════════════════════════════════════════════════

#pragma once
#include <atomic>
#include <cstdint>

inline std::atomic<bool> g_RequestForceUpdate{false};

inline void RequestForceUpdate()
{
    g_RequestForceUpdate.store(true, std::memory_order_relaxed);
}
