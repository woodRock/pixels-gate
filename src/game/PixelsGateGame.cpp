#include "PixelsGateGame.h"
#include "../engine/Input.h"
#include "../engine/Components.h"
#include "../engine/AnimationSystem.h"
#include "../engine/TextureManager.h"
#include "../engine/Pathfinding.h"
#include "../engine/Inventory.h"
#include <SDL2/SDL_ttf.h>

PixelsGateGame::PixelsGateGame() 
    : PixelsEngine::Application("Pixels Gate", 800, 600) {
}

void PixelsGateGame::OnStart() {
    // Init Font
    m_TextRenderer = std::make_unique<PixelsEngine::TextRenderer>(GetRenderer(), "assets/font.ttf", 16);

    // 1. Setup Level with New Isometric Tileset
    // Tiles are 32x32.
    std::string isoTileset = "assets/isometric tileset/spritesheet.png";
    int mapW = 40;
    int mapH = 40;
    m_Level = std::make_unique<PixelsEngine::Tilemap>(GetRenderer(), isoTileset, 32, 32, mapW, mapH);
    m_Level->SetProjection(PixelsEngine::Projection::Isometric);
    
    // Define Tile Indices based on user feedback and standard isometric sheets
    // tile_061 is Stone/Wall.
    // tile_000 is Grass.
    // Let's assume standard layout.
    const int GRASS = 0;  
    const int GRASS_VAR = 1; // tile_001
    const int DIRT = 2;   // tile_002
    
    // tile_010 was "Stone" but user says it's Grass. Let's use it as a grass variant.
    const int GRASS_Dense = 10; 
    
    const int WATER = 28; // Keeping this for now
    const int SAND = 100; // Might be out of range if sheet is small?
    // User listed up to tile_114. So 100 is valid.
    
    const int STONE = 61; // User confirmed tile_061 is stone.

    // Generate Biomes using simple noise (Sin/Cos)
    for (int y = 0; y < mapH; ++y) {
        for (int x = 0; x < mapW; ++x) {
            float nx = x / (float)mapW;
            float ny = y / (float)mapH;
            
            // Elevation / Type Noise
            float noise = std::sin(nx * 10.0f) + std::cos(ny * 10.0f) + std::sin((nx + ny) * 5.0f);
            
            int tile = GRASS; // Default

            if (noise < -1.0f) {
                tile = WATER;
            } else if (noise < -0.6f) {
                tile = SAND;
            } else if (noise < 0.5f) {
                tile = GRASS;
                // Add vegitation variation
                if ((x * y) % 10 == 0) tile = GRASS_VAR;
                if ((x * y) % 17 == 0) tile = GRASS_Dense; 
            } else if (noise < 1.2f) {
                tile = DIRT;
            } else {
                tile = STONE; // Mountains/Walls
            }
            
            // Force borders to be Walls
            if (x == 0 || x == mapW-1 || y == 0 || y == mapH-1) tile = STONE;

            // Force Spawn Area (20,20) to be Grass
            if (x >= 18 && x <= 22 && y >= 18 && y <= 22) {
                tile = GRASS;
            }

            m_Level->SetTile(x, y, tile); 
        }
    }

    // 2. Setup Player Entity
    m_Player = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(m_Player, PixelsEngine::TransformComponent{ 20.0f, 20.0f }); // Center
    GetRegistry().AddComponent(m_Player, PixelsEngine::PlayerComponent{ 5.0f }); 
    GetRegistry().AddComponent(m_Player, PixelsEngine::PathMovementComponent{}); 
    GetRegistry().AddComponent(m_Player, PixelsEngine::StatsComponent{100, 100, 15, false}); 
    
    auto& inv = GetRegistry().AddComponent(m_Player, PixelsEngine::InventoryComponent{});
    inv.AddItem("Potion", 3);
    
    std::string playerSheet = "assets/Pixel Art Top Down - Basic v1.2.2/Texture/TX Player.png";
    auto playerTexture = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), playerSheet);
    
    auto& sprite = GetRegistry().AddComponent(m_Player, PixelsEngine::SpriteComponent{ 
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

    // 4. Quest NPC 1 (Fetch Gold Orb)
    auto npc1 = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(npc1, PixelsEngine::TransformComponent{ 21.0f, 21.0f });
    GetRegistry().AddComponent(npc1, PixelsEngine::SpriteComponent{ playerTexture, {0, 0, 32, 32}, 16, 30 });
    GetRegistry().AddComponent(npc1, PixelsEngine::InteractionComponent{ "Hello Hunter!", false, 0.0f });
    GetRegistry().AddComponent(npc1, PixelsEngine::StatsComponent{50, 50, 5, false}); 
    GetRegistry().AddComponent(npc1, PixelsEngine::QuestComponent{ "FetchOrb", 0, "Gold Orb" });

    // 5. Quest NPC 2 (Hunt Boars)
    auto npc2 = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(npc2, PixelsEngine::TransformComponent{ 18.0f, 22.0f });
    GetRegistry().AddComponent(npc2, PixelsEngine::SpriteComponent{ playerTexture, {0, 0, 32, 32}, 16, 30 });
    GetRegistry().AddComponent(npc2, PixelsEngine::InteractionComponent{ "Hello Warrior!", false, 0.0f });
    GetRegistry().AddComponent(npc2, PixelsEngine::StatsComponent{50, 50, 5, false}); 
    GetRegistry().AddComponent(npc2, PixelsEngine::QuestComponent{ "HuntBoars", 0, "Boar Meat" });

    // 6. Spawn Gold Orb (Item Pickup) at safe location
    auto orb = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(orb, PixelsEngine::TransformComponent{ 23.0f, 23.0f }); 
    // Ensure ground is safe
    m_Level->SetTile(23, 23, GRASS);

    auto orbTexture = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/gold_orb.png");
    GetRegistry().AddComponent(orb, PixelsEngine::SpriteComponent{ 
        orbTexture, 
        {0, 0, 32, 32}, 
        16, 16 
    });
    GetRegistry().AddComponent(orb, PixelsEngine::InteractionComponent{ "Gold Orb", false, 0.0f }); 
}

void PixelsGateGame::CreateBoar(float x, float y) {
    auto boar = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(boar, PixelsEngine::TransformComponent{ x, y });
    GetRegistry().AddComponent(boar, PixelsEngine::StatsComponent{30, 30, 5, false}); // 30 HP
    GetRegistry().AddComponent(boar, PixelsEngine::InteractionComponent{ "Boar", false, 0.0f }); // Name tag

    // Load Boar Textures (Strips)
    // We need a way to support multiple textures for one entity?
    // Or just one texture sheet.
    // The boar assets are split into multiple files (SE_run, NE_run, etc).
    // The current AnimationComponent/SpriteComponent assumes 1 texture.
    // We can pack them, OR update Animation to support changing textures.
    // EASIER: Pick ONE strip (SE_run) and just flip it for now to save time, 
    // or quickly use ImageMagick/Python to pack them?
    // Let's use "boar_SE_run_strip.png" as the default texture.
    
    auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/critters/boar/boar_SE_run_strip.png");
    
    // 164x25 -> 4 frames of 41x25.
    GetRegistry().AddComponent(boar, PixelsEngine::SpriteComponent{ 
        tex, {0, 0, 41, 25}, 20, 20 
    });

    auto& anim = GetRegistry().AddComponent(boar, PixelsEngine::AnimationComponent{});
    anim.AddAnimation("Idle", 0, 0, 41, 25, 1);
    anim.AddAnimation("Run", 0, 0, 41, 25, 4);
    
    // Start running
    anim.Play("Run");
}

void PixelsGateGame::OnUpdate(float deltaTime) {
    HandleInput();

    auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    auto* playerComp = GetRegistry().GetComponent<PixelsEngine::PlayerComponent>(m_Player);
    auto* anim = GetRegistry().GetComponent<PixelsEngine::AnimationComponent>(m_Player);
    auto* sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(m_Player);
    auto* pathComp = GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(m_Player);

    // Check interaction range if walking to NPC
    if (m_SelectedNPC != PixelsEngine::INVALID_ENTITY) {
        auto* npcTransform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_SelectedNPC);
        auto* interaction = GetRegistry().GetComponent<PixelsEngine::InteractionComponent>(m_SelectedNPC);
        
        if (npcTransform && interaction) {
            float dist = std::sqrt(std::pow(transform->x - npcTransform->x, 2) + std::pow(transform->y - npcTransform->y, 2));
            if (dist < 1.5f) { // Within 1.5 blocks
                // Stop and Interact
                pathComp->isMoving = false;
                anim->currentFrameIndex = 0; 
                
                // Check if this is an Item Pickup (Orb)
                if (interaction->dialogueText == "Gold Orb") {
                    auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
                    if (inv) {
                        inv->AddItem("Gold Orb", 1);
                        std::cout << "Picked up Gold Orb!" << std::endl;
                        GetRegistry().RemoveComponent<PixelsEngine::SpriteComponent>(m_SelectedNPC); // Remove from world
                        // Also disable collision/interaction? remove entity entirely?
                        // For now just visually remove.
                        // Ideally: GetRegistry().DestroyEntity(m_SelectedNPC);
                    }
                } 
                // Check if this is the Quest Giver NPC
                else {
                    auto* quest = GetRegistry().GetComponent<PixelsEngine::QuestComponent>(m_SelectedNPC);
                    if (quest) {
                        auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
                        
                        // Start Quest
                        if (quest->state == 0) {
                            if (quest->questId == "FetchOrb") {
                                interaction->dialogueText = "Please find my Gold Orb!";
                            } else if (quest->questId == "HuntBoars") {
                                interaction->dialogueText = "The boars are aggressive. Hunt them!";
                            }
                            quest->state = 1; 
                        } 
                        // Check Completion
                        else if (quest->state == 1) {
                            // Check if player has target item
                            bool hasItem = false;
                            for (auto& item : inv->items) {
                                if (item.name == quest->targetItem && item.quantity > 0) hasItem = true;
                            }
                            
                            if (hasItem) {
                                interaction->dialogueText = "You found it! Thank you.";
                                quest->state = 2; // Complete
                                inv->AddItem("Coins", 50);
                                // Optional: Remove quest item?
                                // for (auto& item : inv->items) if (item.name == quest->targetItem) item.quantity--;
                            } else {
                                if (quest->questId == "FetchOrb") interaction->dialogueText = "Bring me the Orb...";
                                else if (quest->questId == "HuntBoars") interaction->dialogueText = "Bring me Boar Meat.";
                            }
                        }
                        else if (quest->state == 2) {
                            interaction->dialogueText = "Blessings upon you.";
                        }
                    }
                    
                    interaction->showDialogue = true;
                    interaction->dialogueTimer = 3.0f;
                }

                m_SelectedNPC = PixelsEngine::INVALID_ENTITY; // Done
            }
        }
    }

    // Update Dialogue Timers
    auto& interactions = GetRegistry().View<PixelsEngine::InteractionComponent>();
    for (auto& [entity, interact] : interactions) {
        if (interact.showDialogue) {
            interact.dialogueTimer -= deltaTime;
            if (interact.dialogueTimer <= 0) {
                interact.showDialogue = false;
            }
        }
    }

    if (transform && playerComp && anim && sprite && pathComp) {
        
        // Input is now handled in HandleInput() calling CheckInteraction etc.
        // We just process the Movement state here.

        bool wasdInput = false; // We can set this in HandleInput if we wanted mixed control
        float dx = 0, dy = 0;
        std::string newAnim = "Idle";

        // Keep WASD for manual override
        if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_W)) { dx -= 1; dy -= 1; newAnim = "WalkUp"; wasdInput = true; }
        if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_S)) { dx += 1; dy += 1; newAnim = "WalkDown"; wasdInput = true; }
        if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A)) { dx -= 1; dy += 1; newAnim = "WalkLeft"; wasdInput = true; }
        if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D)) { dx += 1; dy -= 1; newAnim = "WalkRight"; wasdInput = true; }

        // Flip logic for manual input
        if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A)) sprite->flip = SDL_FLIP_HORIZONTAL;
        else if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D)) sprite->flip = SDL_FLIP_NONE;

        if (wasdInput) {
            // Cancel path movement if user takes manual control
            pathComp->isMoving = false;
            m_SelectedNPC = PixelsEngine::INVALID_ENTITY;
            
            // Manual collision logic
            float newX = transform->x + dx * playerComp->speed * deltaTime;
            float newY = transform->y + dy * playerComp->speed * deltaTime;

            if (m_Level->IsWalkable((int)newX, (int)newY)) {
                 transform->x = newX;
                 transform->y = newY;
            } else {
                if (m_Level->IsWalkable((int)newX, (int)transform->y)) transform->x = newX;
                else if (m_Level->IsWalkable((int)transform->x, (int)newY)) transform->y = newY;
            }
            
            anim->Play(newAnim);

        } else if (pathComp->isMoving) {
            // 3. Handle Path Movement
            float targetX = pathComp->targetX;
            float targetY = pathComp->targetY;
            
            float diffX = targetX - transform->x;
            float diffY = targetY - transform->y;
            float dist = std::sqrt(diffX*diffX + diffY*diffY);
            
            if (dist < 0.1f) {
                // Reached node, go to next
                transform->x = targetX;
                transform->y = targetY;
                pathComp->currentPathIndex++;
                if (pathComp->currentPathIndex >= pathComp->path.size()) {
                    pathComp->isMoving = false;
                    anim->currentFrameIndex = 0; // Stop anim
                } else {
                    pathComp->targetX = (float)pathComp->path[pathComp->currentPathIndex].first;
                    pathComp->targetY = (float)pathComp->path[pathComp->currentPathIndex].second;
                }
            } else {
                // Move towards target
                float moveX = (diffX / dist) * playerComp->speed * deltaTime;
                float moveY = (diffY / dist) * playerComp->speed * deltaTime;
                
                transform->x += moveX;
                transform->y += moveY;
                
                // Determine animation based on movement vector
                if (std::abs(diffX) > std::abs(diffY)) {
                    // Moving mostly X
                    // Wait, in grid coords:
                    // Up (-1, -1), Down (1, 1), Left (-1, 1), Right (1, -1)
                    
                    // Let's use the delta to guess direction
                    // If dx < 0 && dy < 0 -> Up
                    // If dx > 0 && dy > 0 -> Down
                    // If dx < 0 && dy > 0 -> Left
                    // If dx > 0 && dy < 0 -> Right
                    
                    if (diffX < 0 && diffY < 0) anim->Play("WalkUp");
                    else if (diffX > 0 && diffY > 0) anim->Play("WalkDown");
                    else if (diffX < 0 && diffY > 0) { anim->Play("WalkLeft"); sprite->flip = SDL_FLIP_HORIZONTAL; }
                    else if (diffX > 0 && diffY < 0) { anim->Play("WalkRight"); sprite->flip = SDL_FLIP_NONE; }
                } else {
                    // Fallback similar logic
                     if (diffX < 0 && diffY < 0) anim->Play("WalkUp");
                    else if (diffX > 0 && diffY > 0) anim->Play("WalkDown");
                    else if (diffX < 0 && diffY > 0) { anim->Play("WalkLeft"); sprite->flip = SDL_FLIP_HORIZONTAL; }
                    else if (diffX > 0 && diffY < 0) { anim->Play("WalkRight"); sprite->flip = SDL_FLIP_NONE; }
                }
            }
        } else {
            anim->currentFrameIndex = 0; 
        }

        // Animation Frame Update
        if (wasdInput || pathComp->isMoving) {
            anim->timer += deltaTime;
            auto& currentAnim = anim->animations[anim->currentAnimationIndex];
            if (anim->timer >= currentAnim.frameDuration) {
                anim->timer = 0.0f;
                anim->currentFrameIndex = (anim->currentFrameIndex + 1) % currentAnim.frames.size();
            }
            sprite->srcRect = currentAnim.frames[anim->currentFrameIndex];
        } else {
             auto& currentAnim = anim->animations[anim->currentAnimationIndex];
             sprite->srcRect = currentAnim.frames[0];
        }

        // Camera Follow
        int screenX, screenY;
        m_Level->GridToScreen(transform->x, transform->y, screenX, screenY);
        auto& camera = GetCamera();
        camera.x = screenX - camera.width / 2;
        camera.y = screenY - camera.height / 2;
    }
}




void PixelsGateGame::OnRender() {
    auto& camera = GetCamera();

    // 1. Render Level
    if (m_Level) {
        m_Level->Render(camera);
    }

    // 2. Render Entities with Z-Sorting
    struct Renderable {
        int y; // Sort key (screen Y)
        PixelsEngine::Entity entity;
    };
    std::vector<Renderable> renderQueue;

    auto& sprites = GetRegistry().View<PixelsEngine::SpriteComponent>();
    for (auto& [entity, sprite] : sprites) {
        auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
        if (transform && m_Level) {
            int screenX, screenY;
            m_Level->GridToScreen(transform->x, transform->y, screenX, screenY);
            renderQueue.push_back({ screenY, entity });
        }
    }

    // Sort by Y position (Painter's Algorithm)
    std::sort(renderQueue.begin(), renderQueue.end(), [](const Renderable& a, const Renderable& b) {
        return a.y < b.y;
    });

    // Draw sorted entities
    for (const auto& item : renderQueue) {
        auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(item.entity);
        auto* sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(item.entity);

        if (transform && sprite && sprite->texture) {
            int screenX, screenY;
            m_Level->GridToScreen(transform->x, transform->y, screenX, screenY);
            
            screenX -= (int)camera.x;
            screenY -= (int)camera.y;

            // Draw with Pivot offset and Flip
            sprite->texture->RenderRect(
                screenX + 16 - sprite->pivotX, 
                screenY + 16 - sprite->pivotY, 
                &sprite->srcRect, 
                -1, -1, 
                sprite->flip
            );
        }
    }

    // 3. Render HUD (Last, so it's on top)
    RenderHUD();
    RenderInventory();
}

void PixelsGateGame::RenderInventory() {
    auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if (!inv || !inv->isOpen) return;

    SDL_Renderer* renderer = GetRenderer();
    int winW, winH;
    SDL_GetWindowSize(m_Window, &winW, &winH);

    // Background
    int w = 300;
    int h = 400;
    SDL_Rect panel = { (winW - w) / 2, (winH - h) / 2, w, h };
    
    // Draw Panel (Wood color-ish)
    SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
    SDL_RenderFillRect(renderer, &panel);
    
    // Border
    SDL_SetRenderDrawColor(renderer, 218, 165, 32, 255); // Gold
    SDL_RenderDrawRect(renderer, &panel);
    
    // Title
    m_TextRenderer->RenderTextCentered("INVENTORY", panel.x + w/2, panel.y + 30, {255, 255, 255, 255});

    // List Items
    int y = panel.y + 60;
    for (const auto& item : inv->items) {
        std::string text = item.name + " x" + std::to_string(item.quantity);
        m_TextRenderer->RenderText(text, panel.x + 30, y, {255, 255, 255, 255});
        y += 30;
    }
}

void PixelsGateGame::PerformAttack() {
    auto* pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    auto* pStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);

    if (!pTrans || !pStats) return;

    std::cout << "Player Attacking!" << std::endl;

    // Check for enemies in range (1.5 blocks)
    auto& transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
    for (auto& [entity, transform] : transforms) {
        if (entity == m_Player) continue;

        auto* eStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(entity);
        if (!eStats || eStats->isDead) continue;

        float dist = std::sqrt(std::pow(transform.x - pTrans->x, 2) + std::pow(transform.y - pTrans->y, 2));
        if (dist < 1.5f) {
            // Hit!
            eStats->currentHealth -= pStats->damage;
            std::cout << "Hit Entity " << entity << " for " << pStats->damage << " damage! HP: " << eStats->currentHealth << std::endl;

            if (eStats->currentHealth <= 0) {
                eStats->currentHealth = 0;
                eStats->isDead = true;
                std::cout << "Entity " << entity << " Died!" << std::endl;
                
                // Loot Logic
                // If it's a boar (checked by name for now, simpler than checking texture)
                auto* interact = GetRegistry().GetComponent<PixelsEngine::InteractionComponent>(entity);
                if (interact && interact->dialogueText == "Boar") {
                    auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
                    if (inv) {
                        inv->AddItem("Boar Meat", 1);
                        std::cout << "Looted Boar Meat!" << std::endl;
                    }
                }

                GetRegistry().RemoveComponent<PixelsEngine::SpriteComponent>(entity);
            }
        }
    }
}
void PixelsGateGame::HandleInput() {
    static bool wasDown = false;
    bool isDown = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT);
    bool isPressed = isDown && !wasDown;
    wasDown = isDown;

    if (isPressed) {
        int mx, my;
        PixelsEngine::Input::GetMousePosition(mx, my);

        // Check UI First
        int winW, winH;
        SDL_GetWindowSize(m_Window, &winW, &winH);
        if (my > winH - 60) {
            CheckUIInteraction(mx, my);
        } else {
            CheckWorldInteraction(mx, my);
        }
    }
}

void PixelsGateGame::CheckUIInteraction(int mx, int my) {
    int winW, winH;
    SDL_GetWindowSize(m_Window, &winW, &winH);
    
    // Check buttons
    for (int i = 0; i < 6; ++i) {
        SDL_Rect btnRect = { 20 + (i * 55), winH - 50, 40, 40 };
        if (mx >= btnRect.x && mx <= btnRect.x + btnRect.w &&
            my >= btnRect.y && my <= btnRect.y + btnRect.h) {
            
            std::cout << "Clicked UI Button " << i << std::endl;
            if (i == 0) { // Attack
                PerformAttack();
            } else if (i == 2) { // Inventory
                auto* inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
                if (inv) {
                    inv->isOpen = !inv->isOpen;
                }
            }
        }
    }
}

void PixelsGateGame::CheckWorldInteraction(int mx, int my) {
    auto& camera = GetCamera();
    
    // 1. Check if clicked on an Entity (NPC) - Using Screen Space Bounds for better UX
    auto& transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
    bool clickedEntity = false;

    // We need to check entities in Z-order (roughly) to click the top-most one, 
    // but just checking all is fine for now.
    for (auto& [entity, transform] : transforms) {
        if (entity == m_Player) continue; 

        auto* sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
        if (sprite) {
            // Calculate Screen Position exactly as OnRender does
            int screenX, screenY;
            m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
            screenX -= (int)camera.x;
            screenY -= (int)camera.y;

            // Apply pivot to get the actual rendered rectangle
            SDL_Rect drawRect = {
                screenX + 16 - sprite->pivotX,
                screenY + 16 - sprite->pivotY,
                sprite->srcRect.w,
                sprite->srcRect.h
            };

            // Hit Test
            if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w &&
                my >= drawRect.y && my <= drawRect.y + drawRect.h) {
                
                std::cout << "Clicked Entity " << entity << " (Visual Hit)" << std::endl;
                m_SelectedNPC = entity;
                clickedEntity = true;
                
                // Path to entity
                auto* pathComp = GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(m_Player);
                auto* pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
                
                // Find path to the tile the NPC is standing on
                auto path = PixelsEngine::Pathfinding::FindPath(*m_Level, (int)pTrans->x, (int)pTrans->y, (int)transform.x, (int)transform.y);
                
                if (!path.empty()) {
                    pathComp->path = path;
                    pathComp->currentPathIndex = 0;
                    pathComp->isMoving = true;
                    pathComp->targetX = (float)path[0].first;
                    pathComp->targetY = (float)path[0].second;
                }
                break;
            }
        }
    }

    // 2. If not entity, just walk there (Ground Click)
    if (!clickedEntity) {
        int worldX = mx + camera.x;
        int worldY = my + camera.y;
        int gridX, gridY;
        m_Level->ScreenToGrid(worldX, worldY, gridX, gridY);

        m_SelectedNPC = PixelsEngine::INVALID_ENTITY; // Deselect
        auto* pathComp = GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(m_Player);
        auto* pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);

        auto path = PixelsEngine::Pathfinding::FindPath(*m_Level, (int)pTrans->x, (int)pTrans->y, gridX, gridY);
        if (!path.empty()) {
            pathComp->path = path;
            pathComp->currentPathIndex = 0;
            pathComp->isMoving = true;
            pathComp->targetX = (float)path[0].first;
            pathComp->targetY = (float)path[0].second;
        }
    }
}

void PixelsGateGame::RenderHUD() {
    // ... (Existing HUD code)
    SDL_Renderer* renderer = GetRenderer();
    int winW, winH;
    SDL_GetWindowSize(m_Window, &winW, &winH);
    
    // Bottom Bar (Action Bar)
    SDL_Rect hudRect = { 0, winH - 60, winW, 60 };
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); 
    SDL_RenderFillRect(renderer, &hudRect);
    
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); 
    SDL_RenderDrawRect(renderer, &hudRect);

    // Action Buttons
    const char* labels[] = { "Atk", "Mag", "Inv", "Map", "Chr", "Opt" };
    for (int i = 0; i < 6; ++i) {
        SDL_Rect btnRect = { 20 + (i * 55), winH - 50, 40, 40 };
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &btnRect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &btnRect);
        
        // Render Label
        m_TextRenderer->RenderTextCentered(labels[i], btnRect.x + 20, btnRect.y + 20, {255, 255, 255, 255});
    }

    // Render Dialogue Bubbles (World Space mapped to Screen)
    auto& camera = GetCamera();
    auto& interactions = GetRegistry().View<PixelsEngine::InteractionComponent>();
    for (auto& [entity, interact] : interactions) {
        if (interact.showDialogue) {
            auto* transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
            if (transform) {
                 int screenX, screenY;
                 m_Level->GridToScreen(transform->x, transform->y, screenX, screenY);
                 screenX -= (int)camera.x;
                 screenY -= (int)camera.y;

                 // Draw Bubble background
                 // Estimate width based on text length (approx 8px per char)
                 int w = interact.dialogueText.length() * 10; 
                 int h = 30;
                 SDL_Rect bubble = { screenX + 16 - w/2, screenY - 40, w, h };
                 
                 SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
                 SDL_RenderFillRect(renderer, &bubble);
                 SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                 SDL_RenderDrawRect(renderer, &bubble);

                 // Draw Text
                 m_TextRenderer->RenderTextCentered(interact.dialogueText, bubble.x + w/2, bubble.y + h/2, {0, 0, 0, 255});
            }
        }
    }
}
