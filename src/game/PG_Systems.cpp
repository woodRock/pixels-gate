#include "PixelsGateGame.h"
#include "../engine/Dice.h"
#include "../engine/Input.h"
#include "../engine/TextureManager.h"
#include "../engine/AnimationSystem.h"
#include "../engine/AudioManager.h"
#include <cmath>
#include <algorithm> 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void PixelsGateGame::UpdateAnimations(float deltaTime) {
    auto &view = GetRegistry().View<PixelsEngine::AnimationComponent>();
    for (auto &[entity, anim] : view) {
        if (!anim.isPlaying || anim.animations.empty()) continue;

        anim.timer += deltaTime;
        if (anim.timer >= anim.animations[anim.currentAnimationIndex].frameDuration) {
            anim.timer = 0.0f;
            anim.currentFrameIndex++;
            if (anim.currentFrameIndex >= anim.animations[anim.currentAnimationIndex].frames.size()) {
                if (anim.animations[anim.currentAnimationIndex].loop) {
                    anim.currentFrameIndex = 0;
                } else {
                    anim.currentFrameIndex--;
                    anim.isPlaying = false;
                }
            }
        }

        auto *sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
        if (sprite) {
            sprite->srcRect = anim.animations[anim.currentAnimationIndex].frames[anim.currentFrameIndex];
        }
    }
}

void PixelsGateGame::UpdateAI(float deltaTime) {
    auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    auto *pStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    if (!pTrans || !pStats) return;

    auto &view = GetRegistry().View<PixelsEngine::AIComponent>();
    for (auto &[entity, ai] : view) {
        if (m_State == GameState::Combat && IsInTurnOrder(entity)) continue;
        auto *transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
        if (!transform) continue;

        auto *stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(entity);
        if (stats && stats->isDead) continue; // Safety check

        auto *tag = GetRegistry().GetComponent<PixelsEngine::TagComponent>(entity);
        if (tag && tag->tag == PixelsEngine::EntityTag::Companion) {
            float dist = std::sqrt(std::pow(pTrans->x - transform->x, 2) + std::pow(pTrans->y - transform->y, 2));
            if (dist > 3.0f) {
                float dx = pTrans->x - transform->x;
                float dy = pTrans->y - transform->y;
                float len = std::sqrt(dx*dx + dy*dy);
                if (len > 0) {
                    float speed = 3.8f;
                    float moveX = (dx / len) * speed * deltaTime;
                    float moveY = (dy / len) * speed * deltaTime;
                    auto *map = GetCurrentMap();
                    if (map) {
                        if (map->IsWalkable(transform->x + moveX, transform->y + moveY)) {
                            transform->x += moveX; transform->y += moveY;
                        } else if (map->IsWalkable(transform->x + moveX, transform->y)) {
                            transform->x += moveX;
                        } else if (map->IsWalkable(transform->x, transform->y + moveY)) {
                            transform->y += moveY;
                        }
                    } else {
                        transform->x += moveX; transform->y += moveY;
                    }
                    ai.facingDir = std::atan2(dy, dx) * (180.0f / M_PI);
                }
            }
            continue;
        }

        if (ai.hostileTimer > 0.0f) {
            ai.hostileTimer -= deltaTime;
            if (ai.hostileTimer <= 0.0f) ai.isAggressive = false;
        }

        float dist = std::sqrt(std::pow(pTrans->x - transform->x, 2) + std::pow(pTrans->y - transform->y, 2));
        
        bool detected = false;
        if (dist <= ai.sightRange) {
            float dx = pTrans->x - transform->x;
            float dy = pTrans->y - transform->y;
            float angleToPlayer = std::atan2(dy, dx) * (180.0f / M_PI);
            
            float diff = std::abs(ai.facingDir - angleToPlayer);
            if (diff > 180.0f) diff = 360.0f - diff;

            if (pStats->isStealthed) {
                // Sneaking: only detected if in cone
                if (diff <= ai.coneAngle / 2.0f) detected = true;
            } else {
                // Not sneaking: detected if in cone OR very close
                if (diff <= ai.coneAngle / 2.0f || dist < 2.0f) detected = true;
            }
        }

        if (ai.isAggressive && detected) {
            if (dist > ai.attackRange) {
                float dx = pTrans->x - transform->x; float dy = pTrans->y - transform->y;
                float len = std::sqrt(dx*dx + dy*dy);
                if (len > 0) {
                    transform->x += (dx/len) * 2.0f * deltaTime;
                    transform->y += (dy/len) * 2.0f * deltaTime;
                    ai.facingDir = std::atan2(dy, dx) * (180.0f / M_PI);
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
    // No automatic time progression
    if (m_State == GameState::Camp) m_Time.m_TimeOfDay = 24.0f; // Night
    else m_Time.m_TimeOfDay = 12.0f; // Day
}

void PixelsGateGame::RenderDayNightCycle() {
    SDL_Color tint = {0,0,0,0};
    
    if (m_State == GameState::Camp) {
        tint = {10, 10, 50, 180}; // Brighter Night for Camp
    }
    
    if (tint.a > 0) {
        SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(GetRenderer(), tint.r, tint.g, tint.b, tint.a);
        SDL_Rect s = {0,0,GetWindowWidth(),GetWindowHeight()};
        SDL_RenderFillRect(GetRenderer(), &s);

        // Draw Lights (Additive) - Only visible when it's dark
        if (tint.a > 100) {
            SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_ADD);
            
            auto &lights = GetRegistry().View<PixelsEngine::LightComponent>();
            auto *currentMap = GetCurrentMap();
            auto &camera = GetCamera();

            for(auto &[entity, light] : lights) {
                auto *trans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
                if(!trans || !currentMap) continue;

                int sx, sy;
                currentMap->GridToScreen(trans->x, trans->y, sx, sy);
                float screenX = (float)(sx - camera.x + 16);
                float screenY = (float)(sy - camera.y + 16);

                float radius = light.radius * 32.0f; 
                if(light.flickers) {
                    radius += (std::rand() % 10 - 5);
                }

                std::vector<SDL_Vertex> verts;
                // Center color (Light color)
                verts.push_back({{screenX, screenY}, {light.color.r, light.color.g, light.color.b, 255}, {0,0}});
                
                int segments = 20;
                for(int i=0; i<=segments; ++i) {
                    float angle = (float)i / segments * 2.0f * M_PI;
                    float px = screenX + std::cos(angle) * radius;
                    float py = screenY + std::sin(angle) * radius * 0.6f; // Isometric squash
                    verts.push_back({{px, py}, {0, 0, 0, 0}, {0,0}});
                }
                
                for(int i=0; i<segments; ++i) {
                    SDL_Vertex tri[3] = {verts[0], verts[i+1], verts[i+2]};
                    SDL_RenderGeometry(GetRenderer(), nullptr, tri, 3, nullptr, 0);
                }
            }
            SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
        }
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
    PixelsEngine::AudioManager::PlaySound("assets/dice.wav");
    if (type == PixelsEngine::ContextActionType::Lockpick) {
        PixelsEngine::AudioManager::PlaySound("assets/lockpicking.wav");
    }
}

void PixelsGateGame::ResolveDiceRoll() {
    if (m_DiceRoll.actionType == PixelsEngine::ContextActionType::Lockpick) {
        if (m_DiceRoll.success) {
            auto *lock = GetRegistry().GetComponent<PixelsEngine::LockComponent>(m_DiceRoll.target);
            if (lock) lock->isLocked = false;
            PixelsEngine::AudioManager::PlaySound("assets/chest_open.wav");
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
    } else if (m_DiceRoll.actionType == PixelsEngine::ContextActionType::Pickpocket) {
        bool seen = false;
        auto &aiView = GetRegistry().View<PixelsEngine::AIComponent>();
        auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
        
        for (auto &[witness, wai] : aiView) {
            auto *wStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(witness);
            if (wStats && wStats->isDead) continue;
            auto *wTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(witness);
            if (!wTrans) continue;

            float dx = pTrans->x - wTrans->x;
            float dy = pTrans->y - wTrans->y;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            if (dist <= wai.sightRange) {
                float angleToPlayer = std::atan2(dy, dx) * (180.0f / M_PI);
                float diff = std::abs(wai.facingDir - angleToPlayer);
                if (diff > 180.0f) diff = 360.0f - diff;
                if (diff <= wai.coneAngle / 2.0f) {
                    seen = true;
                    wai.isAggressive = true;
                    wai.hostileTimer = 30.0f;
                    SpawnFloatingText(wTrans->x, wTrans->y, "Thief!", {255, 0, 0, 255});
                }
            }
        }

        if (seen) {
            SpawnFloatingText(pTrans->x, pTrans->y, "Caught!", {255, 0, 0, 255});
            StartCombat(m_DiceRoll.target);
        } else if (m_DiceRoll.success) {
            auto *tInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_DiceRoll.target);
            auto *pInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
            if (tInv && pInv && !tInv->items.empty()) {
                auto &item = tInv->items[0]; 
                pInv->AddItemObject(item);
                SpawnFloatingText(pTrans->x, pTrans->y, "Stole " + item.name, {0, 255, 0, 255});
                tInv->items.erase(tInv->items.begin());
            } else {
                SpawnFloatingText(pTrans->x, pTrans->y, "Nothing to steal", {200, 200, 200, 255});
            }
        } else {
            SpawnFloatingText(pTrans->x, pTrans->y, "Failed!", {255, 100, 0, 255});
            auto *targetAI = GetRegistry().GetComponent<PixelsEngine::AIComponent>(m_DiceRoll.target);
            if (targetAI) {
                targetAI->isAggressive = true;
                targetAI->hostileTimer = 30.0f;
                SpawnFloatingText(pTrans->x, pTrans->y, "Hey!", {255, 0, 0, 255});
                StartCombat(m_DiceRoll.target);
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
                    PixelsEngine::AudioManager::PlaySound("assets/potion.wav");
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