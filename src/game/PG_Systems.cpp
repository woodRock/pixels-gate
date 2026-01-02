#include "PixelsGateGame.h"
#include "../engine/Dice.h"
#include "../engine/Input.h"
#include "../engine/TextureManager.h"
#include <cmath>
#include <algorithm> 

void PixelsGateGame::UpdateAI(float deltaTime) {
    auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    if (!pTrans) return;

    auto &view = GetRegistry().View<PixelsEngine::AIComponent>();
    for (auto &[entity, ai] : view) {
        if (m_State == GameState::Combat && IsInTurnOrder(entity)) continue;
        auto *transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
        if (!transform) continue;

        auto *stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(entity);
        if (stats && stats->isDead) continue; // Safety check

        if (ai.hostileTimer > 0.0f) {
            ai.hostileTimer -= deltaTime;
            if (ai.hostileTimer <= 0.0f) ai.isAggressive = false;
        }

        float dist = std::sqrt(std::pow(pTrans->x - transform->x, 2) + std::pow(pTrans->y - transform->y, 2));
        if (ai.isAggressive && dist <= ai.sightRange) {
            if (dist > ai.attackRange) {
                float dx = pTrans->x - transform->x; float dy = pTrans->y - transform->y;
                float len = std::sqrt(dx*dx + dy*dy);
                if (len > 0) {
                    transform->x += (dx/len) * 2.0f * deltaTime;
                    transform->y += (dy/len) * 2.0f * deltaTime;
                }
            } else {
                ai.attackTimer -= deltaTime;
                if (ai.attackTimer <= 0.0f) {
                    auto *pStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
                    if (pStats) {
                        pStats->currentHealth -= 2;
                        SpawnFloatingText(pTrans->x, pTrans->y, "-2", {255,0,0,255});
                        ai.attackTimer = ai.attackCooldown;
                        StartCombat(entity);
                    }
                }
            }
        }
    }
}

void PixelsGateGame::UpdateMovement(float deltaTime) {
    auto &view = GetRegistry().View<PixelsEngine::PathMovementComponent>();
    for (auto &[entity, pathComp] : view) {
        if (!pathComp.isMoving || pathComp.path.empty()) continue;

        auto *trans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
        auto *pc = GetRegistry().GetComponent<PixelsEngine::PlayerComponent>(entity);
        if (!trans) continue;

        float speed = pc ? pc->speed : 3.0f;
        
        float targetX = (float)pathComp.path[pathComp.currentPathIndex].first;
        float targetY = (float)pathComp.path[pathComp.currentPathIndex].second;

        float dx = targetX - trans->x;
        float dy = targetY - trans->y;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < 0.05f) {
            trans->x = targetX;
            trans->y = targetY;
            pathComp.currentPathIndex++;
            if (pathComp.currentPathIndex >= pathComp.path.size()) {
                pathComp.isMoving = false;
                pathComp.path.clear();
                pathComp.currentPathIndex = 0;
            }
        } else {
            float move = speed * deltaTime;
            if (move > dist) move = dist;

            // Combat movement restriction
            if (m_State == GameState::Combat && entity == m_Player) {
                if (m_Combat.m_MovementLeft < move) move = m_Combat.m_MovementLeft;
                if (move <= 0) {
                    pathComp.isMoving = false;
                    pathComp.path.clear();
                    continue;
                }
                m_Combat.m_MovementLeft -= move;
            }

            trans->x += (dx / dist) * move;
            trans->y += (dy / dist) * move;
        }
    }
}

void PixelsGateGame::UpdateDayNight(float deltaTime) {
    m_Time.Update(deltaTime);
}

void PixelsGateGame::RenderDayNightCycle() {
    SDL_Color tint = {0,0,0,0};
    if (m_Time.m_TimeOfDay < 5.0f || m_Time.m_TimeOfDay > 20.0f) tint = {10,10,50,100};
    else if (m_Time.m_TimeOfDay < 8.0f) tint = {255,100,50,50};
    
    if (tint.a > 0) {
        SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(GetRenderer(), tint.r, tint.g, tint.b, tint.a);
        SDL_Rect s = {0,0,GetWindowWidth(),GetWindowHeight()};
        SDL_RenderFillRect(GetRenderer(), &s);
        SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
    }
    m_TextRenderer->RenderText("Time: " + std::to_string((int)m_Time.m_TimeOfDay) + ":00", GetWindowWidth()-100, 10, {255,255,255,255});
}

void PixelsGateGame::StartDiceRoll(int modifier, int dc, const std::string &skill, PixelsEngine::Entity target, PixelsEngine::ContextActionType type) {
    m_DiceRoll.active = true;
    m_DiceRoll.timer = 0.0f;
    m_DiceRoll.duration = 1.0f; // Ensure it finishes
    m_DiceRoll.resultShown = false;
    m_DiceRoll.modifier = modifier;
    m_DiceRoll.dc = dc;
    m_DiceRoll.skillName = skill;
    m_DiceRoll.target = target;
    m_DiceRoll.actionType = type;
    int roll = PixelsEngine::Dice::Roll(20);
    m_DiceRoll.finalValue = roll;
    m_DiceRoll.success = (roll + modifier >= dc) || (roll == 20);
}

void PixelsGateGame::ResolveDiceRoll() {
    if (m_DiceRoll.actionType == PixelsEngine::ContextActionType::Lockpick) {
        if (m_DiceRoll.success) {
            auto *lock = GetRegistry().GetComponent<PixelsEngine::LockComponent>(m_DiceRoll.target);
            if (lock) lock->isLocked = false;
            SpawnFloatingText(0, 0, "Unlocked!", {0, 255, 0, 255});
        } else {
            // Consume Thieves' Tools on failure
            auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
            if (inv) {
                for (auto it = inv->items.begin(); it != inv->items.end(); ++it) {
                    if (it->name == "Thieves' Tools") {
                        it->quantity--;
                        if (it->quantity <= 0) inv->items.erase(it);
                        SpawnFloatingText(0, 0, "Thieves' Tools broke!", {255, 0, 0, 255});
                        break;
                    }
                }
            }
        }
    }
}

void PixelsGateGame::SpawnFloatingText(float x, float y, const std::string &text, SDL_Color color) {
    m_FloatingText.m_Texts.push_back({x, y, text, 1.5f, color});
}

void PixelsGateGame::SpawnLootBag(float x, float y, const std::vector<PixelsEngine::Item> &items) {
    auto bag = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(bag, PixelsEngine::TransformComponent{x, y});
    
    auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/gold_orb.png"); // Reuse sprite
    GetRegistry().AddComponent(bag, PixelsEngine::SpriteComponent{tex, {0, 0, 32, 32}, 16, 16});
    GetRegistry().AddComponent(bag, PixelsEngine::InteractionComponent{"Loot", "loot_bag", false, 0.0f});
    
    PixelsEngine::LootComponent loot;
    loot.drops = items;
    GetRegistry().AddComponent(bag, loot);
}

void PixelsGateGame::PickupItem(PixelsEngine::Entity entity) {
    auto *interact = GetRegistry().GetComponent<PixelsEngine::InteractionComponent>(entity);
    auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if (!interact || !inv) return;

    // Special case for Gold Orb
    if (interact->uniqueId == "item_gold_orb") {
        inv->AddItem("Gold Orb", 1, PixelsEngine::ItemType::Misc, 0, "assets/gold_orb.png", 500);
        m_WorldFlags["Quest_FetchOrb_Found"] = true; // Found the item flag
        SpawnFloatingText(0, 0, "Picked up Gold Orb", {255, 215, 0, 255});
        GetRegistry().DestroyEntity(entity);
    } 
    else if (interact->uniqueId == "item_key" || interact->uniqueId == "obj_chest_key") {
        inv->AddItem("Chest Key", 1, PixelsEngine::ItemType::Misc, 0, "assets/key.png", 0);
        GetRegistry().DestroyEntity(entity);
    }
    else if (interact->uniqueId == "item_tools") {
        inv->AddItem("Thieves' Tools", 1, PixelsEngine::ItemType::Tool, 0, "assets/thieves_tools.png", 25);
        GetRegistry().DestroyEntity(entity);
    }
}

void PixelsGateGame::UseItem(const std::string &itemName) {
    auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if (!inv) return;
    
    for (auto it = inv->items.begin(); it != inv->items.end(); ++it) {
        if (it->name == itemName) {
            auto *s = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
            auto *t = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
            bool used = false;
            
            if (itemName == "Potion") {
                if (s) {
                    int heal = PixelsEngine::Dice::Roll(4) + PixelsEngine::Dice::Roll(4) + 2;
                    s->currentHealth = std::min(s->maxHealth, s->currentHealth + heal);
                    if (t) SpawnFloatingText(t->x, t->y, "+" + std::to_string(heal) + " HP", {0,255,0,255});
                    used = true;
                }
            } else if (itemName == "Bread") {
                if (s) {
                    s->currentHealth = std::min(s->maxHealth, s->currentHealth + 5);
                    if (t) SpawnFloatingText(t->x, t->y, "+5 HP", {0,255,0,255});
                    used = true;
                }
            } else if (it->type == PixelsEngine::ItemType::Scroll) {
                 if (t) SpawnFloatingText(t->x, t->y, "Scroll Used!", {200,100,255,255});
                 used = true;
            }
            
            if (used) {
                it->quantity--;
                if (it->quantity <= 0) inv->items.erase(it);
                
                // Combat cost
                if (m_State == GameState::Combat) m_Combat.m_BonusActionsLeft--;
            }
            break;
        }
    }
}

std::vector<std::string> PixelsGateGame::GetHotbarItems() {
    std::vector<std::string> res;
    auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if(inv) {
        for(auto &i : inv->items) {
            if(i.type == PixelsEngine::ItemType::Consumable || i.type == PixelsEngine::ItemType::Tool || i.type == PixelsEngine::ItemType::Scroll) {
                res.push_back(i.name);
            }
            if(res.size()>=6) break;
        }
    }
    while(res.size()<6) res.push_back("");
    return res;
}