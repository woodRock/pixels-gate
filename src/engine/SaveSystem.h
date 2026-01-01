#pragma once
#include "ECS.h"
#include "Tilemap.h"
#include "Components.h"
#include "Inventory.h"
#include "UIComponents.h"
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

namespace PixelsEngine {

    class SaveSystem {
    public:
        static void SaveGame(const std::string& filename, Registry& registry, Entity player, const Tilemap& map) {
            std::cout << "Starting SaveGame to " << filename << std::endl;
            std::ofstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open save file: " << filename << std::endl;
                return;
            }

            // 1. Player Transform
            std::cout << "Saving Player Transform..." << std::endl;
            if (auto* trans = registry.GetComponent<TransformComponent>(player)) {
                file << "[PLAYER_TRANSFORM]\n";
                file << trans->x << " " << trans->y << "\n";
            }

            // 2. Player Stats
            std::cout << "Saving Player Stats..." << std::endl;
            if (auto* stats = registry.GetComponent<StatsComponent>(player)) {
                file << "[PLAYER_STATS]\n";
                file << stats->strength << " " << stats->dexterity << " " << stats->constitution << " " 
                     << stats->intelligence << " " << stats->wisdom << " " << stats->charisma << "\n";
                file << stats->maxHealth << " " << stats->currentHealth << " " << stats->damage << " " << stats->inspiration << "\n";
                file << stats->characterClass << "\n";
                file << stats->race << "\n";
            }

            // 3. Inventory
            std::cout << "Saving Inventory..." << std::endl;
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

            // 4. Quest States
            std::cout << "Saving Quests..." << std::endl;
            file << "[QUESTS]\n";
            auto& quests = registry.View<QuestComponent>();
            file << quests.size() << "\n";
            for (auto& [entity, quest] : quests) {
                file << quest.questId << " " << quest.state << "\n";
            }
            if (file.fail()) std::cerr << "Error writing quests!" << std::endl;

            // 5. World Entities
            std::cout << "Saving World Entities..." << std::endl;
            file << "[WORLD_ENTITIES]\n";
            auto& transforms = registry.View<TransformComponent>();
            int entityCount = 0;
            for (auto& [entity, trans] : transforms) {
                if (entity != player) entityCount++;
            }
            std::cout << "Entity Count: " << entityCount << std::endl;
            file << entityCount << "\n";
            
            for (auto& [entity, trans] : transforms) {
                if (entity == player) continue;
                
                std::string uniqueId = "";
                std::string dialogueText = "";
                if (auto* inter = registry.GetComponent<InteractionComponent>(entity)) {
                    uniqueId = inter->uniqueId;
                    dialogueText = inter->dialogueText;
                }
                
                file << uniqueId << "\n";
                file << dialogueText << "\n";
                file << trans.x << " " << trans.y << "\n";
                
                // Stats
                if (auto* s = registry.GetComponent<StatsComponent>(entity)) {
                    file << "1\n"; 
                    file << s->maxHealth << " " << s->currentHealth << " " << s->damage << " " << (s->isDead ? 1 : 0) << "\n";
                } else file << "0\n";

                // AI
                if (auto* ai = registry.GetComponent<AIComponent>(entity)) {
                    file << "1\n";
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

                // Dialogue
                if (auto* diag = registry.GetComponent<DialogueComponent>(entity)) {
                    file << "1\n";
                    file << diag->tree.currentNodeId << "\n";
                    file << diag->tree.nodes.size() << "\n";
                    for (const auto& [id, node] : diag->tree.nodes) {
                        file << id << "\n";
                        file << node.options.size() << "\n";
                        for (const auto& opt : node.options) {
                            file << (opt.hasBeenChosen ? 1 : 0) << " ";
                        }
                        file << "\n";
                    }
                } else file << "0\n";

                // Lock
                if (auto* lock = registry.GetComponent<LockComponent>(entity)) {
                    file << "1\n";
                    file << (lock->isLocked ? 1 : 0) << "\n";
                } else file << "0\n";

                // Loot
                if (auto* loot = registry.GetComponent<LootComponent>(entity)) {
                    file << "1\n";
                    file << loot->drops.size() << "\n";
                    for (const auto& item : loot->drops) {
                        file << item.name << "\n" << item.iconPath << "\n" 
                             << item.quantity << " " << (int)item.type << " " << item.statBonus << " " << item.value << "\n";
                    }
                } else file << "0\n";
            }
            if (file.fail()) std::cerr << "Error writing entities!" << std::endl;

            // 6. Fog of War
            std::cout << "Saving Fog..." << std::endl;
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

            file.flush();
            file.close();
            std::cout << "Game Saved to " << filename << " successfully." << std::endl;
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

            bool worldEntitiesFound = false;
            
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
                        int count = 0;
                        file >> count;
                        file.ignore(); // Consume newline
                        for (int i = 0; i < count; ++i) {
                            std::string name, icon;
                            int qty, type, bonus, val;
                            if (!(file >> std::ws && std::getline(file, name))) break;
                            if (!(file >> std::ws && std::getline(file, icon))) break;
                            
                            if (!icon.empty() && std::isdigit(icon[0]) && icon.find('.') == std::string::npos) {
                                std::stringstream ss(icon);
                                ss >> qty;
                                icon = "";
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
                    int count = 0;
                    file >> count;
                    std::unordered_map<std::string, int> questStates;
                    for (int i = 0; i < count; ++i) {
                        std::string id;
                        int state;
                        file >> id >> state;
                        questStates[id] = state;
                    }
                    auto& quests = registry.View<QuestComponent>();
                    for (auto& [entity, quest] : quests) {
                        if (questStates.find(quest.questId) != questStates.end()) {
                            quest.state = questStates[quest.questId];
                        }
                    }
                }
                else if (line == "[WORLD_ENTITIES]") {
                    worldEntitiesFound = true;
                    int count = 0;
                    file >> count;
                    file.ignore(); 
                    
                    auto& worldTransforms = registry.View<TransformComponent>();
                    std::unordered_map<std::string, Entity> existingEntitiesById;
                    std::vector<Entity> genericEntities; 
                    
                    for (auto& [ent, t] : worldTransforms) {
                        if (ent == player) continue;
                        if (auto* inter = registry.GetComponent<InteractionComponent>(ent)) {
                            if (!inter->uniqueId.empty()) {
                                existingEntitiesById[inter->uniqueId] = ent;
                            } else {
                                genericEntities.push_back(ent);
                            }
                        } else {
                            genericEntities.push_back(ent);
                        }
                    }

                    std::unordered_map<Entity, bool> updatedEntities;
                    int genericIndex = 0;

                    for (int i = 0; i < count; ++i) {
                        std::string uniqueId, dialogueText;
                        std::getline(file, uniqueId);
                        std::getline(file, dialogueText);

                        float ex, ey;
                        file >> ex >> ey;
                        
                        Entity target = INVALID_ENTITY;
                        if (!uniqueId.empty() && existingEntitiesById.count(uniqueId)) {
                            target = existingEntitiesById[uniqueId];
                        } 
                        if (target == INVALID_ENTITY && uniqueId.empty() && genericIndex < genericEntities.size()) {
                            target = genericEntities[genericIndex++];
                        }

                        if (target != INVALID_ENTITY) {
                            updatedEntities[target] = true;
                            if (auto* t = registry.GetComponent<TransformComponent>(target)) {
                                t->x = ex; t->y = ey;
                            }
                            if (auto* inter = registry.GetComponent<InteractionComponent>(target)) {
                                inter->dialogueText = dialogueText;
                            }
                            int hasStats; file >> hasStats;
                            if (hasStats) {
                                auto* s = registry.GetComponent<StatsComponent>(target);
                                int dead;
                                if (s) file >> s->maxHealth >> s->currentHealth >> s->damage >> dead;
                                else { int dummy; file >> dummy >> dummy >> dummy >> dead; }
                                if (s) s->isDead = (dead == 1);
                            }
                            int hasAI; file >> hasAI;
                            if (hasAI) {
                                auto* ai = registry.GetComponent<AIComponent>(target);
                                int aggro; float face;
                                file >> aggro >> face;
                                if (ai) { ai->isAggressive = (aggro == 1); ai->facingDir = face; }
                            }
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
                            int hasDiag; file >> hasDiag;
                            if (hasDiag) {
                                file.ignore(); // Consume newline after hasDiag
                                std::string nodeId;
                                std::getline(file, nodeId);
                                int nodeCount = 0; file >> nodeCount;
                                std::unordered_map<std::string, std::vector<int>> nodeOptionStates;
                                for(int k=0; k<nodeCount; ++k) {
                                    std::string nId;
                                    file >> std::ws; std::getline(file, nId);
                                    int optCount = 0; file >> optCount;
                                    std::vector<int> states;
                                    for(int m=0; m<optCount; ++m) { int s; file >> s; states.push_back(s); }
                                    nodeOptionStates[nId] = states;
                                }
                                if (auto* diag = registry.GetComponent<DialogueComponent>(target)) {
                                    diag->tree.currentNodeId = nodeId;
                                    for (auto& [nId, states] : nodeOptionStates) {
                                        if (diag->tree.nodes.find(nId) != diag->tree.nodes.end()) {
                                            auto& node = diag->tree.nodes[nId];
                                            for(size_t m=0; m<states.size() && m<node.options.size(); ++m) {
                                                node.options[m].hasBeenChosen = (states[m] == 1);
                                            }
                                        }
                                    }
                                }
                            }
                            int hasLock; file >> hasLock;
                            if (hasLock) {
                                int locked; file >> locked;
                                if (auto* lock = registry.GetComponent<LockComponent>(target)) {
                                    lock->isLocked = (locked == 1);
                                }
                            }
                            int hasLoot; file >> hasLoot;
                            file.ignore(); // Consume newline after hasLoot (whether 0 or 1)
                            if (hasLoot) {
                                int itemCount = 0; file >> itemCount;
                                file.ignore();
                                auto* loot = registry.GetComponent<LootComponent>(target);
                                if (!loot) loot = &registry.AddComponent(target, LootComponent{});
                                loot->drops.clear();
                                for(int k=0; k<itemCount; ++k) {
                                    std::string iname, icon;
                                    int qty, type, bonus, val;
                                    std::getline(file, iname);
                                    std::getline(file, icon);
                                    file >> qty >> type >> bonus >> val;
                                    file.ignore();
                                    loot->drops.push_back({iname, icon, qty, (ItemType)type, bonus, val});
                                }
                            }
                        } else {
                            int hasStats; file >> hasStats; if(hasStats) { int d; file >> d >> d >> d >> d; }
                            int hasAI; file >> hasAI; if(hasAI) { float f; int d; file >> d >> f; }
                            int skipInvCount; file >> skipInvCount; file.ignore();
                            for(int j=0; j<skipInvCount; ++j) { 
                                std::string s; int d; 
                                std::getline(file, s); std::getline(file, s);
                                file >> d >> d >> d >> d; file.ignore(); 
                            }
                            int hasDiag; file >> hasDiag;
                            if (hasDiag) {
                                std::string s; 
                                file >> std::ws; std::getline(file, s); 
                                int nodeCount; file >> nodeCount;
                                for(int k=0; k<nodeCount; ++k) {
                                    file >> std::ws; std::getline(file, s); 
                                    int optCount; file >> optCount;
                                    for(int m=0; m<optCount; ++m) { int d; file >> d; }
                                }
                            }
                            int hasLock; file >> hasLock;
                            if (hasLock) { int d; file >> d; }
                            int hasLoot; file >> hasLoot; file.ignore();
                            if (hasLoot) {
                                int cnt; file >> cnt; file.ignore();
                                for(int k=0; k<cnt; ++k) {
                                    std::string s; std::getline(file, s); std::getline(file, s);
                                    int d; file >> d >> d >> d >> d; file.ignore();
                                }
                            }
                        }
                    }

                    for (auto& [id, ent] : existingEntitiesById) {
                        if (updatedEntities.find(ent) == updatedEntities.end()) {
                            registry.DestroyEntity(ent);
                        }
                    }
                }
                else if (line == "[MAP_FOG]") {
                    int w, h;
                    file >> w >> h;
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

} // namespace PixelsEngine
