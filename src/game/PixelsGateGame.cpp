#include "PixelsGateGame.h"
#include "../engine/Input.h"
#include "../engine/Components.h"
#include "../engine/AnimationSystem.h"
#include "../engine/TextureManager.h"
#include "../engine/Pathfinding.h"
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
    m_Level = std::make_unique<PixelsEngine::Tilemap>(GetRenderer(), isoTileset, 32, 32, 20, 20);
    m_Level->SetProjection(PixelsEngine::Projection::Isometric);
    
    // Define some useful tile indices from the spritesheet
    // (You'll need to visually verify these indices match your specific spritesheet layout)
    const int GRASS_1 = 0;
    const int GRASS_2 = 1;
    const int DIRT = 2;
    const int WALL = 10; // Assuming tile 10 looks like a block/wall
    const int WATER = 11;

    // Fill map primarily with grass
    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 20; ++x) {
            // Default to grass
            int tile = GRASS_1;
            
            // Add some noise/variety
            if ((x * y + x) % 7 == 0) tile = GRASS_2;
            if ((x * y) % 13 == 0) tile = DIRT;

            // Create a wall border
            if (x == 0 || x == 19 || y == 0 || y == 19) tile = WALL;

            // Create some random obstacles
            if ((x * y) % 17 == 0 && x > 2 && y > 2) tile = WALL;

            m_Level->SetTile(x, y, tile); 
        }
    }

    // 2. Setup Player Entity
    m_Player = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(m_Player, PixelsEngine::TransformComponent{ 5.0f, 5.0f }); // Spawn inside the walls
    GetRegistry().AddComponent(m_Player, PixelsEngine::PlayerComponent{ 5.0f }); 
    GetRegistry().AddComponent(m_Player, PixelsEngine::PathMovementComponent{}); // Add Path Component
    
    // Load Player Texture
    std::string playerSheet = "assets/Pixel Art Top Down - Basic v1.2.2/Texture/TX Player.png";
    auto playerTexture = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), playerSheet);
    
    // Use the player sprite
    auto& sprite = GetRegistry().AddComponent(m_Player, PixelsEngine::SpriteComponent{ 
        playerTexture, 
        {0, 0, 32, 32}, 
        16, 30 // Pivot at feet
    });

    auto& anim = GetRegistry().AddComponent(m_Player, PixelsEngine::AnimationComponent{});
    // The asset only has content in the first 64 pixels (2 rows).
    // Row 0 (0-32): Likely Front/Side
    // Row 1 (32-64): Likely Back/Side
    
    anim.AddAnimation("Idle", 0, 0, 32, 32, 1);
    
    // Map Down and Right to Row 0
    anim.AddAnimation("WalkDown", 0, 0, 32, 32, 4);
    anim.AddAnimation("WalkRight", 0, 0, 32, 32, 4); 
    
    // Map Up and Left to Row 1
    anim.AddAnimation("WalkUp", 0, 32, 32, 32, 4);
    anim.AddAnimation("WalkLeft", 0, 32, 32, 32, 4);

    // 3. Setup NPC Entity
    auto npc = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(npc, PixelsEngine::TransformComponent{ 8.0f, 8.0f });
    // Reuse player texture for now (or another if available)
    GetRegistry().AddComponent(npc, PixelsEngine::SpriteComponent{ 
        playerTexture, 
        {0, 0, 32, 32}, 
        16, 30
    });
    GetRegistry().AddComponent(npc, PixelsEngine::InteractionComponent{ "Hello Traveler!", false, 0.0f });
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
                interaction->showDialogue = true;
                interaction->dialogueTimer = 3.0f; // Show for 3 seconds
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
}

void PixelsGateGame::HandleInput() {
    // Only process click ONCE per press (MouseDown vs Pressed)
    // For now we check "IsMouseButtonDown". Ideally we want "IsMouseButtonPressed" (frame 1)
    // But our Input class only exposes Down. Let's add a static "WasDown" check locally or just handle it.
    // Simpler: Just check on Down for movement (it's continuous pathfinding recalculation which is fine)
    // But for UI buttons, we want single click.
    
    // Quick "Pressed" check
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
            // Example action
            if (i == 0) { // Attack
                // Trigger attack anim?
            }
        }
    }
}

void PixelsGateGame::CheckWorldInteraction(int mx, int my) {
    auto& camera = GetCamera();
    int worldX = mx + camera.x;
    int worldY = my + camera.y;

    int gridX, gridY;
    m_Level->ScreenToGrid(worldX, worldY, gridX, gridY);

    // 1. Check if clicked on an Entity (NPC)
    // Iterate entities and see if their grid pos matches
    // Or closer approximation
    auto& transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
    bool clickedEntity = false;

    for (auto& [entity, transform] : transforms) {
        if (entity == m_Player) continue; // Don't click player

        // Simple distance check in Grid Coords (1.0 radius)
        float dist = std::sqrt(std::pow(transform.x - gridX, 2) + std::pow(transform.y - gridY, 2));
        if (dist < 1.0f) {
            // Clicked an entity!
            std::cout << "Clicked Entity " << entity << std::endl;
            m_SelectedNPC = entity;
            clickedEntity = true;
            
            // Set path to entity
            auto* pathComp = GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(m_Player);
            auto* pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
            
            // Path to neighbor of NPC
            auto path = PixelsEngine::Pathfinding::FindPath(*m_Level, (int)pTrans->x, (int)pTrans->y, (int)transform.x, (int)transform.y);
            // We want to stop BEFORE the NPC, but A* goes to the target.
            // Our Update loop checks distance < 1.5f and stops.
            
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

    // 2. If not entity, just walk there
    if (!clickedEntity) {
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
