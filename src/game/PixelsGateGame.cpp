#include "PixelsGateGame.h"
#include "../engine/Input.h"
#include "../engine/Components.h"
#include "../engine/AnimationSystem.h"
#include "../engine/TextureManager.h"
#include "../engine/Pathfinding.h"
#include "../engine/Inventory.h"
#include "../engine/Dice.h"
#include <SDL2/SDL_ttf.h>
#include <algorithm>

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
    inv.AddItem("Potion", 3);
    
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
    CreateBoar(18.0f, 18.0f);
    CreateBoar(22.0f, 22.0f);
    CreateBoar(15.0f, 25.0f);

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

void PixelsGateGame::CreateBoar(float x, float y) {
    auto boar = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(boar, PixelsEngine::TransformComponent{ x, y });
    GetRegistry().AddComponent(boar, PixelsEngine::StatsComponent{30, 30, 5, false});
    GetRegistry().AddComponent(boar, PixelsEngine::InteractionComponent{ "Boar", false, 0.0f });
    auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/critters/boar/boar_SE_run_strip.png");
    GetRegistry().AddComponent(boar, PixelsEngine::SpriteComponent{ tex, {0, 0, 41, 25}, 20, 20 });
    auto& anim = GetRegistry().AddComponent(boar, PixelsEngine::AnimationComponent{});
    anim.AddAnimation("Idle", 0, 0, 41, 25, 1);
    anim.AddAnimation("Run", 0, 0, 41, 25, 4);
    anim.Play("Run");
}

void PixelsGateGame::OnUpdate(float deltaTime) {
    if (m_State == GameState::Creation) { HandleCreationInput(); return; }
    HandleInput();

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

void PixelsGateGame::OnRender() {
    if (m_State == GameState::Creation) { RenderCharacterCreation(); return; }
    auto& camera = GetCamera();
    if (m_Level) m_Level->Render(camera);

    struct Renderable { int y; PixelsEngine::Entity entity; };
    std::vector<Renderable> renderQueue;
        auto& sprites = GetRegistry().View<PixelsEngine::SpriteComponent>();
        for (auto& [entity, sprite] : sprites) {
            auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
            if (transform && m_Level) {
                // Check Fog of War
                // If entity is player, always render.
                // If NPC/Monster, only render if tile is Visible.
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
        }
    }
    RenderHUD(); RenderInventory(); RenderContextMenu(); RenderDiceRoll();
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
    SDL_Renderer* renderer = GetRenderer();
    int winW, winH; SDL_GetWindowSize(m_Window, &winW, &winH);
    int w = 300, h = 400; SDL_Rect panel = { (winW - w) / 2, (winH - h) / 2, w, h };
    SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 218, 165, 32, 255); SDL_RenderDrawRect(renderer, &panel);
    m_TextRenderer->RenderTextCentered("INVENTORY", panel.x + w/2, panel.y + 30, {255, 255, 255, 255});
    int y = panel.y + 60;
    for (const auto& item : inv->items) { m_TextRenderer->RenderText(item.name + " x" + std::to_string(item.quantity), panel.x + 30, y, {255, 255, 255, 255}); y += 30; }
}

void PixelsGateGame::PerformAttack() {
    auto* pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    auto* pStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    if (!pTrans || !pStats) return;
    std::cout << "Player Attacking!" << std::endl;
    auto& transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
    for (auto& [entity, transform] : transforms) {
        if (entity == m_Player) continue;
        auto* eStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(entity);
        if (!eStats || eStats->isDead) continue;
        float dist = std::sqrt(std::pow(transform.x - pTrans->x, 2) + std::pow(transform.y - pTrans->y, 2));
        if (dist < 1.5f) {
            eStats->currentHealth -= pStats->damage;
            if (eStats->currentHealth <= 0) { eStats->currentHealth = 0; eStats->isDead = true; 
                auto* interact = GetRegistry().GetComponent<PixelsEngine::InteractionComponent>(entity);
                if (interact && interact->dialogueText == "Boar") {
                    auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
                    if (inv) inv->AddItem("Boar Meat", 1);
                }
                GetRegistry().RemoveComponent<PixelsEngine::SpriteComponent>(entity);
            }
        }
    }
}

void PixelsGateGame::HandleInput() {
    static bool wasDownLeft = false, wasDownRight = false;
    bool isCtrlDown = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_LCTRL) || PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RCTRL);
    bool isDownLeftRaw = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT);
    bool isDownRightRaw = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_RIGHT);
    bool isDownLeft = isDownLeftRaw && !isCtrlDown;
    bool isDownRight = isDownRightRaw || (isDownLeftRaw && isCtrlDown);
    bool isPressedLeft = isDownLeft && !wasDownLeft;
    bool isPressedRight = isDownRight && !wasDownRight;
    wasDownLeft = isDownLeft; wasDownRight = isDownRight;
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);

    if (m_DiceRoll.active) {
        if (!m_DiceRoll.resultShown) {
            m_DiceRoll.timer += 0.016f;
            if (m_DiceRoll.timer > m_DiceRoll.duration) { m_DiceRoll.resultShown = true; ResolveDiceRoll(); }
            if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_SPACE)) {
                auto* stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
                if (stats && stats->inspiration > 0) { stats->inspiration--; m_DiceRoll.finalValue = PixelsEngine::Dice::Roll(20); m_DiceRoll.success = (m_DiceRoll.finalValue + m_DiceRoll.modifier >= m_DiceRoll.dc) || (m_DiceRoll.finalValue == 20); m_DiceRoll.timer = 0.0f; }
            }
        } else if (isPressedLeft) m_DiceRoll.active = false;
        return;
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
                    else if (action.type == PixelsEngine::ContextActionType::Attack) PerformAttack();
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
        else CheckWorldInteraction(mx, my);
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
            else if (i == 5) { auto* stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player); if (stats) { stats->currentHealth = stats->maxHealth; stats->inspiration = 1; } }
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
    SDL_Renderer* renderer = GetRenderer(); int winW, winH; SDL_GetWindowSize(m_Window, &winW, &winH);
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