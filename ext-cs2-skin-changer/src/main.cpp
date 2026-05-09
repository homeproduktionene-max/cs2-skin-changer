#include "menu.h"
#include "SDK/entity/CWeaponServices.h"
#include "SDK/weapon/C_EconEntity.h"
#include "SDK/CEconItemAttributeManager.h"
#include "StateSync.hpp"
#include "config.h"
#include <windows.h>

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    std::cout << "Aguardando CS2...\n";
    while (!mem.Connect(L"cs2.exe"))
    {
        Sleep(2000);
    }
    std::cout << "Conectado ao CS2 (PID " << mem.processId << ")\n";

    const auto regenerateWeaponSkins = Sigs::FindRegenerateWeaponSkins();
    if (regenerateWeaponSkins)
    {
        mem.Write<uint16_t>(regenerateWeaponSkins + 0x52,
            static_cast<uint16_t>(Offsets::m_AttributeManager + Offsets::m_Item +
                                  Offsets::m_AttributeList + Offsets::m_Attributes));
        std::cout << "RegenerateWeaponSkins patcheado\n";
    }

    // Dump síncrono — igual ao projeto de referência
    skindb->Dump();

    InitMenu();

    configManager->Setup();
    configManager->AutoLoad();

    std::cout << "Skin Changer ativo! INSERT para abrir o menu.\n";

    uintptr_t lastLocalPlayer = 0;
    uint8_t   lastLifeState   = 0;

    while (true)
    {
        try
        {
            // Render
            UpdateActiveMenuDef(GetLocalPlayer());
            OnFrame();

            Sleep(10);

            DWORD exitCode;
            if (!GetExitCodeProcess(mem.hProcess, &exitCode) || exitCode != STILL_ACTIVE)
                break;

            const uintptr_t localController = GetLocalController();
            if (!localController) continue;

            const uintptr_t inventoryServices = mem.Read<uintptr_t>(localController + Offsets::m_pInventoryServices);
            const uintptr_t localPlayer = GetLocalPlayer();
            if (!localPlayer || !inventoryServices) continue;

            const uint8_t lifeState = mem.Read<uint8_t>(localPlayer + cs2_dumper::schemas::client_dll::C_BaseEntity::m_lifeState);
            if (localPlayer != lastLocalPlayer || lifeState != lastLifeState)
            {
                ForceUpdate = true;
                lastLocalPlayer = localPlayer;
                lastLifeState   = lifeState;
            }

            const uintptr_t pWeaponServices = mem.Read<uintptr_t>(localPlayer + Offsets::m_pWeaponServices);
            if (!pWeaponServices) { Sleep(500); continue; }

            if (skinManager->MusicKit.id > 0)
                mem.Write<uint16_t>(inventoryServices + Offsets::m_unMusicID, skinManager->MusicKit.id);

            bool ShouldUpdate = false;
            const std::vector<uintptr_t> weapons = GetWeapons(localPlayer);
            if (weapons.empty()) { Sleep(200); continue; }

            // Apply weapon skins
            for (const uintptr_t& weapon : weapons)
            {
                if (!weapon) continue;

                const uintptr_t item    = weapon + Offsets::m_AttributeManager + Offsets::m_Item;
                const uint16_t defIndex = mem.Read<uint16_t>(item + Offsets::m_iItemDefinitionIndex);

                const bool isKnife = (defIndex == 42 || defIndex == 59 ||
                                      (defIndex >= 500 && defIndex <= 526));
                if (isKnife) continue;

                if (ForceUpdate)
                    mem.Write<uint32_t>(item + Offsets::m_iItemIDHigh, 0);

                if (mem.Read<uint32_t>(item + Offsets::m_iItemIDHigh) == static_cast<uint32_t>(-1))
                    continue;

                mem.Write<uint32_t>(item + Offsets::m_iItemIDHigh, static_cast<uint32_t>(-1));

                SkinInfo_t skin = GetSkin(item);
                if (!skin.Paint) continue;

                mem.Write<uint32_t>(weapon + Offsets::m_nFallbackPaintKit, skin.Paint);

                const uint64_t   mask      = skin.bUsesOldModel + 1;
                const uintptr_t  hudWeapon = GetHudWeapon(weapon);
                if (hudWeapon)
                {
                    SetMeshMask(weapon, mask);
                    SetMeshMask(hudWeapon, mask);
                }

                econItemAttributeManager.Create(item, skin);
                ShouldUpdate = true;
            }

            if (ShouldUpdate || ForceUpdate)
                UpdateWeapons(weapons);

            ForceUpdate = false;

            if (ShouldUpdate)
                configManager->AutoSaveDebounced();
        }
        catch (...) { Sleep(1000); }
    }

    return 0;
}
