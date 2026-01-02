#include "PixelsGateGame.h"
#include "../engine/Dice.h"
#include "../engine/Pathfinding.h"
#include "../engine/Tiles.h"
// --- Added Missing Headers ---
#include "../engine/Input.h"
#include "../engine/AnimationSystem.h"
// -----------------------------
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void PixelsGateGame::PerformAttack(PixelsEngine::Entity forcedTarget) {
    auto *playerStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    auto *playerTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    if (!playerStats || !playerTrans) return;

    if (m_State == GameState::Combat) {
        if (!m_Combat.m_TurnOrder[m_Combat.m_CurrentTurnIndex].isPlayer) {
            SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "Not Your Turn!", {200, 0, 0, 255});
            return;
        }
        if (m_Combat.m_ActionsLeft <= 0) {
            SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "No Actions Left!", {200, 0, 0, 255});
            return;
        }
    }

    PixelsEngine::Entity target = forcedTarget;
    if (target == PixelsEngine::INVALID_ENTITY && m_ContextMenu.isOpen && m_ContextMenu.targetEntity != PixelsEngine::INVALID_ENTITY) {
        target = m_ContextMenu.targetEntity;
    }

    if (target == PixelsEngine::INVALID_ENTITY) {
        float minDistance = 3.0f;
        auto &view = GetRegistry().View<PixelsEngine::StatsComponent>();
        for (auto &[entity, stats] : view) {
            if (entity == m_Player || stats.isDead) continue;
            auto *targetTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
            if (targetTrans) {
                float dist = std::sqrt(std::pow(playerTrans->x - targetTrans->x, 2) + std::pow(playerTrans->y - targetTrans->y, 2));
                if (dist < minDistance) {
                    minDistance = dist;
                    target = entity;
                }
            }
        }
    }

    if (target != PixelsEngine::INVALID_ENTITY) {
        auto *targetStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(target);
        if (!targetStats || targetStats->isDead) {
            SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "Invalid Target", {200, 200, 200, 255});
            return;
        }

        if (m_State == GameState::Playing || m_State == GameState::Camp) {
            auto *ai = GetRegistry().GetComponent<PixelsEngine::AIComponent>(target);
            StartCombat(ai ? target : PixelsEngine::INVALID_ENTITY);
        }

        auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
        if (inv) {
            PixelsEngine::Item &activeWeapon = (m_SelectedWeaponSlot == 0) ? inv->equippedMelee : inv->equippedRanged;
            float maxRange = (m_SelectedWeaponSlot == 0) ? 2.0f : 10.0f; // Default fists range
            if (m_SelectedWeaponSlot == 0 && !activeWeapon.IsEmpty()) maxRange = 3.0f;

            auto *targetTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(target);
            float dist = targetTrans ? std::sqrt(std::pow(playerTrans->x - targetTrans->x, 2) + std::pow(playerTrans->y - targetTrans->y, 2)) : 100.0f;

            if (dist > maxRange + 0.5f) { // Small buffer for grid conversion
                SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "Too Far!", {200, 200, 200, 255});
                return;
            }

            int targetAC = 10 + targetStats->GetModifier(targetStats->dexterity);
            int roll = PixelsEngine::Dice::Roll(20);
            if (playerStats->hasAdvantage) {
                int secondRoll = PixelsEngine::Dice::Roll(20);
                roll = std::max(roll, secondRoll);
                playerStats->hasAdvantage = false;
                playerStats->isStealthed = false;
                SpawnFloatingText(playerTrans->x, playerTrans->y, "Advantage!", {0, 255, 0, 255});
            }

            int attackBonus = (m_SelectedWeaponSlot == 0) ? playerStats->GetModifier(playerStats->strength) : playerStats->GetModifier(playerStats->dexterity);
            attackBonus += activeWeapon.statBonus;

            if (roll == 20 || (roll != 1 && roll + attackBonus >= targetAC)) {
                int dmg = playerStats->damage + activeWeapon.statBonus;
                bool isCrit = (roll == 20);
                if (isCrit) dmg *= 2;
                
                targetStats->currentHealth -= dmg;
                
                if (targetTrans) {
                    SpawnFloatingText(targetTrans->x, targetTrans->y, std::to_string(dmg) + (isCrit ? "!" : ""), isCrit ? SDL_Color{255,0,0,255} : SDL_Color{255,255,255,255});
                    SpawnFloatingText(playerTrans->x, playerTrans->y, "Hit: " + std::to_string(roll) + "+" + std::to_string(attackBonus) + " vs " + std::to_string(targetAC), {0, 255, 0, 255});
                }

                if (targetStats->currentHealth <= 0) {
                    targetStats->currentHealth = 0;
                    targetStats->isDead = true;
                    
                    // Loot bag spawning
                    auto *tInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(target);
                    auto *loot = GetRegistry().GetComponent<PixelsEngine::LootComponent>(target);
                    if (!loot) loot = &GetRegistry().AddComponent(target, PixelsEngine::LootComponent{});
                    if (tInv) {
                        for (auto &item : tInv->items) loot->drops.push_back(item);
                        tInv->items.clear();
                    }
                    auto *ai = GetRegistry().GetComponent<PixelsEngine::AIComponent>(target);
                    if (ai) ai->isAggressive = false;
                }

                // Witness logic (Crime)
                auto *targetTag = GetRegistry().GetComponent<PixelsEngine::TagComponent>(target);
                auto *targetAI = GetRegistry().GetComponent<PixelsEngine::AIComponent>(target);
                bool isCrime = (targetTag && (targetTag->tag == PixelsEngine::EntityTag::NPC || targetTag->tag == PixelsEngine::EntityTag::Trader));
                if (targetAI && targetAI->isAggressive) isCrime = false;

                if (isCrime) {
                    auto &aiView = GetRegistry().View<PixelsEngine::AIComponent>();
                    for (auto &[witness, wai] : aiView) {
                        if (witness == m_Player || witness == target) continue;
                        auto *wTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(witness);
                        if (!wTrans) continue;
                        float d = std::sqrt(std::pow(playerTrans->x - wTrans->x, 2) + std::pow(playerTrans->y - wTrans->y, 2));
                        if (d <= wai.sightRange) {
                            wai.isAggressive = true;
                            wai.hostileTimer = 30.0f;
                            SpawnFloatingText(wTrans->x, wTrans->y, "Halt criminal!", {255, 0, 0, 255});
                            if (m_State == GameState::Combat && !IsInTurnOrder(witness)) {
                                auto *wStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(witness);
                                m_Combat.m_TurnOrder.push_back({witness, PixelsEngine::Dice::Roll(20) + (wStats ? wStats->GetModifier(wStats->dexterity) : 0), false});
                            }
                        }
                    }
                }
            } else {
                if (targetTrans) {
                    SpawnFloatingText(targetTrans->x, targetTrans->y, "Miss", {200, 200, 200, 255});
                    SpawnFloatingText(playerTrans->x, playerTrans->y, "Roll: " + std::to_string(roll) + "+" + std::to_string(attackBonus) + " vs " + std::to_string(targetAC), {200, 200, 200, 255});
                }
            }

            if (m_State == GameState::Combat) m_Combat.m_ActionsLeft--;

            if (m_State == GameState::Combat) {
                bool enemiesAlive = false;
                for (const auto &turn : m_Combat.m_TurnOrder) {
                    if (!turn.isPlayer) {
                        auto *s = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(turn.entity);
                        if (s && !s->isDead) enemiesAlive = true;
                    }
                }
                if (!enemiesAlive) EndCombat();
            }
        }
    } else {
        SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "No Target", {200, 200, 200, 255});
    }
}

void PixelsGateGame::StartCombat(PixelsEngine::Entity enemy) {
    if (m_State == GameState::Combat) {
        if (!IsInTurnOrder(enemy)) {
            auto *eStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(enemy);
            if (eStats && !eStats->isDead) {
                int eInit = PixelsEngine::Dice::Roll(20) + eStats->GetModifier(eStats->dexterity);
                m_Combat.m_TurnOrder.push_back({enemy, eInit, false});
                SpawnFloatingText(0, 0, "Reinforcements!", {255, 100, 0, 255});
            }
        }
        return;
    }
    m_Combat.m_TurnOrder.clear();
    bool anyEnemy = false;
    auto *pStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    m_Combat.m_TurnOrder.push_back({m_Player, PixelsEngine::Dice::Roll(20) + (pStats ? pStats->GetModifier(pStats->dexterity) : 0), true});
    SpawnFloatingText(20, 20, "Initiative!", {0, 255, 255, 255});

    if (GetRegistry().Valid(enemy)) {
        auto *eStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(enemy);
        if (eStats && !eStats->isDead) {
            m_Combat.m_TurnOrder.push_back({enemy, PixelsEngine::Dice::Roll(20) + eStats->GetModifier(eStats->dexterity), false});
            anyEnemy = true;
        }
    }

    auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    auto &view = GetRegistry().View<PixelsEngine::AIComponent>();
    for (auto &[ent, ai] : view) {
        if (ent == enemy) continue;
        auto *t = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(ent);
        if (t && pTrans) {
            float dist = std::sqrt(std::pow(t->x - pTrans->x, 2) + std::pow(t->y - pTrans->y, 2));
            if (dist < 15.0f && ai.isAggressive) {
                auto *eStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(ent);
                if (eStats && !eStats->isDead) {
                    m_Combat.m_TurnOrder.push_back({ent, PixelsEngine::Dice::Roll(20) + eStats->GetModifier(eStats->dexterity), false});
                    anyEnemy = true;
                }
            }
        }
    }

    if (!anyEnemy) { m_Combat.m_TurnOrder.clear(); return; }

    std::sort(m_Combat.m_TurnOrder.begin(), m_Combat.m_TurnOrder.end(), [](const CombatManager::CombatTurn &a, const CombatManager::CombatTurn &b) { return a.initiative > b.initiative; });
    m_State = GameState::Combat;
    m_Combat.m_CurrentTurnIndex = -1;
    NextTurn();
}

void PixelsGateGame::EndCombat() {
    m_State = GameState::Playing;
    m_Combat.m_TurnOrder.clear();
    SpawnFloatingText(0, 0, "Combat Ended", {0, 255, 0, 255});
}

void PixelsGateGame::NextTurn() {
    bool enemiesAlive = false;
    std::vector<int> toRemove;
    for (int i = 0; i < m_Combat.m_TurnOrder.size(); ++i) {
        if (!m_Combat.m_TurnOrder[i].isPlayer) {
            auto *stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Combat.m_TurnOrder[i].entity);
            if (stats && !stats->isDead) enemiesAlive = true;
            else if (!stats || stats->isDead) toRemove.push_back(i);
        }
    }

    // Remove dead from turn order (reverse to keep indices valid)
    for(int i = (int)toRemove.size()-1; i>=0; --i) {
        if (toRemove[i] < m_Combat.m_CurrentTurnIndex) m_Combat.m_CurrentTurnIndex--;
        else if (toRemove[i] == m_Combat.m_CurrentTurnIndex) m_Combat.m_CurrentTurnIndex--; // Let it increment back to 0 or next
        m_Combat.m_TurnOrder.erase(m_Combat.m_TurnOrder.begin() + toRemove[i]);
    }

    if (!enemiesAlive) { EndCombat(); return; }

    m_Combat.m_CurrentTurnIndex++;
    if (m_Combat.m_CurrentTurnIndex >= m_Combat.m_TurnOrder.size() || m_Combat.m_CurrentTurnIndex < 0) m_Combat.m_CurrentTurnIndex = 0;

    auto &current = m_Combat.m_TurnOrder[m_Combat.m_CurrentTurnIndex];
    if (current.isSurprised) {
        current.isSurprised = false;
        SpawnFloatingText(0, 0, "Surprised! Skipping.", {255, 100, 0, 255});
        NextTurn();
        return;
    }
    auto *cStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(current.entity);
    if (cStats && cStats->isDead) { NextTurn(); return; }

    m_Combat.m_ActionsLeft = 1; m_Combat.m_BonusActionsLeft = 1; m_Combat.m_MovementLeft = 5.0f;
    if (current.isPlayer) {
        auto *pc = GetRegistry().GetComponent<PixelsEngine::PlayerComponent>(m_Player);
        if (pc) m_Combat.m_MovementLeft = pc->speed;
        SpawnFloatingText(0, 0, "YOUR TURN", {0, 255, 0, 255});
    } else {
        m_Combat.m_CombatTurnTimer = 1.0f;
        auto *ai = GetRegistry().GetComponent<PixelsEngine::AIComponent>(current.entity);
        if (ai) m_Combat.m_MovementLeft = ai->sightRange;
    }
}

void PixelsGateGame::UpdateCombat(float deltaTime) {
    if (m_Combat.m_CurrentTurnIndex < 0 || m_Combat.m_CurrentTurnIndex >= m_Combat.m_TurnOrder.size()) return;
    auto &turn = m_Combat.m_TurnOrder[m_Combat.m_CurrentTurnIndex];

    if (turn.isPlayer) { HandleCombatInput(); }
    else {
        auto *aiTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(turn.entity);
        auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
        auto *aiComp = GetRegistry().GetComponent<PixelsEngine::AIComponent>(turn.entity);
        auto *aiStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(turn.entity);
        auto *anim = GetRegistry().GetComponent<PixelsEngine::AnimationComponent>(turn.entity);

        if (!aiTrans || !pTrans || !aiStats || aiStats->isDead) { NextTurn(); return; }

        if (m_Combat.m_CombatTurnTimer > 0.0f) {
            m_Combat.m_CombatTurnTimer -= deltaTime;
            if (m_Combat.m_CombatTurnTimer <= 0.0f) {
                auto *currentMap = GetCurrentMap();
                m_CurrentAIPath = PixelsEngine::Pathfinding::FindPath(*currentMap, (int)aiTrans->x, (int)aiTrans->y, (int)pTrans->x, (int)pTrans->y);
                m_CurrentAIPathIndex = 1;
            }
            return;
        }

        bool moving = false;
        if (m_CurrentAIPathIndex != -1 && m_CurrentAIPathIndex < m_CurrentAIPath.size() && m_Combat.m_MovementLeft > 0.1f) {
            float distToPlayer = std::sqrt(std::pow(aiTrans->x - pTrans->x, 2) + std::pow(aiTrans->y - pTrans->y, 2));
            if (distToPlayer > aiComp->attackRange) {
                float targetX = (float)m_CurrentAIPath[m_CurrentAIPathIndex].first;
                float targetY = (float)m_CurrentAIPath[m_CurrentAIPathIndex].second;
                float dx = targetX - aiTrans->x; float dy = targetY - aiTrans->y;
                float distStep = std::sqrt(dx*dx + dy*dy);
                if (distStep < 0.05f) m_CurrentAIPathIndex++;
                else {
                    float speed = 4.0f; float moveThisFrame = speed * deltaTime;
                    if (moveThisFrame > distStep) moveThisFrame = distStep;
                    if (moveThisFrame > m_Combat.m_MovementLeft) moveThisFrame = m_Combat.m_MovementLeft;
                    aiTrans->x += (dx/distStep) * moveThisFrame; aiTrans->y += (dy/distStep) * moveThisFrame;
                    m_Combat.m_MovementLeft -= moveThisFrame; moving = true;
                    if (anim) anim->Play("Run");
                }
            }
        }
        if (moving) return;

        if (anim) anim->Play("Idle");
        float finalDist = std::sqrt(std::pow(aiTrans->x - pTrans->x, 2) + std::pow(aiTrans->y - pTrans->y, 2));
        if (finalDist <= aiComp->attackRange && m_Combat.m_ActionsLeft > 0) {
            auto *pTargetStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
            if (pTargetStats) {
                int roll = PixelsEngine::Dice::Roll(20);
                // Simplified AC check
                if (roll + aiStats->GetModifier(aiStats->strength) >= 10 + pTargetStats->GetModifier(pTargetStats->dexterity)) {
                    int dmg = aiStats->damage;
                    pTargetStats->currentHealth -= dmg;
                    SpawnFloatingText(pTrans->x, pTrans->y, std::to_string(dmg), {255, 0, 0, 255});
                } else {
                    SpawnFloatingText(pTrans->x, pTrans->y, "Miss", {200, 200, 200, 255});
                }
                m_Combat.m_ActionsLeft--;
            }
        }
        m_CurrentAIPathIndex = -1;
        NextTurn();
    }
}

void PixelsGateGame::CastSpell(const std::string &spellName, PixelsEngine::Entity target) {
    auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    if (m_State == GameState::Combat && m_Combat.m_ActionsLeft <= 0) {
        SpawnFloatingText(pTrans->x, pTrans->y, "No Actions!", {255, 0, 0, 255});
        m_State = m_ReturnState; return;
    }

    bool success = false;
    if (spellName == "Heal" || spellName == "Hel") {
        PixelsEngine::Entity t = (target == PixelsEngine::INVALID_ENTITY) ? m_Player : target;
        auto *s = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(t);
        if (s) { 
            int healing = PixelsEngine::Dice::Roll(8) + 4;
            s->currentHealth = std::min(s->maxHealth, s->currentHealth + healing); 
            auto *tTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(t);
            if(tTrans) SpawnFloatingText(tTrans->x, tTrans->y, "+" + std::to_string(healing), {0, 255, 0, 255});
            success = true; 
        }
    } else if ((spellName == "Fireball" || spellName == "Fir")) {
        // Area effect or direct target
        PixelsEngine::Entity t = target;
        float tx, ty;
        if (t != PixelsEngine::INVALID_ENTITY) {
            auto *tt = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(t);
            tx = tt->x; ty = tt->y;
        } else {
            int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
            auto &camera = GetCamera();
            auto *currentMap = (m_State == GameState::Camp) ? m_CampLevel.get() : m_Level.get();
            int gx, gy; currentMap->ScreenToGrid(mx + camera.x, my + camera.y, gx, gy);
            tx = (float)gx; ty = (float)gy;
        }

        auto hazard = GetRegistry().CreateEntity();
        GetRegistry().AddComponent(hazard, PixelsEngine::TransformComponent{tx, ty});
        GetRegistry().AddComponent(hazard, PixelsEngine::HazardComponent{PixelsEngine::HazardComponent::Type::Fire, 8, 5.0f});
        SpawnFloatingText(tx, ty, "BOOM!", {255, 100, 0, 255});
        success = true;
    } else if ((spellName == "Magic Missile" || spellName == "Mis") && target != PixelsEngine::INVALID_ENTITY) {
        auto *s = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(target);
        if (s) { 
            int dmg = PixelsEngine::Dice::Roll(4) + 1;
            s->currentHealth -= dmg; 
            auto *tTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(target);
            if(tTrans) SpawnFloatingText(tTrans->x, tTrans->y, std::to_string(dmg), {200, 100, 255, 255});
            success = true; 
        }
    } else if (spellName == "Shield" || spellName == "Shd") {
        SpawnFloatingText(pTrans->x, pTrans->y, "Shielded!", {100, 200, 255, 255});
        success = true;
    }

    if (success && m_State == GameState::Combat) m_Combat.m_ActionsLeft--;
    m_State = m_ReturnState;
}

void PixelsGateGame::PerformJump(int targetX, int targetY) {
    auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    if (!pTrans) return;

    auto *currentMap = GetCurrentMap();
    if (!currentMap->IsWalkable(targetX, targetY)) {
        // Special case: Allow jumping onto interactive objects even if tile is blocked by them
        // Actually, our IsWalkable checks for TILE types, not entities.
        // If it still says invalid, maybe the tile type at (5,5) is ROCK? 
        // Let's allow jumping if it's within range regardless of walkability for testing, 
        // OR better: check if it's a solid tile type.
        if (currentMap->GetTile(targetX, targetY) == PixelsEngine::Tiles::ROCK) {
             SpawnFloatingText(pTrans->x, pTrans->y, "Invalid Location", {200, 200, 200, 255});
             m_State = m_ReturnState;
             return;
        }
    }

    if (m_State == GameState::Combat) {
        if (m_Combat.m_BonusActionsLeft <= 0 || m_Combat.m_MovementLeft < 3.0f) {
            SpawnFloatingText(pTrans->x, pTrans->y, "No Bonus Action/Movement!", {255, 0, 0, 255});
            m_State = m_ReturnState;
            return;
        }
        m_Combat.m_BonusActionsLeft--;
        m_Combat.m_MovementLeft -= 3.0f;
    }

    pTrans->x = (float)targetX;
    pTrans->y = (float)targetY;
    SpawnFloatingText(pTrans->x, pTrans->y, "Jump!", {200, 255, 200, 255});
    m_State = m_ReturnState;
}

void PixelsGateGame::PerformDash(int targetX, int targetY) {
    auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    if (!pTrans) return;

    float dist = std::sqrt(std::pow(pTrans->x - targetX, 2) + std::pow(pTrans->y - targetY, 2));
    float maxDist = 5.0f; // Dash range

    if (dist > maxDist) {
        SpawnFloatingText(pTrans->x, pTrans->y, "Too Far!", {200, 200, 200, 255});
        m_State = m_ReturnState;
        return;
    }

    auto *currentMap = (m_State == GameState::Camp) ? m_CampLevel.get() : m_Level.get();
    if (!currentMap->IsWalkable(targetX, targetY)) {
        SpawnFloatingText(pTrans->x, pTrans->y, "Invalid Location", {200, 200, 200, 255});
        m_State = m_ReturnState;
        return;
    }

    if (m_State == GameState::Combat) {
        if (m_Combat.m_ActionsLeft <= 0) {
            SpawnFloatingText(pTrans->x, pTrans->y, "No Actions!", {255, 0, 0, 255});
            m_State = m_ReturnState;
            return;
        }
        m_Combat.m_ActionsLeft--;
    }

    pTrans->x = (float)targetX;
    pTrans->y = (float)targetY;
    SpawnFloatingText(pTrans->x, pTrans->y, "Dash!", {255, 255, 255, 255});
    m_State = m_ReturnState;
}

void PixelsGateGame::PerformShove(PixelsEngine::Entity target) {
    auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    if (!pTrans || target == PixelsEngine::INVALID_ENTITY || target == m_Player) {
        m_State = m_ReturnState;
        return;
    }

    auto *tTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(target);
    auto *tStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(target);
    if (!tTrans || !tStats || tStats->isDead) {
        m_State = m_ReturnState;
        return;
    }

    float dist = std::sqrt(std::pow(pTrans->x - tTrans->x, 2) + std::pow(pTrans->y - tTrans->y, 2));
    if (dist > 2.0f) {
        SpawnFloatingText(pTrans->x, pTrans->y, "Too Far!", {200, 200, 200, 255});
        m_State = m_ReturnState;
        return;
    }

    if (m_State == GameState::Combat) {
        if (m_Combat.m_BonusActionsLeft <= 0) {
            SpawnFloatingText(pTrans->x, pTrans->y, "No Bonus Action!", {255, 0, 0, 255});
            m_State = m_ReturnState;
            return;
        }
        m_Combat.m_BonusActionsLeft--;
    }

    // Simple shove: move target 1 tile away
    float dx = tTrans->x - pTrans->x;
    float dy = tTrans->y - pTrans->y;
    float mag = std::sqrt(dx*dx + dy*dy);
    if (mag > 0) {
        int nx = (int)(tTrans->x + dx/mag);
        int ny = (int)(tTrans->y + dy/mag);
        auto *currentMap = (m_State == GameState::Camp) ? m_CampLevel.get() : m_Level.get();
        if (currentMap->IsWalkable(nx, ny)) {
            tTrans->x = (float)nx;
            tTrans->y = (float)ny;
            SpawnFloatingText(tTrans->x, tTrans->y, "Shoved!", {255, 150, 0, 255});
        }
    }

    m_State = m_ReturnState;
}

bool PixelsGateGame::IsInTurnOrder(PixelsEngine::Entity entity) {
    for (const auto &turn : m_Combat.m_TurnOrder) if (turn.entity == entity) return true;
    return false;
}

void PixelsGateGame::RenderCombatUI() {
    SDL_Renderer *renderer = GetRenderer();
    int w = GetWindowWidth(); int h = GetWindowHeight();
    
    // Turn Order Bar
    int slotW = 60; 
    int totalBarW = (int)m_Combat.m_TurnOrder.size() * slotW;
    int startX = (w - totalBarW) / 2;
    
    for (int i = 0; i < m_Combat.m_TurnOrder.size(); ++i) {
        auto &turn = m_Combat.m_TurnOrder[i];
        auto *s = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(turn.entity);
        if (!s) continue;

        SDL_Rect slot = {startX + i * slotW, 10, slotW - 5, 40};
        bool isCurrent = (i == m_Combat.m_CurrentTurnIndex);
        
        // Background
        SDL_SetRenderDrawColor(renderer, isCurrent ? 50 : 30, isCurrent ? 150 : 30, isCurrent ? 50 : 30, 255);
        SDL_RenderFillRect(renderer, &slot);
        
        // HP Percentage
        int hpPct = (int)((float)s->currentHealth / s->maxHealth * 100);
        SDL_Color hpColor = (hpPct > 50) ? SDL_Color{100, 255, 100, 255} : (hpPct > 20 ? SDL_Color{255, 255, 100, 255} : SDL_Color{255, 50, 50, 255});
        m_TextRenderer->RenderTextSmall(std::to_string(hpPct) + "%", slot.x + 5, slot.y + 5, hpColor);
        
        // Label (P or E)
        m_TextRenderer->RenderTextSmall(turn.isPlayer ? "YOU" : "ENMY", slot.x + 5, slot.y + 20, {255, 255, 255, 255});

        // Border
        SDL_SetRenderDrawColor(renderer, isCurrent ? 255 : 150, isCurrent ? 255 : 150, 255, 255);
        SDL_RenderDrawRect(renderer, &slot);
    }

    // Current Turn Info
    if (m_Combat.m_CurrentTurnIndex >= 0 && m_Combat.m_CurrentTurnIndex < m_Combat.m_TurnOrder.size()) {
        auto &current = m_Combat.m_TurnOrder[m_Combat.m_CurrentTurnIndex];
        if (current.isPlayer) {
            std::string resStr = "Action: " + std::to_string(m_Combat.m_ActionsLeft) + 
                                 " | Bonus: " + std::to_string(m_Combat.m_BonusActionsLeft) + 
                                 " | Move: " + std::to_string((int)m_Combat.m_MovementLeft) + "m";
            m_TextRenderer->RenderTextCentered(resStr, w/2, h - 140, {255, 255, 0, 255});
            m_TextRenderer->RenderTextCentered("Press SPACE to End Turn", w/2, h - 120, {200, 200, 200, 255});
        } else {
            m_TextRenderer->RenderTextCentered("ENEMY TURN...", w/2, h - 140, {255, 100, 100, 255});
        }
    }
}

void PixelsGateGame::RenderEnemyCones(const PixelsEngine::Camera &camera) {
    if (!PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_LSHIFT)) return;
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
    auto &view = GetRegistry().View<PixelsEngine::AIComponent>();
    for (auto &[entity, ai] : view) {
        auto *transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
        if (!transform) continue;
        float screenX = (transform->x - camera.x) * 32.0f + GetWindowWidth() / 2.0f;
        float screenY = (transform->y - camera.y) * 32.0f + GetWindowHeight() / 2.0f;
        
        float radDir = ai.facingDir * (M_PI / 180.0f);
        float radHalf = (ai.coneAngle / 2.0f) * (M_PI / 180.0f);
        float range = ai.sightRange * 32.0f;
        
        std::vector<SDL_Vertex> verts;
        verts.push_back({{screenX, screenY}, {255, 0, 0, 100}, {0,0}});
        for (int i = 0; i <= 5; ++i) {
            float angle = radDir - radHalf + (i * (2 * radHalf) / 5.0f);
            verts.push_back({{screenX + cos(angle)*range, screenY + sin(angle)*range}, {255,0,0,100}, {0,0}});
        }
        for (size_t i = 0; i < verts.size() - 1; ++i) {
            SDL_Vertex tri[3] = {verts[0], verts[i], verts[i+1]};
            SDL_RenderGeometry(GetRenderer(), NULL, tri, 3, NULL, 0);
        }
    }
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
}