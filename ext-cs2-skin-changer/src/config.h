#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include "StateSync.hpp"
#include "SDK/weapon/C_EconEntity.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

void SyncKnifeUiState();
void SyncSelectedKnifeIndexFromKnifeDef();

// Logs detalhados ([APPLY] / [UPDATE]) — defina EXT_CS2_SKIN_VERBOSE=1 no ambiente para ativar.
inline bool SkinChangerVerboseLogs()
{
#ifdef _WIN32
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, "EXT_CS2_SKIN_VERBOSE") != 0 || !buf)
        return false;
    const bool on = (buf[0] == '1' && buf[1] == '\0');
    free(buf);
    return on;
#else
    const char* v = std::getenv("EXT_CS2_SKIN_VERBOSE");
    return v && v[0] == '1' && v[1] == '\0';
#endif
}

class CConfigManager
{
private:
    std::filesystem::path configDir;

public:
    void Setup()
    {
        const fs::path legacyConfigDir = L"C:\\Skin2Merde\\";
        char* localAppData = nullptr;
        size_t len = 0;
        if (_dupenv_s(&localAppData, &len, "LOCALAPPDATA") == 0 && localAppData && *localAppData)
        {
            configDir = fs::path(localAppData) / "ext-cs2-skin-changer";
        }
        else
        {
            configDir = fs::current_path() / "ext-cs2-skin-changer";
        }

        if (localAppData)
            free(localAppData);

        configDir /= "configs";
        fs::create_directories(configDir);

        if (fs::exists(legacyConfigDir))
        {
            for (const auto& entry : fs::directory_iterator(legacyConfigDir))
            {
                if (!entry.is_regular_file() || entry.path().extension() != ".json")
                    continue;

                const fs::path target = configDir / entry.path().filename();
                if (fs::exists(target))
                    continue;

                std::error_code ec;
                fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing, ec);
                if (!ec)
                    std::cout << "[config] migrated " << entry.path().filename().string() << "\n";
            }
        }
    }

    std::vector<std::string> GetConfigs()
    {
        std::vector<std::string> configs;
        if (!fs::exists(configDir))
            return configs;

        for (const auto& entry : fs::directory_iterator(configDir))
        {
            if (entry.path().extension() == ".json")
            {
                configs.push_back(entry.path().stem().string());
            }
        }
        return configs;
    }

    void Save(const std::string& name)
    {
        if (name.empty()) return;

        json j;

        // Skins
        for (const auto& skin : skinManager->Skins)
        {
            json s;
            s["weapon"] = (int)skin.weaponType;
            s["paint"] = skin.Paint;
            s["rarity"] = skin.rarity;
            s["name"] = skin.name;
            s["image"] = skin.image_url;
            
            j["skins"].push_back(s);
        }

        // Music Kit
        j["musickit_id"] = skinManager->MusicKit.id;
        j["musickit_name"] = skinManager->MusicKit.name;

        // Knife
        j["knife_def"] = skinManager->Knife.defIndex;
        j["knife_name"] = skinManager->Knife.name;
        j["knife_model"] = skinManager->Knife.model;

        // Gloves
        j["glove_def"] = skinManager->ActiveGlove.defIndex;
        j["glove_paint"] = skinManager->ActiveGlove.Paint;
        j["glove_name"] = skinManager->ActiveGlove.name;

        // Settings
        j["disk_cache_enabled"] = diskCacheEnabled;

        // Write
        const fs::path path = configDir / (name + ".json");
        std::ofstream o(path);
        o << std::setw(4) << j << std::endl;
    }

    // Nome RemoveConfig em vez de Delete — evita colisão com macros do Windows SDK.
    bool RemoveConfig(const std::string& name)
    {
        if (name.empty())
            return false;
        if (name.find("..") != std::string::npos)
            return false;
        if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos)
            return false;

        const fs::path path = configDir / (name + ".json");
        if (!fs::exists(path))
        {
            std::cout << "[config] delete: missing " << path.string() << "\n";
            return false;
        }

        std::error_code ec;
        fs::remove(path, ec);
        if (ec)
        {
            std::cout << "[config] delete failed " << path.string() << " (" << ec.message() << ")\n";
            return false;
        }
        std::cout << "[config] deleted " << path.string() << "\n";
        return true;
    }

    void Load(const std::string& name)
    {
        if (name.empty()) return;
        const fs::path path = configDir / (name + ".json");
        
        if (!fs::exists(path))
        {
            std::cout << "[config] missing " << path.string() << "\n";
            return;
        }

        try {
            std::ifstream i(path);
            json j;
            i >> j;

            skinManager->Skins.clear();

            if (j.find("skins") != j.end())
            {
                for (auto& element : j["skins"])
                {
                    SkinInfo_t skin;
                    skin.weaponType = (WeaponsEnum)element["weapon"].get<int>();
                    skin.Paint = element["paint"].get<int>();
                    

                    if (element.find("rarity") != element.end()) skin.rarity = element["rarity"].get<int>();
                    else skin.rarity = 1;
                    
                    if (element.find("name") != element.end()) skin.name = element["name"].get<std::string>();
                    else skin.name = "Unknown";
                    
                    if (element.find("image") != element.end()) skin.image_url = element["image"].get<std::string>();
                    
                    skinManager->AddSkin(skin);
                }
            }
            
            if (j.find("musickit_id") != j.end()) {
                skinManager->MusicKit.id = j["musickit_id"].get<uint16_t>();
                if (j.find("musickit_name") != j.end())
                    skinManager->MusicKit.name = j["musickit_name"].get<std::string>();
            }

            if (j.find("knife_def") != j.end()) {
                const uint16_t knifeDef = j["knife_def"].get<uint16_t>();
                skinManager->Knife.defIndex = KnifeNames.find(knifeDef) != KnifeNames.end() ? knifeDef : 0;
                if (j.find("knife_name") != j.end())
                    skinManager->Knife.name = j["knife_name"].get<std::string>();
                else if (KnifeNames.find(knifeDef) != KnifeNames.end())
                    skinManager->Knife.name = KnifeNames[knifeDef];

                if (j.find("knife_model") != j.end())
                    skinManager->Knife.model = j["knife_model"].get<std::string>();
                else if (KnifeModels.find(knifeDef) != KnifeModels.end())
                    skinManager->Knife.model = KnifeModels[knifeDef];

                SyncSelectedKnifeIndexFromKnifeDef();
            }

            if (j.find("glove_def") != j.end()) {
                skinManager->ActiveGlove.defIndex = j["glove_def"].get<uint16_t>();
            }
            if (j.find("glove_paint") != j.end()) {
                skinManager->ActiveGlove.Paint = j["glove_paint"].get<int>();
            }
            if (j.find("glove_name") != j.end()) {
                skinManager->ActiveGlove.name = j["glove_name"].get<std::string>();
            }

            if (j.find("disk_cache_enabled") != j.end()) {
                diskCacheEnabled = j["disk_cache_enabled"].get<bool>();
            }

            SyncKnifeUiState();
            RequestForceUpdate();
            std::cout << "[config] loaded " << path.string() << "\n";
        } catch (...) {
            std::cout << "[config] failed to load " << path.string() << "\n";
        }
    }
    
    // Auto Logic
    void AutoSave() { Save("default"); }
    // Evita gravar default.json a cada frame quando várias armas atualizam de uma vez (disco + SSD).
    void AutoSaveDebounced(const std::chrono::milliseconds minInterval = std::chrono::milliseconds(750))
    {
        using clock = std::chrono::steady_clock;
        static clock::time_point last{};
        const auto now = clock::now();
        if (now - last < minInterval)
            return;
        last = now;
        Save("default");
    }
    void AutoLoad() { Load("default"); }

    // Settings
    bool diskCacheEnabled = true;
};

inline CConfigManager* configManager = new CConfigManager();

