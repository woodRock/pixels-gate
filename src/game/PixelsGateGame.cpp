#include "PixelsGateGame.h"
#include <iostream>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "../engine/Input.h"
#include "../engine/Dice.h"
#include "../engine/SaveSystem.h"
#include "../engine/TextureManager.h"
#include "../engine/Components.h"
#include "../engine/Pathfinding.h"
#include "../engine/AnimationSystem.h"
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <functional>
#include <filesystem>

PixelsGateGame::PixelsGateGame() 
    : PixelsEngine::Application("Pixels Gate", 800, 600) {
    PixelsEngine::Dice::Init();
}

void PixelsGateGame::OnStart() {
    // Init Font
    m_TextRenderer = std::make_unique<PixelsEngine::TextRenderer>(GetRenderer(), "assets/font.ttf", 16);

    // 1. Setup Level with New Isometric Tileset
    std::string isoTileset = "assets/isometric tileset/spritesheet.png";
    int mapW = 40;
    int mapH = 40;
    m_Level = std::make_unique<PixelsEngine::Tilemap>(GetRenderer(), isoTileset, 32, 32, mapW, mapH);
    m_Level->SetProjection(PixelsEngine::Projection::Isometric);
    
    const int GRASS = 0;  
    const int GRASS_VAR = 1;
    const int DIRT = 2;   
    const int GRASS_Dense = 10; 
    const int WATER = 28; 
    const int SAND = 100; 
    const int STONE = 61; 

    // Generate Biomes
    for (int y = 0; y < mapH; ++y) {
        for (int x = 0; x < mapW; ++x) {
            float nx = x / (float)mapW;
            float ny = y / (float)mapH;
            float noise = std::sin(nx * 10.0f) + std::cos(ny * 10.0f) + std::sin((nx + ny) * 5.0f);
            
            int tile = GRASS;
            if (noise < -1.0f) tile = WATER;
            else if (noise < -0.6f) tile = SAND;
            else if (noise < 0.5f) {
                tile = GRASS;
                if ((x * y) % 10 == 0) tile = GRASS_VAR;
                if ((x * y) % 17 == 0) tile = GRASS_Dense; 
            } else if (noise < 1.2f) tile = DIRT;
            else tile = STONE;
            
            if (x == 0 || x == mapW-1 || y == 0 || y == mapH-1) tile = STONE;
            if (x >= 18 && x <= 22 && y >= 18 && y <= 22) tile = GRASS;

            m_Level->SetTile(x, y, tile); 
        }
    }

    // 2. Setup Player Entity
    m_Player = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(m_Player, PixelsEngine::TransformComponent{ 20.0f, 20.0f });
    GetRegistry().AddComponent(m_Player, PixelsEngine::PlayerComponent{ 5.0f }); 
    GetRegistry().AddComponent(m_Player, PixelsEngine::PathMovementComponent{}); 
    GetRegistry().AddComponent(m_Player, PixelsEngine::StatsComponent{100, 100, 15, false}); 
    
    auto& inv = GetRegistry().AddComponent(m_Player, PixelsEngine::InventoryComponent{});
    inv.AddItem("Potion", 3, PixelsEngine::ItemType::Consumable);
    inv.AddItem("Sword", 1, PixelsEngine::ItemType::WeaponMelee, 10, "assets/sword.png");
    inv.AddItem("Leather Armor", 1, PixelsEngine::ItemType::Armor, 5, "assets/armor.png");
    inv.AddItem("Shortbow", 1, PixelsEngine::ItemType::WeaponRanged, 8, "assets/bow.png");
    
    std::string playerSheet = "assets/Pixel Art Top Down - Basic v1.2.2/Texture/TX Player.png";
    auto playerTexture = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), playerSheet);
    
    GetRegistry().AddComponent(m_Player, PixelsEngine::SpriteComponent{ 
        playerTexture, {0, 0, 32, 32}, 16, 30 
    });

    auto& anim = GetRegistry().AddComponent(m_Player, PixelsEngine::AnimationComponent{});
    anim.AddAnimation("Idle", 0, 0, 32, 32, 1);
    anim.AddAnimation("WalkDown", 0, 0, 32, 32, 4);
    anim.AddAnimation("WalkRight", 0, 0, 32, 32, 4); 
    anim.AddAnimation("WalkUp", 0, 32, 32, 32, 4);
    anim.AddAnimation("WalkLeft", 0, 32, 32, 32, 4);

    // 3. Spawn Boars
    CreateBoar(28.0f, 28.0f);
    CreateBoar(32.0f, 32.0f);
    CreateBoar(25.0f, 35.0f);

    // 4. NPCs
    auto npc1 = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(npc1, PixelsEngine::TransformComponent{ 21.0f, 21.0f });
    GetRegistry().AddComponent(npc1, PixelsEngine::SpriteComponent{ playerTexture, {0, 0, 32, 32}, 16, 30 });
    GetRegistry().AddComponent(npc1, PixelsEngine::InteractionComponent{ "Hello Hunter!", false, 0.0f });
    GetRegistry().AddComponent(npc1, PixelsEngine::StatsComponent{50, 50, 5, false}); 
    GetRegistry().AddComponent(npc1, PixelsEngine::QuestComponent{ "FetchOrb", 0, "Gold Orb" });

    auto npc2 = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(npc2, PixelsEngine::TransformComponent{ 18.0f, 22.0f });
    GetRegistry().AddComponent(npc2, PixelsEngine::SpriteComponent{ playerTexture, {0, 0, 32, 32}, 16, 30 });
    GetRegistry().AddComponent(npc2, PixelsEngine::InteractionComponent{ "Hello Warrior!", false, 0.0f });
    GetRegistry().AddComponent(npc2, PixelsEngine::StatsComponent{50, 50, 5, false}); 
    GetRegistry().AddComponent(npc2, PixelsEngine::QuestComponent{ "HuntBoars", 0, "Boar Meat" });

    // 5. Spawn Gold Orb
    auto orb = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(orb, PixelsEngine::TransformComponent{ 23.0f, 23.0f }); 
    m_Level->SetTile(23, 23, GRASS);
    auto orbTexture = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/gold_orb.png");
    GetRegistry().AddComponent(orb, PixelsEngine::SpriteComponent{ 
        orbTexture, {0, 0, 32, 32}, 16, 16 
    });
    GetRegistry().AddComponent(orb, PixelsEngine::InteractionComponent{ "Gold Orb", false, 0.0f }); 
}

// Temporary storage for creation menu
static int tempStats[6] = {10, 10, 10, 10, 10, 10}; 
static const char* statNames[] = {"Strength", "Dexterity", "Constitution", "Intelligence", "Wisdom", "Charisma"};
static std::string tempClasses[] = {"Fighter", "Rogue", "Wizard", "Cleric"};
static int classIndex = 0;
static std::string tempRaces[] = {"Human", "Elf", "Dwarf", "Halfling"};
static int raceIndex = 0;
static int selectionIndex = 0; 
static int pointsRemaining = 5; 

void PixelsGateGame::RenderCharacterCreation() {
    SDL_Renderer* renderer = GetRenderer();
    m_TextRenderer->RenderTextCentered("CHARACTER CREATION", 400, 50, {255, 215, 0, 255});
    m_TextRenderer->RenderTextCentered("Points Remaining: " + std::to_string(pointsRemaining), 400, 90, {200, 255, 200, 255});

    int y = 130;
    SDL_Color selectedColor = {50, 255, 50, 255}; 
    SDL_Color normalColor = {255, 255, 255, 255};

    for (int i = 0; i < 6; ++i) {
        std::string text = std::string(statNames[i]) + ": " + std::to_string(tempStats[i]);
        SDL_Color c = (selectionIndex == i) ? selectedColor : normalColor;
        m_TextRenderer->RenderText(text, 300, y, c);
        if (selectionIndex == i) m_TextRenderer->RenderText("<   >", 500, y, selectedColor);
        y += 35;
    }
    y += 20;
    {
        std::string text = "Class: " + tempClasses[classIndex];
        SDL_Color c = (selectionIndex == 6) ? selectedColor : normalColor;
        m_TextRenderer->RenderText(text, 300, y, c);
        if (selectionIndex == 6) m_TextRenderer->RenderText("<   >", 550, y, selectedColor);
    }
    y += 35;
    {
        std::string text = "Race: " + tempRaces[raceIndex];
        SDL_Color c = (selectionIndex == 7) ? selectedColor : normalColor;
        m_TextRenderer->RenderText(text, 300, y, c);
        if (selectionIndex == 7) m_TextRenderer->RenderText("<   >", 550, y, selectedColor);
    }
    y += 50;
    SDL_Rect btnRect = { 300, y, 200, 50 };
    if (selectionIndex == 8) SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
    else SDL_SetRenderDrawColor(renderer, 50, 150, 50, 255);
    SDL_RenderFillRect(renderer, &btnRect);
    m_TextRenderer->RenderTextCentered("START ADVENTURE", 400, y + 25, {255, 255, 255, 255});
    m_TextRenderer->RenderTextCentered("Controls: W/S to Select, A/D to Change", 400, 550, {150, 150, 150, 255});
}

void PixelsGateGame::HandleCreationInput() {
    static bool wasUp = false, wasDown = false, wasLeft = false, wasRight = false, wasEnter = false;
    bool isUp = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_W);
    bool isDown = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_S);
    bool isLeft = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A);
    bool isRight = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D);
    bool isEnter = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RETURN) || PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_SPACE);

    bool pressUp = isUp && !wasUp;
    bool pressDown = isDown && !wasDown;
    bool pressLeft = isLeft && !wasLeft;
    bool pressRight = isRight && !wasRight;
    bool pressEnter = isEnter && !wasEnter;

    wasUp = isUp; wasDown = isDown; wasLeft = isLeft; wasRight = isRight; wasEnter = isEnter;

    if (pressUp) { selectionIndex--; if (selectionIndex < 0) selectionIndex = 8; }
    if (pressDown) { selectionIndex++; if (selectionIndex > 8) selectionIndex = 0; }

    if (pressLeft || pressRight) {
        int delta = pressRight ? 1 : -1;
        if (selectionIndex >= 0 && selectionIndex <= 5) {
            int currentVal = tempStats[selectionIndex];
            if (delta > 0) { if (pointsRemaining > 0 && currentVal < 18) { tempStats[selectionIndex]++; pointsRemaining--; } }
            else { if (currentVal > 8) { tempStats[selectionIndex]--; pointsRemaining++; } }
        } else if (selectionIndex == 6) classIndex = (classIndex + delta + 4) % 4;
        else if (selectionIndex == 7) raceIndex = (raceIndex + delta + 4) % 4;
    }

    if (pressEnter || (selectionIndex == 8 && (pressRight || PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT)))) {
        auto* stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
        if (stats) {
            stats->strength = tempStats[0]; stats->dexterity = tempStats[1]; stats->constitution = tempStats[2];
            stats->intelligence = tempStats[3]; stats->wisdom = tempStats[4]; stats->charisma = tempStats[5];
            stats->characterClass = tempClasses[classIndex]; stats->race = tempRaces[raceIndex];
            stats->maxHealth = 10 + stats->GetModifier(stats->constitution) * 2;
            stats->currentHealth = stats->maxHealth;
            stats->damage = (stats->characterClass == "Wizard") ? 12 : (2 + stats->GetModifier(stats->strength));
        }
        m_State = GameState::Playing;
    }
}

void PixelsGateGame::PerformAttack(PixelsEngine::Entity forcedTarget) {
    auto* playerStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    auto* playerTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    if (!playerStats || !playerTrans) return;

    if (m_State == GameState::Combat) {
        if (!m_TurnOrder[m_CurrentTurnIndex].isPlayer) {
             SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "Not Your Turn!", {200, 0, 0, 255});
             return;
        }
        if (m_ActionsLeft <= 0) {
             SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "No Actions Left!", {200, 0, 0, 255});
             return;
        }
    }

    PixelsEngine::Entity target = forcedTarget;

    // 1. Try Context Menu Target
    if (target == PixelsEngine::INVALID_ENTITY && m_ContextMenu.isOpen && m_ContextMenu.targetEntity != PixelsEngine::INVALID_ENTITY) {
        target = m_ContextMenu.targetEntity;
    }
    
    // 2. If no target, find closest enemy
    if (target == PixelsEngine::INVALID_ENTITY) {
        float minDistance = 3.0f; // Increased from 2.0f for better usability
        auto& view = GetRegistry().View<PixelsEngine::StatsComponent>();
        for (auto& [entity, stats] : view) {
            if (entity == m_Player || stats.isDead) continue;
            auto* targetTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
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
        // If not in combat, start combat!
        if (m_State == GameState::Playing) {
            StartCombat(target);
        }

        auto* targetStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(target);
        auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
        if (targetStats && inv) {
            // Check range for forced target too
            auto* targetTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(target);
            float dist = targetTrans ? std::sqrt(std::pow(playerTrans->x - targetTrans->x, 2) + std::pow(playerTrans->y - targetTrans->y, 2)) : 100.0f;
            
            if (dist > 3.0f) { // Increased from 2.0f
                SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "Too Far!", {200, 200, 200, 255});
                return;
            }

            // Hit Roll against AC (Assuming NPCs have default 10 AC + Dex mod for now)
            int targetAC = 10 + targetStats->GetModifier(targetStats->dexterity);
            int roll = PixelsEngine::Dice::Roll(20);
            int attackBonus = playerStats->GetModifier(playerStats->strength) + inv->equippedMelee.statBonus;

            if (roll == 20 || (roll != 1 && roll + attackBonus >= targetAC)) {
                // Hit!
                int dmg = playerStats->damage + inv->equippedMelee.statBonus;
                bool isCrit = (roll == 20);
                if (isCrit) dmg *= 2;
                
                targetStats->currentHealth -= dmg;
                
                if (targetTrans) {
                    std::string dmgText = std::to_string(dmg);
                    if (isCrit) dmgText += "!";
                    SpawnFloatingText(targetTrans->x, targetTrans->y, dmgText, isCrit ? SDL_Color{255, 0, 0, 255} : SDL_Color{255, 255, 255, 255});
                }
            } else {
                // Miss
                if (targetTrans) SpawnFloatingText(targetTrans->x, targetTrans->y, "Miss", {200, 200, 200, 255});
            }

            if (m_State == GameState::Combat) {
                m_ActionsLeft--;
            }

            if (targetStats->currentHealth <= 0) {
                targetStats->currentHealth = 0;
                targetStats->isDead = true;
                
                // Drop Loot
                auto* loot = GetRegistry().GetComponent<PixelsEngine::LootComponent>(target);
                if (loot && targetTrans) {
                    SpawnLootBag(targetTrans->x, targetTrans->y, loot->drops);
                }
                
                // Destroy Entity
                GetRegistry().DestroyEntity(target);
            }
        }
    } else {
        // Whiff
         SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "No Target", {200, 200, 200, 255});
    }
}

void PixelsGateGame::CreateBoar(float x, float y) {
    auto boar = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(boar, PixelsEngine::TransformComponent{ x, y });
    GetRegistry().AddComponent(boar, PixelsEngine::StatsComponent{30, 30, 2, false}); // Reduced damage from 5 to 2
    GetRegistry().AddComponent(boar, PixelsEngine::InteractionComponent{ "Boar", false, 0.0f });
    // Add AI
    GetRegistry().AddComponent(boar, PixelsEngine::AIComponent{ 8.0f, 1.2f, 2.0f, 0.0f, true });
    // Add Loot
    std::vector<PixelsEngine::Item> drops;
    drops.push_back({"Boar Meat", "", 1, PixelsEngine::ItemType::Consumable, 0});
    GetRegistry().AddComponent(boar, PixelsEngine::LootComponent{ drops });

    auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/critters/boar/boar_SE_run_strip.png");
    GetRegistry().AddComponent(boar, PixelsEngine::SpriteComponent{ tex, {0, 0, 41, 25}, 20, 20 });
    auto& anim = GetRegistry().AddComponent(boar, PixelsEngine::AnimationComponent{});
    anim.AddAnimation("Idle", 0, 0, 41, 25, 1);
    anim.AddAnimation("Run", 0, 0, 41, 25, 4);
    anim.Play("Idle");
}

void PixelsGateGame::OnUpdate(float deltaTime) {
    if (m_State == GameState::Creation) { HandleCreationInput(); return; }

    // Update Timers
    if (m_SaveMessageTimer > 0.0f) m_SaveMessageTimer -= deltaTime;

    // Update Day/Night
    UpdateDayNight(deltaTime);

    // Update Floating Texts
    for (auto it = m_FloatingTexts.begin(); it != m_FloatingTexts.end(); ) {
        it->y -= 20.0f * deltaTime; // Float up
        it->life -= deltaTime;
        if (it->life <= 0.0f) {
            it = m_FloatingTexts.erase(it);
        } else {
            ++it;
        }
    }

    if (m_FadeState != FadeState::None) {
        m_FadeTimer -= deltaTime;
        if (m_FadeState == FadeState::FadingOut) {
            if (m_FadeTimer <= 0.0f) {
                PixelsEngine::SaveSystem::LoadGame(m_PendingLoadFile, GetRegistry(), m_Player, *m_Level);
                m_FadeState = FadeState::FadingIn;
                m_FadeTimer = m_FadeDuration;
                if (m_State == GameState::MainMenu) m_State = GameState::Playing;
            }
        } else if (m_FadeState == FadeState::FadingIn) {
            if (m_FadeTimer <= 0.0f) {
                m_FadeState = FadeState::None;
            }
        }
        return; 
    }

    switch (m_State) {
        case GameState::MainMenu:
            HandleMainMenuInput();
            break;
        case GameState::Creation:
            HandleCreationInput();
            break;
        case GameState::Paused:
            HandlePauseMenuInput();
            break;
        case GameState::Options:
            if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_ESCAPE)) m_State = GameState::MainMenu;
            break;
        case GameState::Credits:
        case GameState::Controls:
            break;
        case GameState::Combat:
        {
            static bool wasEsc = false;
            static bool wasI = false;
            bool isEsc = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_ESCAPE);
            bool isI = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_I);
            
            auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
            
            if (isEsc && !wasEsc) {
                if (inv && inv->isOpen) {
                    inv->isOpen = false;
                } else {
                    m_State = GameState::Paused;
                    m_MenuSelection = 0;
                }
            }
            if (isI && !wasI && inv) {
                inv->isOpen = !inv->isOpen;
            }
            wasEsc = isEsc;
            wasI = isI;
            
            UpdateCombat(deltaTime);
            break;
        }
        case GameState::Playing:
        {
            static bool wasEsc = false;
            static bool wasI = false;
            bool isEsc = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_ESCAPE);
            bool isI = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_I);

            auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);

            if (isEsc && !wasEsc) {
                if (inv && inv->isOpen) {
                    inv->isOpen = false;
                } else {
                    m_State = GameState::Paused;
                    m_MenuSelection = 0;
                }
            }
            if (isI && !wasI && inv) {
                inv->isOpen = !inv->isOpen;
            }
            wasEsc = isEsc;
            wasI = isI;

            if (m_State == GameState::Playing) {
                HandleInput();
                UpdateAI(deltaTime); // Update AI

                auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
                auto* playerComp = GetRegistry().GetComponent<PixelsEngine::PlayerComponent>(m_Player);
                auto* anim = GetRegistry().GetComponent<PixelsEngine::AnimationComponent>(m_Player);
                auto* sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(m_Player);
                auto* pathComp = GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(m_Player);

                if (m_SelectedNPC != PixelsEngine::INVALID_ENTITY) {
                    auto* npcTransform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_SelectedNPC);
                    auto* interaction = GetRegistry().GetComponent<PixelsEngine::InteractionComponent>(m_SelectedNPC);
                    if (npcTransform && interaction) {
                        float dist = std::sqrt(std::pow(transform->x - npcTransform->x, 2) + std::pow(transform->y - npcTransform->y, 2));
                        if (dist < 1.5f) {
                            pathComp->isMoving = false; anim->currentFrameIndex = 0; 
                            if (interaction->dialogueText == "Gold Orb") {
                                auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
                                if (inv) { inv->AddItem("Gold Orb", 1); GetRegistry().RemoveComponent<PixelsEngine::SpriteComponent>(m_SelectedNPC); }
                            } else if (interaction->dialogueText == "Loot") {
                                // Loot Bag
                                auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
                                auto* loot = GetRegistry().GetComponent<PixelsEngine::LootComponent>(m_SelectedNPC);
                                if (inv && loot) {
                                    for (const auto& item : loot->drops) {
                                        inv->AddItemObject(item);
                                        SpawnFloatingText(transform->x, transform->y, "+ " + item.name, {255, 255, 0, 255});
                                    }
                                    GetRegistry().DestroyEntity(m_SelectedNPC); // Destroy bag
                                }
                            } else {
                                auto* quest = GetRegistry().GetComponent<PixelsEngine::QuestComponent>(m_SelectedNPC);
                                if (quest) {
                                    auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
                                    if (quest->state == 0) { 
                                        if (quest->questId == "FetchOrb") interaction->dialogueText = "Please find my Gold Orb!";
                                        else if (quest->questId == "HuntBoars") interaction->dialogueText = "The boars are aggressive. Hunt them!";
                                        quest->state = 1; 
                                    } else if (quest->state == 1) {
                                        bool hasItem = false;
                                        for (auto& item : inv->items) { if (item.name == quest->targetItem && item.quantity > 0) hasItem = true; }
                                        if (hasItem) { interaction->dialogueText = "You found it! Thank you."; quest->state = 2; inv->AddItem("Coins", 50); }
                                        else { if (quest->questId == "FetchOrb") interaction->dialogueText = "Bring me the Orb..."; else if (quest->questId == "HuntBoars") interaction->dialogueText = "Bring me Boar Meat."; }
                                    } else if (quest->state == 2) interaction->dialogueText = "Blessings upon you.";
                                }
                                interaction->showDialogue = true; interaction->dialogueTimer = 3.0f;
                            }
                            m_SelectedNPC = PixelsEngine::INVALID_ENTITY;
                        }
                    }
                }

                auto& interactions = GetRegistry().View<PixelsEngine::InteractionComponent>();
                for (auto& [entity, interact] : interactions) {
                    if (interact.showDialogue) { interact.dialogueTimer -= deltaTime; if (interact.dialogueTimer <= 0) interact.showDialogue = false; }
                }

                if (transform && playerComp && anim && sprite && pathComp) {
                    
                    // Update Fog of War based on Player Position
                    m_Level->UpdateVisibility((int)transform->x, (int)transform->y, 5); // 5 tile radius

                    bool wasdInput = false; 
                    float dx = 0, dy = 0; 
                    std::string newAnim = "Idle";

                    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_W)) { dx -= 1; dy -= 1; newAnim = "WalkUp"; wasdInput = true; }
                    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_S)) { dx += 1; dy += 1; newAnim = "WalkDown"; wasdInput = true; }
                    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A)) { dx -= 1; dy += 1; newAnim = "WalkLeft"; wasdInput = true; }
                    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D)) { dx += 1; dy -= 1; newAnim = "WalkRight"; wasdInput = true; }

                    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A)) sprite->flip = SDL_FLIP_HORIZONTAL;
                    else if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D)) sprite->flip = SDL_FLIP_NONE;

                    if (wasdInput) {
                        pathComp->isMoving = false; m_SelectedNPC = PixelsEngine::INVALID_ENTITY;
                        float newX = transform->x + dx * playerComp->speed * deltaTime;
                        float newY = transform->y + dy * playerComp->speed * deltaTime;
                        if (m_Level->IsWalkable((int)newX, (int)newY)) { transform->x = newX; transform->y = newY; }
                        else { if (m_Level->IsWalkable((int)newX, (int)transform->y)) transform->x = newX; else if (m_Level->IsWalkable((int)transform->x, (int)newY)) transform->y = newY; }
                        anim->Play(newAnim);
                    } else if (pathComp->isMoving) {
                        float targetX = pathComp->targetX, targetY = pathComp->targetY;
                        float diffX = targetX - transform->x, diffY = targetY - transform->y;
                        float dist = std::sqrt(diffX*diffX + diffY*diffY);
                        if (dist < 0.1f) {
                            transform->x = targetX; transform->y = targetY;
                            pathComp->currentPathIndex++;
                            if (pathComp->currentPathIndex >= pathComp->path.size()) { pathComp->isMoving = false; anim->currentFrameIndex = 0; }
                            else { pathComp->targetX = (float)pathComp->path[pathComp->currentPathIndex].first; pathComp->targetY = (float)pathComp->path[pathComp->currentPathIndex].second; }
                        } else {
                            float moveX = (diffX / dist) * playerComp->speed * deltaTime;
                            float moveY = (diffY / dist) * playerComp->speed * deltaTime;
                            transform->x += moveX; transform->y += moveY;
                            if (diffX < 0 && diffY < 0) anim->Play("WalkUp");
                            else if (diffX > 0 && diffY > 0) anim->Play("WalkDown");
                            else if (diffX < 0 && diffY > 0) { anim->Play("WalkLeft"); sprite->flip = SDL_FLIP_HORIZONTAL; }
                            else if (diffX > 0 && diffY < 0) { anim->Play("WalkRight"); sprite->flip = SDL_FLIP_NONE; }
                        }
                    } else anim->currentFrameIndex = 0;

                    if (wasdInput || pathComp->isMoving) {
                        anim->timer += deltaTime;
                        auto& currentAnim = anim->animations[anim->currentAnimationIndex];
                        if (anim->timer >= currentAnim.frameDuration) { anim->timer = 0.0f; anim->currentFrameIndex = (anim->currentFrameIndex + 1) % currentAnim.frames.size(); }
                        sprite->srcRect = currentAnim.frames[anim->currentFrameIndex];
                    } else {
                         auto& currentAnim = anim->animations[anim->currentAnimationIndex];
                         sprite->srcRect = currentAnim.frames[0];
                    }

                    int screenX, screenY; m_Level->GridToScreen(transform->x, transform->y, screenX, screenY);
                    auto& camera = GetCamera(); camera.x = screenX - camera.width / 2; camera.y = screenY - camera.height / 2;
                }
            }
        }
        break;
    }
}

void PixelsGateGame::OnRender() {
    switch (m_State) {
        case GameState::MainMenu:
            RenderMainMenu();
            break;
        case GameState::Creation:
            RenderCharacterCreation();
            break;
        case GameState::Options:
            RenderOptions();
            break;
        case GameState::Credits:
            RenderCredits();
            break;
        case GameState::Controls:
            RenderControls();
            break;
        case GameState::Paused:
        case GameState::Playing:
        case GameState::Combat:
        {
            auto& camera = GetCamera();
                if (m_Level) m_Level->Render(camera);
            
                RenderEnemyCones(camera);
            
                struct Renderable { int y; PixelsEngine::Entity entity; };            std::vector<Renderable> renderQueue;
            auto& sprites = GetRegistry().View<PixelsEngine::SpriteComponent>();
            for (auto& [entity, sprite] : sprites) {
                auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
                if (transform && m_Level) {
                    // Check Fog of War
                    if (entity != m_Player) {
                        if (!m_Level->IsVisible((int)transform->x, (int)transform->y)) {
                            continue; // Skip rendering hidden entity
                        }
                    }
        
                    int screenX, screenY;
                    m_Level->GridToScreen(transform->x, transform->y, screenX, screenY);
                    renderQueue.push_back({ screenY, entity });
                }
            }
        
            std::sort(renderQueue.begin(), renderQueue.end(), [](const Renderable& a, const Renderable& b) { return a.y < b.y; });

            for (const auto& item : renderQueue) {
                auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(item.entity);
                auto* sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(item.entity);
                if (transform && sprite && sprite->texture) {
                    int screenX, screenY; m_Level->GridToScreen(transform->x, transform->y, screenX, screenY);
                    screenX -= (int)camera.x; screenY -= (int)camera.y;
                            sprite->texture->RenderRect(screenX + 16 - sprite->pivotX, screenY + 16 - sprite->pivotY, &sprite->srcRect, -1, -1, sprite->flip);
                    
                            // --- Render Health Bar Above Entity (in Combat or if damaged) ---
                            auto* entStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(item.entity);
                            if (entStats && (m_State == GameState::Combat || entStats->currentHealth < entStats->maxHealth) && !entStats->isDead) {
                                int barW = 32;
                                int barH = 4;
                                int barX = screenX;
                                int barY = screenY - 8;
                    
                                SDL_Rect bg = {barX, barY, barW, barH};
                                SDL_SetRenderDrawColor(GetRenderer(), 50, 50, 50, 255);
                                SDL_RenderFillRect(GetRenderer(), &bg);
                    
                                float hpPercent = (float)entStats->currentHealth / (float)entStats->maxHealth;
                                SDL_Rect fg = {barX, barY, (int)(barW * hpPercent), barH};
                                SDL_SetRenderDrawColor(GetRenderer(), 255, 0, 0, 255);
                                SDL_RenderFillRect(GetRenderer(), &fg);
                            }
                        }            }
            // Render Floating Texts
            for (const auto& ft : m_FloatingTexts) {
                int screenX, screenY;
                m_Level->GridToScreen(ft.x, ft.y, screenX, screenY);
                screenX -= (int)camera.x;
                screenY -= (int)camera.y;
                m_TextRenderer->RenderTextCentered(ft.text, screenX + 16, screenY - 20, ft.color);
            }

            RenderHUD(); 
            if (m_State == GameState::Combat) RenderCombatUI();
            
            RenderInventory(); RenderContextMenu(); RenderDiceRoll();
            RenderDayNightCycle(); // Render tint on top

            // Render Save Alert
            if (m_SaveMessageTimer > 0.0f) {
                m_TextRenderer->RenderText("Saving...", 20, 80, {255, 255, 0, 255}); // Below HP bar
            }

            // Render Fade Overlay
            if (m_FadeState != FadeState::None) {
                SDL_Renderer* renderer = GetRenderer();
                int w, h;
                SDL_GetWindowSize(m_Window, &w, &h);
                
                float alpha = 0.0f;
                if (m_FadeState == FadeState::FadingOut) {
                    alpha = 1.0f - (m_FadeTimer / m_FadeDuration);
                } else {
                    alpha = (m_FadeTimer / m_FadeDuration);
                }
                if (alpha < 0.0f) alpha = 0.0f;
                if (alpha > 1.0f) alpha = 1.0f;

                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)(alpha * 255));
                SDL_Rect screenRect = {0, 0, w, h};
                SDL_RenderFillRect(renderer, &screenRect);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }
            
            // If Paused, render Pause Menu on top
            if (m_State == GameState::Paused) {
                RenderPauseMenu();
            }
        }
        break;
    }
}

void PixelsGateGame::StartDiceRoll(int modifier, int dc, const std::string& skill, PixelsEngine::Entity target, PixelsEngine::ContextActionType type) {
    m_DiceRoll.active = true; m_DiceRoll.timer = 0.0f; m_DiceRoll.resultShown = false;
    m_DiceRoll.modifier = modifier; m_DiceRoll.dc = dc; m_DiceRoll.skillName = skill;
    m_DiceRoll.target = target; m_DiceRoll.actionType = type;
    int roll = PixelsEngine::Dice::Roll(20); m_DiceRoll.finalValue = roll;
    m_DiceRoll.success = (roll + modifier >= dc) || (roll == 20);
}

void PixelsGateGame::ResolveDiceRoll() {
    if (m_DiceRoll.success) {
        if (m_DiceRoll.actionType == PixelsEngine::ContextActionType::Pickpocket) {
            std::cout << "Pickpocket Success!" << std::endl;
            auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
            if (inv) inv->AddItem("Coins", 10);
        }
    } else {
        if (m_DiceRoll.actionType == PixelsEngine::ContextActionType::Pickpocket) {
             auto* interact = GetRegistry().GetComponent<PixelsEngine::InteractionComponent>(m_DiceRoll.target);
             if (interact) { interact->dialogueText = "Hey! Hands off!"; interact->showDialogue = true; interact->dialogueTimer = 2.0f; }
        }
    }
}

void PixelsGateGame::RenderDiceRoll() {
    if (!m_DiceRoll.active) return;
    SDL_Renderer* renderer = GetRenderer();
    int winW, winH; SDL_GetWindowSize(m_Window, &winW, &winH);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150); SDL_Rect overlay = {0, 0, winW, winH}; SDL_RenderFillRect(renderer, &overlay);
    int boxW = 300, boxH = 300; SDL_Rect box = {(winW - boxW)/2, (winH - boxH)/2, boxW, boxH};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255); SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); SDL_RenderDrawRect(renderer, &box);
    m_TextRenderer->RenderTextCentered("Skill Check: " + m_DiceRoll.skillName, box.x + boxW/2, box.y + 30, {255, 255, 255, 255});
    m_TextRenderer->RenderTextCentered("Difficulty Class: " + std::to_string(m_DiceRoll.dc), box.x + boxW/2, box.y + 60, {200, 200, 200, 255});
    int displayVal = m_DiceRoll.resultShown ? m_DiceRoll.finalValue : PixelsEngine::Dice::Roll(20);
    m_TextRenderer->RenderTextCentered(std::to_string(displayVal), box.x + boxW/2, box.y + 120, {255, 255, 0, 255});
    m_TextRenderer->RenderTextCentered("Bonus: " + std::string((m_DiceRoll.modifier >= 0) ? "+" : "") + std::to_string(m_DiceRoll.modifier), box.x + boxW/2, box.y + 170, {100, 255, 100, 255});
    if (m_DiceRoll.resultShown) {
        std::string resultText = m_DiceRoll.success ? "SUCCESS" : "FAILURE";
        SDL_Color resColor = m_DiceRoll.success ? SDL_Color{50, 255, 50, 255} : SDL_Color{255, 50, 50, 255};
        m_TextRenderer->RenderTextCentered(resultText, box.x + boxW/2, box.y + 220, resColor);
        m_TextRenderer->RenderTextCentered("Click to Continue", box.x + boxW/2, box.y + 260, {255, 255, 255, 255});
    } else {
        auto* stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
        if (stats && stats->inspiration > 0) m_TextRenderer->RenderTextCentered("Press SPACE to use Inspiration (Reroll)", box.x + boxW/2, box.y + 260, {100, 200, 255, 255});
    }
}

void PixelsGateGame::RenderContextMenu() {
    if (!m_ContextMenu.isOpen) return;
    SDL_Renderer* renderer = GetRenderer();
    int w = 150, h = m_ContextMenu.actions.size() * 30 + 10;
    SDL_Rect menuRect = { m_ContextMenu.x, m_ContextMenu.y, w, h };
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); SDL_RenderFillRect(renderer, &menuRect);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); SDL_RenderDrawRect(renderer, &menuRect);
    int y = m_ContextMenu.y + 5;
    for (const auto& action : m_ContextMenu.actions) { m_TextRenderer->RenderText(action.label, m_ContextMenu.x + 10, y, {255, 255, 255, 255}); y += 30; }
}

void PixelsGateGame::RenderInventory() {
    auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if (!inv || !inv->isOpen) return;
    
    HandleInventoryInput(); // Handle clicks inside inventory

    SDL_Renderer* renderer = GetRenderer();
    int winW, winH; SDL_GetWindowSize(m_Window, &winW, &winH);
    int w = 400, h = 500; 
    SDL_Rect panel = { (winW - w) / 2, (winH - h) / 2, w, h };
    
    SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 218, 165, 32, 255); SDL_RenderDrawRect(renderer, &panel);
    
    m_TextRenderer->RenderTextCentered("INVENTORY", panel.x + w/2, panel.y + 20, {255, 255, 255, 255});

    // Close Button (X in top right)
    SDL_Rect closeBtn = { panel.x + w - 30, panel.y + 10, 20, 20 };
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255); SDL_RenderFillRect(renderer, &closeBtn);
    m_TextRenderer->RenderTextCentered("X", closeBtn.x + 10, closeBtn.y + 10, {255, 255, 255, 255});

    // --- Equipment Slots (Top) ---
    int slotSize = 40;
    int startX = panel.x + 50;
    int startY = panel.y + 60;
    
    auto DrawSlot = [&](const std::string& label, PixelsEngine::Item& item, int x, int y) {
        SDL_Rect slotRect = {x, y, slotSize, slotSize};
        SDL_SetRenderDrawColor(renderer, 50, 30, 10, 255); SDL_RenderFillRect(renderer, &slotRect);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); SDL_RenderDrawRect(renderer, &slotRect);
        
        m_TextRenderer->RenderTextCentered(label, x + slotSize/2, y - 15, {200, 200, 200, 255});
        if (!item.IsEmpty()) {
            if (!item.iconPath.empty()) {
                auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), item.iconPath);
                if (tex) tex->Render(x + 4, y + 4, 32, 32);
            } else {
                m_TextRenderer->RenderTextCentered(item.name.substr(0, 3), x + slotSize/2, y + slotSize/2, {255, 255, 0, 255});
            }
        }
    };

    DrawSlot("Melee", inv->equippedMelee, startX, startY);
    DrawSlot("Range", inv->equippedRanged, startX + 100, startY);
    DrawSlot("Armor", inv->equippedArmor, startX + 200, startY);

    // --- Stats Summary ---
    auto* stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    if (stats) {
        int armorBonus = inv->equippedArmor.statBonus;
        int damageBonus = inv->equippedMelee.statBonus + inv->equippedRanged.statBonus;
        std::string statStr = "AC: " + std::to_string(10 + stats->GetModifier(stats->dexterity) + armorBonus) + 
                              "  Dmg Bonus: " + std::to_string(damageBonus);
        m_TextRenderer->RenderTextCentered(statStr, panel.x + w/2, startY + 60, {200, 255, 200, 255});
    }

    // --- Item List ---
    int listY = startY + 90;
    SDL_Rect listArea = { panel.x + 20, listY, w - 40, h - (listY - panel.y) - 20 };
    SDL_SetRenderDrawColor(renderer, 80, 40, 10, 255); SDL_RenderFillRect(renderer, &listArea);
    
    int itemY = listArea.y + 10;
    for (int i = 0; i < inv->items.size(); ++i) {
        auto& item = inv->items[i];
        
        // Simple Hover logic
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        SDL_Rect rowRect = { listArea.x, itemY, listArea.w, 40 }; // Increased height for icons
        bool hover = (mx >= rowRect.x && mx <= rowRect.x + rowRect.w && my >= rowRect.y && my <= rowRect.y + rowRect.h);
        
        if (hover) SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
        else SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); 
        if (hover) SDL_RenderFillRect(renderer, &rowRect);

        RenderInventoryItem(item, listArea.x + 10, itemY + 4);
        itemY += 45;
    }

    // --- Drag and Drop Visual ---
    if (m_DraggingItemIndex != -1 && m_DraggingItemIndex < inv->items.size()) {
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        RenderInventoryItem(inv->items[m_DraggingItemIndex], mx - 16, my - 16);
    }
}

void PixelsGateGame::RenderInventoryItem(const PixelsEngine::Item& item, int x, int y) {
    if (!item.iconPath.empty()) {
        auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), item.iconPath);
        if (tex) tex->Render(x, y, 32, 32);
    }
    
    std::string typeStr = "";
    if (item.type == PixelsEngine::ItemType::WeaponMelee) typeStr = "[M]";
    else if (item.type == PixelsEngine::ItemType::WeaponRanged) typeStr = "[R]";
    else if (item.type == PixelsEngine::ItemType::Armor) typeStr = "[A]";
    
    std::string display = item.name + " x" + std::to_string(item.quantity) + " " + typeStr;
    m_TextRenderer->RenderText(display, x + 40, y + 8, {255, 255, 255, 255});
}

void PixelsGateGame::HandleInput() {
    static bool wasDownLeft = false, wasDownRight = false;
    bool isCtrlDown = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_LCTRL) || PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RCTRL);
    bool isDownLeftRaw = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT);
    bool isDownRightRaw = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_RIGHT);
    
    // Separate Left Click and Right Click (or Ctrl+LeftClick)
    bool isPressedLeft = isDownLeftRaw && !wasDownLeft;
    bool isPressedRight = isDownRightRaw && !wasDownRight;
    
    wasDownLeft = isDownLeftRaw; wasDownRight = isDownRightRaw;
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);

    if (m_DiceRoll.active) {
        if (!m_DiceRoll.resultShown) {
            m_DiceRoll.timer += 0.016f; // approx delta
            if (m_DiceRoll.timer > m_DiceRoll.duration) {
                m_DiceRoll.resultShown = true;
                ResolveDiceRoll();
            }
            
            // Handle Inspiration
            if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_SPACE)) {
                auto* stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
                if (stats && stats->inspiration > 0) {
                    stats->inspiration--;
                    // Reroll
                    m_DiceRoll.finalValue = PixelsEngine::Dice::Roll(20);
                    m_DiceRoll.success = (m_DiceRoll.finalValue + m_DiceRoll.modifier >= m_DiceRoll.dc) || (m_DiceRoll.finalValue == 20);
                    m_DiceRoll.timer = 0.0f; // Restart anim
                    std::cout << "Used Inspiration! Rerolling..." << std::endl;
                }
            }
        } else {
            // Click to close
            if (isPressedLeft) {
                m_DiceRoll.active = false;
            }
        }
        return; // Block other input
    }

    // Quick Save/Load
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_F5)) {
        PixelsEngine::SaveSystem::SaveGame("quicksave.dat", GetRegistry(), m_Player, *m_Level);
        ShowSaveMessage();
    }
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_F8)) {
        TriggerLoadTransition("quicksave.dat");
    }

    if (m_ContextMenu.isOpen) {
        if (isPressedLeft) {
            int w = 150, h = m_ContextMenu.actions.size() * 30 + 10;
            SDL_Rect menuRect = { m_ContextMenu.x, m_ContextMenu.y, w, h };
            if (mx >= menuRect.x && mx <= menuRect.x + w && my >= menuRect.y && my <= menuRect.y + h) {
                int index = (my - m_ContextMenu.y - 5) / 30;
                if (index >= 0 && index < m_ContextMenu.actions.size()) {
                    auto& action = m_ContextMenu.actions[index];
                    if (action.type == PixelsEngine::ContextActionType::Talk) m_SelectedNPC = m_ContextMenu.targetEntity;
                    else if (action.type == PixelsEngine::ContextActionType::Attack) PerformAttack(m_ContextMenu.targetEntity);
                    else if (action.type == PixelsEngine::ContextActionType::Pickpocket) {
                        auto* stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
                        StartDiceRoll(stats->GetModifier(stats->dexterity), 15, "Dexterity (Sleight of Hand)", m_ContextMenu.targetEntity, PixelsEngine::ContextActionType::Pickpocket);
                    }
                }
            }
            m_ContextMenu.isOpen = false;
        } else if (isPressedRight) m_ContextMenu.isOpen = false;
        return;
    }

    if (isPressedLeft) {
        int winW, winH; SDL_GetWindowSize(m_Window, &winW, &winH);
        if (my > winH - 60) CheckUIInteraction(mx, my);
        else {
            // Check for Ctrl + Click Attack
            if (isCtrlDown) {
                auto& camera = GetCamera(); auto& transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
                bool found = false;
                for (auto& [entity, transform] : transforms) {
                    if (entity == m_Player) continue;
                    auto* sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
                    if (sprite) {
                        int screenX, screenY; m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
                        screenX -= (int)camera.x; screenY -= (int)camera.y;
                        SDL_Rect drawRect = { screenX + 16 - sprite->pivotX, screenY + 16 - sprite->pivotY, sprite->srcRect.w, sprite->srcRect.h };
                        if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w && my >= drawRect.y && my <= drawRect.y + drawRect.h) {
                            PerformAttack(entity);
                            found = true; break;
                        }
                    }
                }
                if (!found) CheckWorldInteraction(mx, my); // Fallback to move
            } else {
                CheckWorldInteraction(mx, my);
            }
        }
    }
    
    if (isPressedRight) {
        auto& camera = GetCamera(); auto& transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
        bool found = false;
        for (auto& [entity, transform] : transforms) {
            if (entity == m_Player) continue;
            auto* sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
            if (sprite) {
                int screenX, screenY; m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
                screenX -= (int)camera.x; screenY -= (int)camera.y;
                SDL_Rect drawRect = { screenX + 16 - sprite->pivotX, screenY + 16 - sprite->pivotY, sprite->srcRect.w, sprite->srcRect.h };
                if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w && my >= drawRect.y && my <= drawRect.y + drawRect.h) {
                    m_ContextMenu.isOpen = true; m_ContextMenu.x = mx; m_ContextMenu.y = my; m_ContextMenu.targetEntity = entity;
                    m_ContextMenu.actions.clear();
                    m_ContextMenu.actions.push_back({"Attack", PixelsEngine::ContextActionType::Attack});
                    if (GetRegistry().HasComponent<PixelsEngine::InteractionComponent>(entity)) { m_ContextMenu.actions.push_back({"Talk", PixelsEngine::ContextActionType::Talk}); m_ContextMenu.actions.push_back({"Pickpocket", PixelsEngine::ContextActionType::Pickpocket}); }
                    found = true; break; 
                }
            }
        }
        if (!found) m_ContextMenu.isOpen = false;
    }
}

void PixelsGateGame::CheckUIInteraction(int mx, int my) {
    int winW, winH; SDL_GetWindowSize(m_Window, &winW, &winH);
    for (int i = 0; i < 6; ++i) {
        SDL_Rect btnRect = { 20 + (i * 55), winH - 50, 40, 40 };
        if (mx >= btnRect.x && mx <= btnRect.x + btnRect.w && my >= btnRect.y && my <= btnRect.y + btnRect.h) {
            if (i == 0) PerformAttack();
            else if (i == 2) { auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player); if (inv) inv->isOpen = !inv->isOpen; }
            else if (i == 5) { // Opt -> Save/System
                // For now, let's just SAVE on click.
                // In a real game, this would open a menu.
                PixelsEngine::SaveSystem::SaveGame("savegame.dat", GetRegistry(), m_Player, *m_Level);
                ShowSaveMessage();
                // Also restore HP/Inspiration as a bonus "Rest" (previous functionality)
                auto* stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player); 
                if (stats) { stats->currentHealth = stats->maxHealth; stats->inspiration = 1; }
            }
        }
    }
}

void PixelsGateGame::CheckWorldInteraction(int mx, int my) {
    auto& camera = GetCamera(); auto& transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
    bool clickedEntity = false;
    for (auto& [entity, transform] : transforms) {
        if (entity == m_Player) continue; 
        auto* sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
        if (sprite) {
            int screenX, screenY; m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
            screenX -= (int)camera.x; screenY -= (int)camera.y;
            SDL_Rect drawRect = { screenX + 16 - sprite->pivotX, screenY + 16 - sprite->pivotY, sprite->srcRect.w, sprite->srcRect.h };
            if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w && my >= drawRect.y && my <= drawRect.y + drawRect.h) {
                m_SelectedNPC = entity; clickedEntity = true;
                auto* pathComp = GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(m_Player);
                auto* pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
                auto path = PixelsEngine::Pathfinding::FindPath(*m_Level, (int)pTrans->x, (int)pTrans->y, (int)transform.x, (int)transform.y);
                if (!path.empty()) { pathComp->path = path; pathComp->currentPathIndex = 0; pathComp->isMoving = true; pathComp->targetX = (float)path[0].first; pathComp->targetY = (float)path[0].second; }
                break;
            }
        }
    }
    if (!clickedEntity) {
        int worldX = mx + camera.x, worldY = my + camera.y, gridX, gridY; m_Level->ScreenToGrid(worldX, worldY, gridX, gridY);
        m_SelectedNPC = PixelsEngine::INVALID_ENTITY;
        auto* pathComp = GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(m_Player);
        auto* pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
        auto path = PixelsEngine::Pathfinding::FindPath(*m_Level, (int)pTrans->x, (int)pTrans->y, gridX, gridY);
        if (!path.empty()) { pathComp->path = path; pathComp->currentPathIndex = 0; pathComp->isMoving = true; pathComp->targetX = (float)path[0].first; pathComp->targetY = (float)path[0].second; }
    }
}

void PixelsGateGame::RenderHUD() {
    SDL_Renderer* renderer = GetRenderer();
    int winW, winH;
    SDL_GetWindowSize(m_Window, &winW, &winH);

    // --- 1. Health Bar (Top Left) ---
    auto* stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    if (stats) {
        int barW = 200;
        int barH = 20;
        int x = 20;
        int y = 20;

        // Background (Grey)
        SDL_Rect bgRect = {x, y, barW, barH};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &bgRect);

        // Foreground (Red)
        float hpPercent = (float)stats->currentHealth / (float)stats->maxHealth;
        if (hpPercent < 0) hpPercent = 0;
        SDL_Rect fgRect = {x, y, (int)(barW * hpPercent), barH};
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        SDL_RenderFillRect(renderer, &fgRect);

        // Border (White)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &bgRect);

        // Text
        std::string hpText = "HP: " + std::to_string(stats->currentHealth) + " / " + std::to_string(stats->maxHealth);
        m_TextRenderer->RenderText(hpText, x + 10, y + 25, {255, 255, 255, 255});
    }

    // --- 2. Minimap (Top Right) ---
    if (m_Level) {
        int mapW = m_Level->GetWidth();
        int mapH = m_Level->GetHeight();
        int tileSize = 4; // Size of each tile on minimap
        int miniW = mapW * tileSize;
        int miniH = mapH * tileSize;
        int mx = winW - miniW - 20;
        int my = 20;

        // Background / Border
        SDL_Rect miniRect = {mx - 2, my - 2, miniW + 4, miniH + 4};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &miniRect);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(renderer, &miniRect);

        // Draw Tiles
        for (int ty = 0; ty < mapH; ++ty) {
            for (int tx = 0; tx < mapW; ++tx) {
                // Check Exploration
                if (!m_Level->IsExplored(tx, ty)) continue;

                int tileIdx = m_Level->GetTile(tx, ty);
                SDL_Color c = {0, 0, 0, 255};

                // Determine Color based on tile index
                // 0, 1, 10 = Grass
                if (tileIdx == 0 || tileIdx == 1 || tileIdx == 10) c = {34, 139, 34, 255}; // Green
                else if (tileIdx == 2) c = {139, 69, 19, 255}; // Brown (Dirt)
                else if (tileIdx == 28) c = {30, 144, 255, 255}; // Blue (Water)
                else if (tileIdx == 100) c = {210, 180, 140, 255}; // Tan (Sand)
                else if (tileIdx == 61) c = {128, 128, 128, 255}; // Grey (Stone)
                else c = {50, 200, 50, 255}; // Default Greenish

                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
                SDL_Rect tileRect = {mx + tx * tileSize, my + ty * tileSize, tileSize, tileSize};
                SDL_RenderFillRect(renderer, &tileRect);
            }
        }

        // Draw Player on Minimap
        auto* trans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
        if (trans) {
            int px = (int)trans->x;
            int py = (int)trans->y;
            SDL_Rect playerDot = {mx + px * tileSize, my + py * tileSize, tileSize, tileSize};
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red Dot
            SDL_RenderFillRect(renderer, &playerDot);
        }
    }

    // --- 3. Bottom Action Bar (Existing) ---
    SDL_Rect hudRect = { 0, winH - 60, winW, 60 };
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); SDL_RenderFillRect(renderer, &hudRect);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); SDL_RenderDrawRect(renderer, &hudRect);
    const char* labels[] = { "Atk", "Mag", "Inv", "Map", "Chr", "Opt" };
    for (int i = 0; i < 6; ++i) {
        SDL_Rect btnRect = { 20 + (i * 55), winH - 50, 40, 40 };
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); SDL_RenderFillRect(renderer, &btnRect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); SDL_RenderDrawRect(renderer, &btnRect);
        m_TextRenderer->RenderTextCentered(labels[i], btnRect.x + 20, btnRect.y + 20, {255, 255, 255, 255});
    }
    auto& camera = GetCamera(); auto& interactions = GetRegistry().View<PixelsEngine::InteractionComponent>();
    for (auto& [entity, interact] : interactions) {
        if (interact.showDialogue) {
            auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
            if (transform) {
                 int screenX, screenY; m_Level->GridToScreen(transform->x, transform->y, screenX, screenY);
                 screenX -= (int)camera.x; screenY -= (int)camera.y;
                 int w = interact.dialogueText.length() * 10, h = 30; SDL_Rect bubble = { screenX + 16 - w/2, screenY - 40, w, h };
                 SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200); SDL_RenderFillRect(renderer, &bubble);
                 SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); SDL_RenderDrawRect(renderer, &bubble);
                 m_TextRenderer->RenderTextCentered(interact.dialogueText, bubble.x + w/2, bubble.y + h/2, {0, 0, 0, 255});
            }
        }
    }
}

// --- Menu Implementations ---

void PixelsGateGame::HandleMenuNavigation(int numOptions, std::function<void(int)> onSelect, std::function<void()> onCancel, int forceSelection) {
    static bool wasUp = false, wasDown = false, wasEnter = false, wasEsc = false, wasMouseClick = false;
    bool isUp = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_W) || PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_UP);
    bool isDown = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_S) || PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_DOWN);
    bool isEnter = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RETURN) || PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_SPACE);
    bool isEsc = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_ESCAPE);
    bool isMouseClick = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT);

    bool pressUp = isUp && !wasUp;
    bool pressDown = isDown && !wasDown;
    bool pressEnter = isEnter && !wasEnter;
    bool pressEsc = isEsc && !wasEsc;
    bool pressMouse = isMouseClick && !wasMouseClick;

    wasUp = isUp; wasDown = isDown; wasEnter = isEnter; wasEsc = isEsc; wasMouseClick = isMouseClick;

    if (forceSelection != -1) {
        m_MenuSelection = forceSelection;
    }

    if (pressUp) {
        m_MenuSelection--;
        if (m_MenuSelection < 0) m_MenuSelection = numOptions - 1;
    }
    if (pressDown) {
        m_MenuSelection++;
        if (m_MenuSelection >= numOptions) m_MenuSelection = 0;
    }
    if ((pressEnter || (pressMouse && forceSelection != -1)) && onSelect) {
        onSelect(m_MenuSelection);
    }
    if (pressEsc && onCancel) {
        onCancel();
    }
}

void PixelsGateGame::HandleMainMenuInput() {
    int w, h; SDL_GetWindowSize(m_Window, &w, &h);
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
    
    int hovered = -1;
    // Layout MUST match RenderMainMenu
    int y = 250;
    int numOptions = 6;
    for (int i = 0; i < numOptions; ++i) {
        // Assume text height ~30px, allow some padding.
        // Text is centered. Width is variable but let's assume a safe click area of 300px width.
        SDL_Rect itemRect = { (w / 2) - 150, y - 5, 300, 30 };
        if (mx >= itemRect.x && mx <= itemRect.x + itemRect.w && my >= itemRect.y && my <= itemRect.y + itemRect.h) {
            hovered = i;
        }
        y += 40;
    }

    HandleMenuNavigation(6, [&](int selection) {
        switch (selection) {
            case 0: // Continue
                if (std::filesystem::exists("savegame.dat")) {
                    TriggerLoadTransition("savegame.dat");
                } else if (std::filesystem::exists("quicksave.dat")) {
                     TriggerLoadTransition("quicksave.dat");
                }
                break;
            case 1: // New Game
                m_State = GameState::Creation;
                selectionIndex = 0; // Reset creation selection
                break;
            case 2: // Load Game
                if (std::filesystem::exists("savegame.dat")) TriggerLoadTransition("savegame.dat");
                break;
            case 3: // Options
                m_State = GameState::Options;
                m_MenuSelection = 0;
                break;
            case 4: // Credits
                m_State = GameState::Credits;
                break;
            case 5: // Quit
                m_IsRunning = false;
                break;
        }
    }, nullptr, hovered);
}

void PixelsGateGame::RenderMainMenu() {
    // 1. Render Background (Level)
    if (m_Level) {
        // Auto-scroll camera for effect
        static float scrollX = 0;
        scrollX += 0.5f;
        auto& camera = GetCamera();
        camera.x = (int)scrollX % (m_Level->GetWidth() * 32); 
        camera.y = 200; // Fixed Y
        
        m_Level->Render(camera);
    }

    // 2. Dark Overlay
    SDL_Renderer* renderer = GetRenderer();
    int w, h;
    SDL_GetWindowSize(m_Window, &w, &h);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    SDL_Rect overlay = {0, 0, w, h};
    SDL_RenderFillRect(renderer, &overlay);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // 3. Title
    m_TextRenderer->RenderTextCentered("PIXELS GATE", w / 2, 100, {255, 215, 0, 255});

    // 4. Menu Options
    std::vector<std::string> options = { "Continue", "New Game", "Load Game", "Options", "Credits", "Quit" };
    int y = 250;
    for (int i = 0; i < options.size(); ++i) {
        SDL_Color color = (i == m_MenuSelection) ? SDL_Color{50, 255, 50, 255} : SDL_Color{200, 200, 200, 255};
        m_TextRenderer->RenderTextCentered(options[i], w / 2, y, color);
        if (i == m_MenuSelection) {
             m_TextRenderer->RenderText(">", w / 2 - 100, y, color);
             m_TextRenderer->RenderText("<", w / 2 + 100, y, color);
        }
        y += 40;
    }
    
    m_TextRenderer->RenderTextCentered("v1.0.0", w - 50, h - 30, {100, 100, 100, 255});
}


void PixelsGateGame::RenderPauseMenu() {
    // 1. Dark Overlay (on top of frozen game)
    SDL_Renderer* renderer = GetRenderer();
    int w, h;
    SDL_GetWindowSize(m_Window, &w, &h);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    SDL_Rect overlay = {0, 0, w, h};
    SDL_RenderFillRect(renderer, &overlay);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // 2. Menu Box
    int boxW = 300, boxH = 400;
    SDL_Rect box = {(w - boxW)/2, (h - boxH)/2, boxW, boxH};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &box);

    m_TextRenderer->RenderTextCentered("PAUSED", w / 2, box.y + 30, {255, 255, 255, 255});

    std::vector<std::string> options = { "Resume", "Save Game", "Load Game", "Controls", "Options", "Main Menu" };
    int y = box.y + 80;
    for (int i = 0; i < options.size(); ++i) {
        SDL_Color color = (i == m_MenuSelection) ? SDL_Color{50, 255, 50, 255} : SDL_Color{200, 200, 200, 255};
        m_TextRenderer->RenderTextCentered(options[i], w / 2, y, color);
        y += 40;
    }
}

void PixelsGateGame::HandlePauseMenuInput() {
    int w, h; SDL_GetWindowSize(m_Window, &w, &h);
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);

    int boxW = 300, boxH = 400;
    SDL_Rect box = {(w - boxW)/2, (h - boxH)/2, boxW, boxH};
    
    int hovered = -1;
    // Layout MUST match RenderPauseMenu
    int y = box.y + 80;
    int numOptions = 6;
    for (int i = 0; i < numOptions; ++i) {
        SDL_Rect itemRect = { (w / 2) - 100, y - 5, 200, 30 }; // Smaller click area for pause menu items?
        if (mx >= itemRect.x && mx <= itemRect.x + itemRect.w && my >= itemRect.y && my <= itemRect.y + itemRect.h) {
            hovered = i;
        }
        y += 40;
    }

    HandleMenuNavigation(6, [&](int selection) {
        switch (selection) {
            case 0: // Resume
                m_State = GameState::Playing;
                break;
            case 1: // Save
                PixelsEngine::SaveSystem::SaveGame("savegame.dat", GetRegistry(), m_Player, *m_Level);
                ShowSaveMessage();
                break;
            case 2: // Load
                TriggerLoadTransition("savegame.dat");
                m_State = GameState::Playing; // Will happen after fade
                break;
            case 3: // Controls
                m_State = GameState::Controls;
                break;
            case 4: // Options
                m_State = GameState::Options;
                m_MenuSelection = 0;
                break;
            case 5: // Main Menu
                m_State = GameState::MainMenu;
                m_MenuSelection = 0;
                break;
        }
    }, [&]() {
        // On Cancel (ESC)
        m_State = GameState::Playing;
    }, hovered);
}

void PixelsGateGame::RenderOptions() {
    SDL_Renderer* renderer = GetRenderer();
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);
    
    int w, h; SDL_GetWindowSize(m_Window, &w, &h);
    m_TextRenderer->RenderTextCentered("OPTIONS", w/2, 50, {255, 255, 255, 255});
    m_TextRenderer->RenderTextCentered("No options available yet.", w/2, h/2, {150, 150, 150, 255});
    m_TextRenderer->RenderTextCentered("Press ESC to Back", w/2, h - 50, {100, 100, 100, 255});
    
    static bool wasEsc = false;
    bool isEsc = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_ESCAPE);
    if (isEsc && !wasEsc) {
        // Go back to previous state? simplified: go to MainMenu if valid, else Paused?
        // Actually we don't track previous state easily here. 
        // Let's assume if we came from Pause we want to go back to Pause.
        // A simple hack: We can check if m_Player is set up? No.
        // Let's just default to MainMenu for now, or add a 'Back' button.
        m_State = GameState::MainMenu; 
    }
    wasEsc = isEsc;
}

void PixelsGateGame::RenderCredits() {
    SDL_Renderer* renderer = GetRenderer();
    SDL_SetRenderDrawColor(renderer, 10, 10, 30, 255);
    SDL_RenderClear(renderer);

    int w, h; SDL_GetWindowSize(m_Window, &w, &h);
    m_TextRenderer->RenderTextCentered("CREDITS", w/2, 50, {255, 255, 255, 255});
    
    m_TextRenderer->RenderTextCentered("Created by Jesse Wood", w/2, 200, {200, 200, 255, 255});
    m_TextRenderer->RenderTextCentered("Engine: PixelsEngine (Custom)", w/2, 240, {200, 200, 255, 255});
    m_TextRenderer->RenderTextCentered("Assets: scrabling.itch.io/pixel-isometric-tiles", w/2, 280, {200, 200, 255, 255});
    
    m_TextRenderer->RenderTextCentered("Press ESC to Back", w/2, h - 50, {100, 100, 100, 255});

    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_ESCAPE)) m_State = GameState::MainMenu;
}

void PixelsGateGame::RenderControls() {
    SDL_Renderer* renderer = GetRenderer();
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);
    
    int w, h; SDL_GetWindowSize(m_Window, &w, &h);
    m_TextRenderer->RenderTextCentered("CONTROLS", w/2, 50, {255, 255, 255, 255});
    
    int y = 150;
    m_TextRenderer->RenderTextCentered("W/A/S/D - Move", w/2, y, {200, 200, 200, 255}); y += 40;
    m_TextRenderer->RenderTextCentered("Left Click - Move / Interact", w/2, y, {200, 200, 200, 255}); y += 40;
    m_TextRenderer->RenderTextCentered("Right Click - Context Menu", w/2, y, {200, 200, 200, 255}); y += 40;
    m_TextRenderer->RenderTextCentered("F5 - Quick Save", w/2, y, {200, 200, 200, 255}); y += 40;
    m_TextRenderer->RenderTextCentered("F8 - Quick Load", w/2, y, {200, 200, 200, 255}); y += 40;
    m_TextRenderer->RenderTextCentered("ESC - Pause Menu", w/2, y, {200, 200, 200, 255}); y += 40;

    m_TextRenderer->RenderTextCentered("Press ESC to Back", w/2, h - 50, {100, 100, 100, 255});

    static bool wasEsc = false;
    bool isEsc = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_ESCAPE);
    if (isEsc && !wasEsc) m_State = GameState::Paused;
    wasEsc = isEsc;
}

// --- New Features Implementations ---

void PixelsGateGame::UpdateAI(float deltaTime) {
    if (m_State == GameState::Combat) return;

    auto* pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    auto* pStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    if (!pTrans || !pStats || pStats->isDead) return;

    auto& view = GetRegistry().View<PixelsEngine::AIComponent>();
    for (auto& [entity, ai] : view) {
        auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
        auto* anim = GetRegistry().GetComponent<PixelsEngine::AnimationComponent>(entity);
        
        if (!transform) continue;

        float dist = std::sqrt(std::pow(pTrans->x - transform->x, 2) + std::pow(pTrans->y - transform->y, 2));

        bool canSee = false;
        if (ai.isAggressive && dist <= ai.sightRange) {
            float dx = pTrans->x - transform->x;
            float dy = pTrans->y - transform->y;
            float angleToPlayer = std::atan2(dy, dx) * (180.0f / M_PI); // Degrees

            // Normalize angles to -180 to 180
            float angleDiff = angleToPlayer - ai.facingDir;
            while (angleDiff > 180.0f) angleDiff -= 360.0f;
            while (angleDiff < -180.0f) angleDiff += 360.0f;

            if (std::abs(angleDiff) <= ai.coneAngle / 2.0f) {
                canSee = true;
            }
        }

        if (canSee) {
            // Chase
            if (dist > ai.attackRange) {
                float dx = pTrans->x - transform->x;
                float dy = pTrans->y - transform->y;
                // Normalize
                float len = std::sqrt(dx*dx + dy*dy);
                if (len > 0) { dx /= len; dy /= len; }
                
                float speed = 2.0f; // Slower than player
                transform->x += dx * speed * deltaTime;
                transform->y += dy * speed * deltaTime;
                
                // Update facing direction when moving
                ai.facingDir = std::atan2(dy, dx) * (180.0f / M_PI);

                if (anim) anim->Play("Run");
            } else {
                // Attack
                if (anim) anim->Play("Idle");
                ai.attackTimer -= deltaTime;
                if (ai.attackTimer <= 0.0f) {
                    // Deal Damage
                    pStats->currentHealth -= 2; // Reduced from 5 for balance
                    if (pStats->currentHealth < 0) pStats->currentHealth = 0;
                    SpawnFloatingText(pTrans->x, pTrans->y, "-2", {255, 0, 0, 255});
                    ai.attackTimer = ai.attackCooldown;
                }
            }
        } else {
            if (anim) anim->Play("Idle");
        }
    }
}

void PixelsGateGame::UpdateDayNight(float deltaTime) {
    m_TimeOfDay += deltaTime * m_TimeSpeed;
    if (m_TimeOfDay >= 24.0f) m_TimeOfDay -= 24.0f;
}

void PixelsGateGame::RenderDayNightCycle() {
    SDL_Renderer* renderer = GetRenderer();
    int w, h;
    SDL_GetWindowSize(m_Window, &w, &h);

    SDL_Color tint = {0, 0, 0, 0};
    
    // Interpolation helpers
    auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
    auto lerpColor = [&](SDL_Color c1, SDL_Color c2, float t) {
        return SDL_Color{
            (Uint8)lerp(c1.r, c2.r, t),
            (Uint8)lerp(c1.g, c2.g, t),
            (Uint8)lerp(c1.b, c2.b, t),
            (Uint8)lerp(c1.a, c2.a, t)
        };
    };

    // Define cycle stages
    if (m_TimeOfDay >= 5.0f && m_TimeOfDay < 8.0f) { // Sunrise to Early Day
        float t = (m_TimeOfDay - 5.0f) / 3.0f;
        tint = lerpColor({255, 200, 100, 40}, {20, 20, 40, 20}, t);
    } else if (m_TimeOfDay >= 8.0f && m_TimeOfDay < 17.0f) { // Day
        tint = {20, 20, 40, 20}; // Subtle ambient darkening during day
    } else if (m_TimeOfDay >= 17.0f && m_TimeOfDay < 20.0f) { // Late Day to Sunset
        float t = (m_TimeOfDay - 17.0f) / 3.0f;
        tint = lerpColor({20, 20, 40, 20}, {255, 100, 50, 60}, t);
    } else if (m_TimeOfDay >= 20.0f && m_TimeOfDay < 23.0f) { // Sunset to Night
        float t = (m_TimeOfDay - 20.0f) / 3.0f;
        tint = lerpColor({255, 100, 50, 60}, {10, 10, 50, 110}, t);
    } else if (m_TimeOfDay >= 23.0f || m_TimeOfDay < 4.0f) { // Deep Night
        tint = {10, 10, 50, 110}; // Reduced from 180 for better visibility
    } else { // Night to Sunrise (4.0 to 5.0)
        float t = (m_TimeOfDay - 4.0f) / 1.0f;
        tint = lerpColor({10, 10, 50, 110}, {255, 200, 100, 40}, t);
    }

    if (tint.a > 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, tint.r, tint.g, tint.b, tint.a);
        SDL_Rect screen = {0, 0, w, h};
        SDL_RenderFillRect(renderer, &screen);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    
    // Render Clock UI
    m_TextRenderer->RenderText("Time: " + std::to_string((int)m_TimeOfDay) + ":00", w - 120, 10, {255, 255, 255, 255});
}

void PixelsGateGame::SpawnFloatingText(float x, float y, const std::string& text, SDL_Color color) {
    m_FloatingTexts.push_back({x, y, text, 1.5f, color});
}

void PixelsGateGame::SpawnLootBag(float x, float y, const std::vector<PixelsEngine::Item>& items) {
    auto bag = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(bag, PixelsEngine::TransformComponent{ x, y });
    auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/gold_orb.png"); // Reuse orb sprite for now
    GetRegistry().AddComponent(bag, PixelsEngine::SpriteComponent{ tex, {0, 0, 32, 32}, 16, 16 });
    GetRegistry().AddComponent(bag, PixelsEngine::InteractionComponent{ "Loot", false, 0.0f });
    
    PixelsEngine::LootComponent loot;
    loot.drops = items;
    GetRegistry().AddComponent(bag, loot);
}

void PixelsGateGame::HandleInventoryInput() {
    int winW, winH; SDL_GetWindowSize(m_Window, &winW, &winH);
    int w = 400, h = 500; 
    SDL_Rect panel = { (winW - w) / 2, (winH - h) / 2, w, h };
    
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
    auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if (!inv) return;

    static bool wasDown = false;
    bool isDown = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT);
    bool pressed = isDown && !wasDown;
    bool released = !isDown && wasDown;
    wasDown = isDown;

    float currentTime = SDL_GetTicks() / 1000.0f;

    if (pressed) {
        // Check Close Button
        SDL_Rect closeBtn = { panel.x + w - 30, panel.y + 10, 20, 20 };
        if (mx >= closeBtn.x && mx <= closeBtn.x + closeBtn.w && my >= closeBtn.y && my <= closeBtn.y + closeBtn.h) {
            inv->isOpen = false;
            m_DraggingItemIndex = -1;
            return;
        }

        // Check for double click or start drag
        int listY = panel.y + 150;
        int itemY = listY + 10;
        for (int i = 0; i < inv->items.size(); ++i) {
            SDL_Rect rowRect = { panel.x + 20, itemY, w - 40, 40 };
            if (mx >= rowRect.x && mx <= rowRect.x + rowRect.w && my >= rowRect.y && my <= rowRect.y + rowRect.h) {
                // Potential Double Click
                if (i == m_LastClickedItemIndex && (currentTime - m_LastClickTime) < 0.5f) {
                    // EQUIP via Double Click
                    PixelsEngine::Item& item = inv->items[i];
                    bool equipped = false;
                    if (item.type == PixelsEngine::ItemType::WeaponMelee) {
                        if (!inv->equippedMelee.IsEmpty()) inv->AddItemObject(inv->equippedMelee);
                        inv->equippedMelee = item; inv->equippedMelee.quantity = 1; equipped = true;
                    } else if (item.type == PixelsEngine::ItemType::WeaponRanged) {
                        if (!inv->equippedRanged.IsEmpty()) inv->AddItemObject(inv->equippedRanged);
                        inv->equippedRanged = item; inv->equippedRanged.quantity = 1; equipped = true;
                    } else if (item.type == PixelsEngine::ItemType::Armor) {
                        if (!inv->equippedArmor.IsEmpty()) inv->AddItemObject(inv->equippedArmor);
                        inv->equippedArmor = item; inv->equippedArmor.quantity = 1; equipped = true;
                    }
                    if (equipped) {
                        item.quantity--;
                        if (item.quantity <= 0) inv->items.erase(inv->items.begin() + i);
                        m_LastClickedItemIndex = -1;
                        return;
                    }
                }
                m_LastClickTime = currentTime;
                m_LastClickedItemIndex = i;
                m_DraggingItemIndex = i;
            }
            itemY += 45;
        }

        // Check Unequip
        int startX = panel.x + 50;
        int startY = panel.y + 60;
        int slotSize = 40;
        auto CheckUnequip = [&](PixelsEngine::Item& slotItem, int x, int y) {
            SDL_Rect r = {x, y, slotSize, slotSize};
            if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) {
                if (!slotItem.IsEmpty()) { inv->AddItemObject(slotItem); slotItem = {"", "", 0, PixelsEngine::ItemType::Misc, 0}; }
            }
        };
        CheckUnequip(inv->equippedMelee, startX, startY);
        CheckUnequip(inv->equippedRanged, startX + 100, startY);
        CheckUnequip(inv->equippedArmor, startX + 200, startY);
    }

    if (released && m_DraggingItemIndex != -1) {
        // Handle Drop
        int startX = panel.x + 50;
        int startY = panel.y + 60;
        int slotSize = 40;
        
        PixelsEngine::Item& dragItem = inv->items[m_DraggingItemIndex];
        bool dropped = false;

        auto CheckDrop = [&](PixelsEngine::Item& slotItem, int x, int y, PixelsEngine::ItemType allowed) {
            SDL_Rect r = {x, y, slotSize, slotSize};
            if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h && dragItem.type == allowed) {
                if (!slotItem.IsEmpty()) inv->AddItemObject(slotItem);
                slotItem = dragItem; slotItem.quantity = 1;
                dropped = true;
            }
        };

        CheckDrop(inv->equippedMelee, startX, startY, PixelsEngine::ItemType::WeaponMelee);
        CheckDrop(inv->equippedRanged, startX + 100, startY, PixelsEngine::ItemType::WeaponRanged);
        CheckDrop(inv->equippedArmor, startX + 200, startY, PixelsEngine::ItemType::Armor);

        if (dropped) {
            dragItem.quantity--;
            if (dragItem.quantity <= 0) inv->items.erase(inv->items.begin() + m_DraggingItemIndex);
        }
        m_DraggingItemIndex = -1;
    }
}

void PixelsGateGame::StartCombat(PixelsEngine::Entity enemy) {
    if (m_State == GameState::Combat) return;

    m_TurnOrder.clear();
    
    // Add Player
    auto* pStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    int pInit = PixelsEngine::Dice::Roll(20) + (pStats ? pStats->GetModifier(pStats->dexterity) : 0);
    m_TurnOrder.push_back({m_Player, pInit, true});
    SpawnFloatingText(20, 20, "Initiative!", {0, 255, 255, 255});

    // Add Target Enemy
    if (GetRegistry().Valid(enemy)) {
        auto* eStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(enemy);
        int eInit = PixelsEngine::Dice::Roll(20) + (eStats ? eStats->GetModifier(eStats->dexterity) : 0);
        m_TurnOrder.push_back({enemy, eInit, false});
    }

    // Find nearby enemies
    auto* pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    auto& view = GetRegistry().View<PixelsEngine::AIComponent>();
    for (auto& [ent, ai] : view) {
        if (ent == enemy) continue; // Already added
        auto* t = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(ent);
        if (t && pTrans) {
            float dist = std::sqrt(std::pow(t->x - pTrans->x, 2) + std::pow(t->y - pTrans->y, 2));
            if (dist < 15.0f) { // Combat radius
                 auto* eStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(ent);
                 if (eStats && !eStats->isDead) {
                     int eInit = PixelsEngine::Dice::Roll(20) + eStats->GetModifier(eStats->dexterity);
                     m_TurnOrder.push_back({ent, eInit, false});
                 }
            }
        }
    }

    // Sort
    std::sort(m_TurnOrder.begin(), m_TurnOrder.end(), [](const CombatTurn& a, const CombatTurn& b) {
        return a.initiative > b.initiative; // Descending
    });

    m_State = GameState::Combat;
    m_CurrentTurnIndex = -1;
    NextTurn();
}

void PixelsGateGame::EndCombat() {
    m_State = GameState::Playing;
    m_TurnOrder.clear();
    SpawnFloatingText(0, 0, "Combat Ended", {0, 255, 0, 255});
}

void PixelsGateGame::NextTurn() {
    // Check if combat is over
    bool enemiesAlive = false;
    for (const auto& turn : m_TurnOrder) {
        if (!turn.isPlayer) {
             auto* stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(turn.entity);
             if (stats && !stats->isDead) enemiesAlive = true;
        }
    }
    
    auto* pStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    if (!pStats || pStats->isDead) {
        EndCombat();
        return;
    }

    if (!enemiesAlive) {
        EndCombat();
        return;
    }

    m_CurrentTurnIndex++;
    if (m_CurrentTurnIndex >= m_TurnOrder.size()) m_CurrentTurnIndex = 0;

    auto& current = m_TurnOrder[m_CurrentTurnIndex];
    auto* cStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(current.entity);
    
    // Skip dead
    if (!GetRegistry().HasComponent<PixelsEngine::StatsComponent>(current.entity) || (cStats && cStats->isDead)) {
        // Infinite recursion protection needed ideally, but assumes at least player alive
        NextTurn();
        return;
    }

    // Restore Resources
    m_ActionsLeft = 1;
    m_BonusActionsLeft = 1;
    m_MovementLeft = 5.0f; // Default
    if (current.isPlayer) {
        auto* pc = GetRegistry().GetComponent<PixelsEngine::PlayerComponent>(m_Player);
        if (pc) m_MovementLeft = pc->speed; 
        SpawnFloatingText(0, 0, "YOUR TURN", {0, 255, 0, 255});
    } else {
        m_CombatTurnTimer = 1.0f; // AI thinking time
        auto* ai = GetRegistry().GetComponent<PixelsEngine::AIComponent>(current.entity);
        if (ai) m_MovementLeft = ai->sightRange; 
    }
}

void PixelsGateGame::UpdateCombat(float deltaTime) {
    if (m_CurrentTurnIndex < 0 || m_CurrentTurnIndex >= m_TurnOrder.size()) return;

    auto& turn = m_TurnOrder[m_CurrentTurnIndex];
    
    if (turn.isPlayer) {
        HandleCombatInput();
    } else {
        // AI Logic
        m_CombatTurnTimer -= deltaTime;
        if (m_CombatTurnTimer <= 0.0f) {
            auto* aiTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(turn.entity);
            auto* pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
            auto* aiStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(turn.entity);
            auto* aiComp = GetRegistry().GetComponent<PixelsEngine::AIComponent>(turn.entity);
            
            if (aiTrans && pTrans && aiStats && !aiStats->isDead && aiComp) {
                 float dist = std::sqrt(std::pow(aiTrans->x - pTrans->x, 2) + std::pow(aiTrans->y - pTrans->y, 2));
                 
                 // Move
                 if (dist > aiComp->attackRange && m_MovementLeft > 0) {
                     float dx = pTrans->x - aiTrans->x;
                     float dy = pTrans->y - aiTrans->y;
                     float len = std::sqrt(dx*dx + dy*dy);
                     if (len > 0) { dx/=len; dy/=len; }
                     
                     float moveDist = 1.0f; 
                     if (m_MovementLeft >= moveDist) {
                         aiTrans->x += dx * moveDist;
                         aiTrans->y += dy * moveDist;
                         m_MovementLeft -= moveDist;
                     }
                 }
                 
                 // Attack
                 dist = std::sqrt(std::pow(aiTrans->x - pTrans->x, 2) + std::pow(aiTrans->y - pTrans->y, 2));
                 if (dist <= aiComp->attackRange && m_ActionsLeft > 0) {
                     auto* pTargetStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
                     auto* pInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
                     if (pTargetStats && pInv) {
                         int ac = 10 + pTargetStats->GetModifier(pTargetStats->dexterity) + pInv->equippedArmor.statBonus;
                         int roll = PixelsEngine::Dice::Roll(20);
                         
                         if (roll == 20 || (roll != 1 && roll + aiStats->GetModifier(aiStats->strength) >= ac)) {
                             // Hit!
                             int dmg = aiStats->damage; 
                             if (roll == 20) dmg *= 2;
                             
                             pTargetStats->currentHealth -= dmg;
                             if (pTargetStats->currentHealth < 0) pTargetStats->currentHealth = 0;
                             
                             std::string txt = std::to_string(dmg);
                             if (roll == 20) txt += "!";
                             SpawnFloatingText(pTrans->x, pTrans->y, txt, {255, 0, 0, 255});
                         } else {
                             // Miss
                             SpawnFloatingText(pTrans->x, pTrans->y, "Miss", {200, 200, 200, 255});
                         }
                         
                         m_ActionsLeft--;
                     }
                 }
            }
            NextTurn();
        }
    }
}

void PixelsGateGame::HandleCombatInput() {
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_SPACE)) {
        NextTurn();
        return;
    }

    float dx = 0, dy = 0;
    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_W)) dy -= 1;
    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_S)) dy += 1;
    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A)) dx -= 1;
    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D)) dx += 1;

    if ((dx != 0 || dy != 0) && m_MovementLeft > 0) {
        float speed = 5.0f; 
        float moveCost = speed * 0.016f; 
        if (m_MovementLeft >= moveCost) {
             auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
             auto* anim = GetRegistry().GetComponent<PixelsEngine::AnimationComponent>(m_Player);
             if (transform) {
                 transform->x += dx * moveCost;
                 transform->y += dy * moveCost;
                 m_MovementLeft -= moveCost;
                 if (anim) {
                     if (dy < 0) anim->Play("WalkUp");
                     else if (dy > 0) anim->Play("WalkDown");
                     else if (dx < 0) anim->Play("WalkLeft");
                     else anim->Play("WalkRight");
                 }
             }
        }
    }
    
    static bool wasDownLeft = false;
    bool isDownLeftRaw = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT);
    bool isPressedLeft = isDownLeftRaw && !wasDownLeft;
    bool isCtrlDown = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_LCTRL) || PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RCTRL);
    wasDownLeft = isDownLeftRaw;

    if (isPressedLeft) {
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        int winW, winH; SDL_GetWindowSize(m_Window, &winW, &winH);
        
        if (my > winH - 60) {
            CheckUIInteraction(mx, my);
        } else {
            if (isCtrlDown) {
                auto& camera = GetCamera(); auto& transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
                bool found = false;
                for (auto& [entity, transform] : transforms) {
                    if (entity == m_Player) continue;
                    auto* sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
                    if (sprite) {
                        int screenX, screenY; m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
                        screenX -= (int)camera.x; screenY -= (int)camera.y;
                        SDL_Rect drawRect = { screenX + 16 - sprite->pivotX, screenY + 16 - sprite->pivotY, sprite->srcRect.w, sprite->srcRect.h };
                        if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w && my >= drawRect.y && my <= drawRect.y + drawRect.h) {
                            PerformAttack(entity);
                            found = true; break;
                        }
                    }
                }
                if (!found) CheckWorldInteraction(mx, my);
            } else {
                CheckWorldInteraction(mx, my);
            }
        }
    }
}

void PixelsGateGame::RenderCombatUI() {
    SDL_Renderer* renderer = GetRenderer();
    int w, h; SDL_GetWindowSize(m_Window, &w, &h);

    // Turn Order Bar
    int count = m_TurnOrder.size();
    int slotW = 50;
    int startX = (w - (count * slotW)) / 2;
    int startY = 10;

    for (int i = 0; i < count; ++i) {
        SDL_Rect slot = {startX + i * slotW, startY, slotW - 5, 30};
        
        SDL_Color bg = {100, 100, 100, 255};
        if (i == m_CurrentTurnIndex) bg = {0, 200, 0, 255}; // Highlight current
        else if (m_TurnOrder[i].isPlayer) bg = {50, 50, 200, 255}; // Player blue
        
        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderFillRect(renderer, &slot);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &slot);
        
        auto* entStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_TurnOrder[i].entity);
        std::string label = "HP: ??%";
        if (entStats) {
            int hpPercent = (int)((float)entStats->currentHealth / (float)entStats->maxHealth * 100.0f);
            label = std::to_string(hpPercent) + "%";
        }
        m_TextRenderer->RenderTextCentered(label, slot.x + slotW/2, slot.y + 15, {255, 255, 255, 255});
    }

    // Resources UI
    std::string resStr = "Actions: " + std::to_string(m_ActionsLeft) + 
                         " | Bonus: " + std::to_string(m_BonusActionsLeft) + 
                         " | Move: " + std::to_string((int)m_MovementLeft);
    
    m_TextRenderer->RenderTextCentered(resStr, w/2, h - 80, {255, 255, 0, 255});
    
    if (m_TurnOrder[m_CurrentTurnIndex].isPlayer) {
        m_TextRenderer->RenderTextCentered("Press SPACE to End Turn", w/2, h - 100, {200, 200, 200, 255});
    } else {
        m_TextRenderer->RenderTextCentered("Enemy Turn...", w/2, h - 100, {255, 100, 100, 255});
    }
}void PixelsGateGame::RenderEnemyCones(const PixelsEngine::Camera& camera) {
    if (!PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_LSHIFT)) return;

    SDL_Renderer* renderer = GetRenderer();
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    int winW, winH;
    SDL_GetWindowSize(m_Window, &winW, &winH);

    auto& view = GetRegistry().View<PixelsEngine::AIComponent>();
    for (auto& [entity, ai] : view) {
        auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
        if (!transform) continue;

        // Center of the entity (assuming 32x32 tiles and entity center is roughly center of tile)
        // Transform->x/y are in tile coordinates
        float cx = transform->x + 0.5f; 
        float cy = transform->y + 0.5f;

        // Calculate screen coordinates matching the sprite rendering logic
        // screenX = (transform->x - camera.x) * 32 + winW/2 - 16;
        // The -16 centers the 32x32 sprite. 
        // For the cone origin, we want the center of the sprite.
        // centerScreenX = ((transform->x - camera.x) * 32) + winW/2 - 16 + 16 = ((transform->x - camera.x) * 32) + winW/2
        // Wait, transform->x is float tile coords.
        // Let's use the exact center calculation.
        
        float screenX = (transform->x - camera.x) * 32.0f + winW / 2.0f;
        float screenY = (transform->y - camera.y) * 32.0f + winH / 2.0f;

        float radDir = ai.facingDir * (M_PI / 180.0f);
        float radHalfCone = (ai.coneAngle / 2.0f) * (M_PI / 180.0f);
        float rangePixels = ai.sightRange * 32.0f;

        // We can approximate the cone with a triangle fan for better look, but a simple triangle is a start.
        // Let's do a fan of 3 triangles for a smoother arc.
        
        int segments = 5;
        float angleStep = (ai.coneAngle * (M_PI / 180.0f)) / segments;
        float currentAngle = radDir - radHalfCone;

        std::vector<SDL_Vertex> vertices;
        // Center vertex
        SDL_Vertex center = { {screenX, screenY}, {255, 0, 0, 100}, {0, 0} };

        for (int i = 0; i <= segments; ++i) {
            float x = screenX + std::cos(currentAngle) * rangePixels;
            float y = screenY + std::sin(currentAngle) * rangePixels;
            
            vertices.push_back({ {x, y}, {255, 0, 0, 100}, {0, 0} });
            currentAngle += angleStep;
        }

        // Draw triangles
        for (size_t i = 0; i < vertices.size() - 1; ++i) {
            SDL_Vertex tri[3];
            tri[0] = center;
            tri[1] = vertices[i];
            tri[2] = vertices[i+1];
            SDL_RenderGeometry(renderer, NULL, tri, 3, NULL, 0);
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
