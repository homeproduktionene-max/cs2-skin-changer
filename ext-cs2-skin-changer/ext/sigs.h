#include <iostream>

#include "mem.h"

#pragma once

namespace Sigs
{
	// Primary signature for RegenerateWeaponSkins
	inline uintptr_t FindRegenerateWeaponSkins()
	{
		// Try primary signature
		uintptr_t addr = mem.SigScan(L"client.dll", "48 83 EC ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 10");
		if (addr) {
			std::cout << "[SIG] Found RegenerateWeaponSkins (primary pattern)\n";
			return addr;
		}

		// Try alternative signature 1
		addr = mem.SigScan(L"client.dll", "48 89 5C 24 ? 57 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 85 C0");
		if (addr) {
			std::cout << "[SIG] Found RegenerateWeaponSkins (alternative pattern 1)\n";
			return addr;
		}

		// Try alternative signature 2
		addr = mem.SigScan(L"client.dll", "40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 85 C0 74 ? 48 8B 10");
		if (addr) {
			std::cout << "[SIG] Found RegenerateWeaponSkins (alternative pattern 2)\n";
			return addr;
		}

		std::cerr << "[SIG] Failed to find RegenerateWeaponSkins with any pattern!\n";
		return 0;
	}

	const auto RegenerateWeaponSkins = FindRegenerateWeaponSkins();
}