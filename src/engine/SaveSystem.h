#pragma once
#include "ECS.h"
#include "Tilemap.h"
#include "Components.h"
#include "Inventory.h"
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

namespace PixelsEngine {

    class SaveSystem {
    public:
        static void SaveGame(const std::string& filename, Registry& registry, Entity player, const Tilemap& map) {
            std::ofstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open save file: " << filename << std::endl;
                return;
            }

            // 1. Player Transform
            if (auto* trans = registry.GetComponent<TransformComponent>(player)) {
                file << "[PLAYER_TRANSFORM]\n";
                file << trans->x << " " << trans->y << "\n";
            }

            // 2. Player Stats
            if (auto* stats = registry.GetComponent<StatsComponent>(player)) {
                file << "[PLAYER_STATS]\n";
                file << stats->strength << " " << stats->dexterity << " " << stats->constitution << " " 
                     << stats->intelligence << " " << stats->wisdom << " " << stats->charisma << "\n";
                file << stats->maxHealth << " " << stats->currentHealth << " " << stats->damage << " " << stats->inspiration << "\n";
                file << stats->characterClass << "\n";
                file << stats->race << "\n";
            }

            // 3. Inventory
            if (auto* inv = registry.GetComponent<InventoryComponent>(player)) {
                file << "[INVENTORY]\n";
                file << inv->items.size() << "\n";
                for (const auto& item : inv->items) {
                    file << item.name << "\n";
                    file << item.iconPath << "\n";
                    file << item.quantity << " " << (int)item.type << " " << item.statBonus << "\n";
                }
                // Save Equipment too!
                file << "[EQUIPMENT]\n";
                auto saveEquip = [&](const Item& item) {
                    file << item.name << "\n" << item.iconPath << "\n" << item.quantity << " " << (int)item.type << " " << item.statBonus << "\n";
                };
                saveEquip(inv->equippedMelee);
                saveEquip(inv->equippedRanged);
                saveEquip(inv->equippedArmor);
            }

            // 4. Quest States (Iterate all QuestComponents)
            // We need to identify WHICH npc has which quest. 
            // For now, we assume unique Quest IDs map 1:1 to NPCs in the level.
            file << "[QUESTS]\n";
            auto& quests = registry.View<QuestComponent>();
            file << quests.size() << "\n";
            for (auto& [entity, quest] : quests) {
                file << quest.questId << " " << quest.state << "\n";
            }

            // 5. Fog of War
            file << "[MAP_FOG]\n";
            int w = map.GetWidth();
            int h = map.GetHeight();
            file << w << " " << h << "\n";
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    file << (int)(map.IsExplored(x, y) ? (map.IsVisible(x, y) ? 2 : 1) : 0) << " ";
                }
                file << "\n";
            }

            file.close();
            std::cout << "Game Saved to " << filename << std::endl;
        }

        static void LoadGame(const std::string& filename, Registry& registry, Entity player, Tilemap& map) {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open save file: " << filename << std::endl;
                return;
            }

            std::string line;
            while (std::getline(file, line)) {
                if (line == "[PLAYER_TRANSFORM]") {
                    if (auto* trans = registry.GetComponent<TransformComponent>(player)) {
                        file >> trans->x >> trans->y;
                    }
                }
                else if (line == "[PLAYER_STATS]") {
                    if (auto* stats = registry.GetComponent<StatsComponent>(player)) {
                        file >> stats->strength >> stats->dexterity >> stats->constitution 
                             >> stats->intelligence >> stats->wisdom >> stats->charisma;
                        file >> stats->maxHealth >> stats->currentHealth >> stats->damage >> stats->inspiration;
                        file.ignore(); // Consume newline
                        std::getline(file, stats->characterClass);
                        std::getline(file, stats->race);
                    }
                }
                else if (line == "[INVENTORY]") {
                    if (auto* inv = registry.GetComponent<InventoryComponent>(player)) {
                        inv->items.clear();
                        int count;
                        file >> count;
                        file.ignore(); // Consume newline
                        for (int i = 0; i < count; ++i) {
                            std::string name, icon;
                            int qty, type, bonus;
                            std::getline(file, name);
                            std::getline(file, icon);
                            file >> qty >> type >> bonus;
                            file.ignore();
                            inv->AddItem(name, qty, (ItemType)type, bonus, icon);
                        }
                    }
                }
                else if (line == "[EQUIPMENT]") {
                    if (auto* inv = registry.GetComponent<InventoryComponent>(player)) {
                        auto loadEquip = [&](Item& item) {
                            std::getline(file, item.name);
                            std::getline(file, item.iconPath);
                            int qty, type, bonus;
                            file >> qty >> type >> bonus;
                            file.ignore();
                            item.quantity = qty; item.type = (ItemType)type; item.statBonus = bonus;
                        };
                        loadEquip(inv->equippedMelee);
                        loadEquip(inv->equippedRanged);
                        loadEquip(inv->equippedArmor);
                    }
                }
                else if (line == "[QUESTS]") {
                    int count;
                    file >> count;
                    // Read into map for quick lookup
                    std::unordered_map<std::string, int> questStates;
                    for (int i = 0; i < count; ++i) {
                        std::string id;
                        int state;
                        file >> id >> state;
                        questStates[id] = state;
                    }
                    
                    // Apply to entities
                    auto& quests = registry.View<QuestComponent>();
                    for (auto& [entity, quest] : quests) {
                        if (questStates.find(quest.questId) != questStates.end()) {
                            quest.state = questStates[quest.questId];
                        }
                    }
                }
                else if (line == "[MAP_FOG]") {
                    int w, h;
                    file >> w >> h;
                    // Need to expose a method in Tilemap to SetVisibility
                    // We will read the ints and set them.
                    std::vector<int> fogData;
                    fogData.resize(w * h);
                    for (int i = 0; i < w * h; ++i) {
                        file >> fogData[i];
                    }
                    map.LoadFog(fogData);
                }
            }

            file.close();
            std::cout << "Game Loaded from " << filename << std::endl;
        }
    };

}
