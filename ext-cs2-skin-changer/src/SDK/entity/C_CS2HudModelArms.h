#include "dwEntityListManager.h"
#include "../../../ext/offsets.h"

#pragma once

uintptr_t GetHudArms()
{
    return GetEntityByHandle(mem.Read<uint32_t>(GetLocalPlayer() + Offsets::m_hHudModelArms));
}

uintptr_t GetHudWeapon(const uintptr_t weapon)
{
    const auto armsBase = GetHudArms();
    if (!armsBase) return 0;

    const auto armsNode = mem.Read<uintptr_t>(armsBase + Offsets::m_pGameSceneNode);
    if (!armsNode) return 0;

    for (uintptr_t viewModel = mem.Read<uintptr_t>(armsNode + Offsets::m_pChild);
         viewModel && viewModel > 0x10000;
         viewModel = mem.Read<uintptr_t>(viewModel + Offsets::m_pNextSibling))
    {
        const uintptr_t owner = mem.Read<uintptr_t>(viewModel + Offsets::m_pOwner);
        if (!owner || owner < 0x10000) continue;

        const uintptr_t ownerEntity = GetEntityByHandle(mem.Read<uint32_t>(owner + Offsets::m_hOwnerEntity));
        if (ownerEntity == weapon)
            return owner;
    }

    return 0; // Not found — no recursion
}

//uintptr_t GetHudWeapon(const uintptr_t& weapon)
//{
//	uintptr_t hudWeapon = NULL;
//
//	while (!hudWeapon) { hudWeapon = GetEntityByHandle(mem.Read<uint16_t>(weapon + Offsets::m_hViewModelWeapon)); }
//
//	return hudWeapon;
//}

//std::array<uintptr_t, 8> GetAWeapons(uintptr_t LocalPlayer)
//{
//    std::array<uintptr_t, 8> weapons{};
//    int idx = 0;
//
//    const uintptr_t LocalNode = mem->Read<uintptr_t>(LocalPlayer + Offsets::m_pGameSceneNode);
//
//    uintptr_t weapon = mem->Read<uintptr_t>(LocalNode + Offsets::m_pChild);
//    while (weapon && idx < weapons.size())
//    {
//        uintptr_t weaponEntity = mem->Read<uintptr_t>(weapon + Offsets::m_pOwner);
//
//        if (weaponEntity != 0)
//            weapons[idx++] = weaponEntity;
//
//        weapon = mem->Read<uintptr_t>(weapon + Offsets::m_pNextSibling);
//    }
//
//    return weapons;
//}

static const uintptr_t pActiveHud = mem.Read<uintptr_t>(mem.ResolveRelativeAddress(mem.SigScan(L"client.dll", "48 8D 0D ? ? ? ? E8 ? ? ? ? EB ? 33 D2")) + 0x8);

inline uintptr_t GetActiveHudWeapon()
{
    return mem.Read<uintptr_t>(pActiveHud);
}