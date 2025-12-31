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
                    file << item.quantity << " " << (int)item.type << " " << item.statBonus << " " << item.value << "\n";
                }
                // Save Equipment too!
                file << "[EQUIPMENT]\n";
                auto saveEquip = [&](const Item& item) {
                    file << item.name << "\n" << item.iconPath << "\n" << item.quantity << " " << (int)item.type << " " << item.statBonus << " " << item.value << "\n";
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

            // 5. World Entities (NPCs and Mobs)
            file << "[WORLD_ENTITIES]\n";
            auto& transforms = registry.View<TransformComponent>();
            int entityCount = 0;
            for (auto& [entity, trans] : transforms) {
                if (entity != player) entityCount++;
            }
            file << entityCount << "\n";
            for (auto& [entity, trans] : transforms) {
                if (entity == player) continue;
                
                // Identify entity by its interaction/tag name for reloading
                std::string name = "Unknown";
                if (auto* inter = registry.GetComponent<InteractionComponent>(entity)) name = inter->dialogueText;
                
                file << name << "\n";
                file << trans.x << " " << trans.y << "\n";
                
                // Stats
                if (auto* s = registry.GetComponent<StatsComponent>(entity)) {
                    file << "1\n"; // Has stats
                    file << s->maxHealth << " " << s->currentHealth << " " << s->damage << " " << (s->isDead ? 1 : 0) << "\n";
                } else file << "0\n";

                // AI
                if (auto* ai = registry.GetComponent<AIComponent>(entity)) {
                    file << "1\n"; // Has AI
                    file << (ai->isAggressive ? 1 : 0) << " " << ai->facingDir << "\n";
                } else file << "0\n";

                // Inventory
                if (auto* inv = registry.GetComponent<InventoryComponent>(entity)) {
                    file << inv->items.size() << "\n";
                    for (const auto& item : inv->items) {
                        file << item.name << "\n" << item.iconPath << "\n" 
                             << item.quantity << " " << (int)item.type << " " << item.statBonus << " " << item.value << "\n";
                    }
                } else file << "0\n";
            }

            // 6. Fog of War
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

        static void SaveWorldFlags(const std::string& filename, const std::unordered_map<std::string, bool>& flags) {
            std::ofstream file(filename + ".flags");
            if (file.is_open()) {
                file << flags.size() << "\n";
                for (auto const& [key, val] : flags) {
                    file << key << " " << (val ? 1 : 0) << "\n";
                }
                file.close();
            }
        }

        static void LoadWorldFlags(const std::string& filename, std::unordered_map<std::string, bool>& flags) {
            std::ifstream file(filename + ".flags");
            if (file.is_open()) {
                flags.clear();
                size_t size;
                file >> size;
                for (size_t i = 0; i < size; ++i) {
                    std::string key;
                    int val;
                    file >> key >> val;
                    flags[key] = (val == 1);
                }
                file.close();
            }
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
                            int qty, type, bonus, val;
                            if (!(file >> std::ws && std::getline(file, name))) break;
                            if (!(file >> std::ws && std::getline(file, icon))) break;
                            
                            // Basic validation: if icon looks like a number, it's probably an old save format
                            if (!icon.empty() && std::isdigit(icon[0]) && icon.find('.') == std::string::npos) {
                                // This is likely an old save where the next line was actually 'qty'
                                std::stringstream ss(icon);
                                ss >> qty;
                                icon = ""; // Reset icon
                                // Try to read remaining fields from current line
                                // But simpler to just skip this item or handle carefully
                                // For now, let's just use defaults for old saves
                                type = (int)ItemType::Misc;
                                bonus = 0;
                                val = 10;
                            } else {
                                if (!(file >> qty >> type >> bonus >> val)) break;
                                file.ignore();
                            }
                            inv->AddItem(name, qty, (ItemType)type, bonus, icon, val);
                        }
                    }
                }
                else if (line == "[EQUIPMENT]") {
                    if (auto* inv = registry.GetComponent<InventoryComponent>(player)) {
                        auto loadEquip = [&](Item& item) {
                            std::getline(file, item.name);
                            std::getline(file, item.iconPath);
                            int qty, type, bonus, val;
                            file >> qty >> type >> bonus >> val;
                            file.ignore();
                            item.quantity = qty; item.type = (ItemType)type; item.statBonus = bonus; item.value = val;
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
                else if (line == "[WORLD_ENTITIES]") {
                    int count;
                    file >> count;
                    file.ignore();
                    
                    // Pre-cache existing world entities to update them
                    auto& worldTransforms = registry.View<TransformComponent>();
                    std::unordered_map<std::string, Entity> existingEntities;
                    for (auto& [ent, t] : worldTransforms) {
                        if (ent == player) continue;
                        if (auto* inter = registry.GetComponent<InteractionComponent>(ent)) {
                            existingEntities[inter->dialogueText] = ent;
                        }
                    }

                    for (int i = 0; i < count; ++i) {
                        std::string name;
                        std::getline(file, name);
                        float ex, ey;
                        file >> ex >> ey;
                        
                        Entity target = INVALID_ENTITY;
                        if (existingEntities.count(name)) {
                            target = existingEntities[name];
                        }

                        if (target != INVALID_ENTITY) {
                            if (auto* t = registry.GetComponent<TransformComponent>(target)) {
                                t->x = ex; t->y = ey;
                            }
                            
                            // Load Stats
                            int hasStats; file >> hasStats;
                            if (hasStats) {
                                auto* s = registry.GetComponent<StatsComponent>(target);
                                int dead;
                                if (s) file >> s->maxHealth >> s->currentHealth >> s->damage >> dead;
                                else { int dummy; file >> dummy >> dummy >> dummy >> dead; }
                                if (s) s->isDead = (dead == 1);
                            }

                            // Load AI
                            int hasAI; file >> hasAI;
                            if (hasAI) {
                                auto* ai = registry.GetComponent<AIComponent>(target);
                                int aggro; float face;
                                file >> aggro >> face;
                                if (ai) { ai->isAggressive = (aggro == 1); ai->facingDir = face; }
                            }

                            // Load Inventory
                            int invCount; file >> invCount;
                            file.ignore();
                            if (invCount > 0 || registry.HasComponent<InventoryComponent>(target)) {
                                auto* inv = registry.GetComponent<InventoryComponent>(target);
                                if (inv) inv->items.clear();
                                for (int j = 0; j < invCount; ++j) {
                                    std::string iname, icon;
                                    int qty, type, bonus, val;
                                    std::getline(file, iname);
                                    std::getline(file, icon);
                                    file >> qty >> type >> bonus >> val;
                                    file.ignore();
                                    if (inv) inv->AddItem(iname, qty, (ItemType)type, bonus, icon, val);
                                }
                            }
                        } else {
                            // If entity doesn't exist in current scene, we might need to spawn it, 
                            // but for this simple engine we assume all persistent NPCs are created in OnStart.
                            // We'll skip the extra data for now to keep the file pointer in sync.
                            int hasStats; file >> hasStats; if(hasStats) { int d; file >> d >> d >> d >> d; }
                            int hasAI; file >> hasAI; if(hasAI) { float f; int d; file >> d >> f; }
                            int skipInvCount; file >> skipInvCount; file.ignore();
                            for(int j=0; j<skipInvCount; ++j) { 
                                std::string s; int d; 
                                std::getline(file, s); // name
                                std::getline(file, s); // icon
                                file >> d >> d >> d >> d; // qty, type, bonus, val
                                file.ignore(); 
                            }
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
