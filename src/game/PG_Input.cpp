#include "PixelsGateGame.h"
#include "../engine/Input.h"
#include "../engine/Pathfinding.h"
#include "../engine/SaveSystem.h"
#include <filesystem>

// --- Helper Functions ---

void PixelsGateGame::HandleMenuNavigation(int numOptions, std::function<void(int)> onSelect, std::function<void()> onCancel, int forceSelection) {
    if (numOptions <= 0) {
        if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE) && onCancel) onCancel();
        return;
    }

    bool up = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_W) || PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_UP);
    bool down = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_S) || PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_DOWN);
    bool enter = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_RETURN) || PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_SPACE);
    bool esc = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE);
    bool click = PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT);

    if (forceSelection != -1) m_MenuSelection = forceSelection;
    
    if (up) m_MenuSelection = (m_MenuSelection - 1 + numOptions) % numOptions;
    if (down) m_MenuSelection = (m_MenuSelection + 1) % numOptions;
    
    // Check for click separately to ensure it registers even if forceSelection was just set
    if ((enter || (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT) && m_MenuSelection != -1)) && onSelect) {
        onSelect(m_MenuSelection);
    }
    
    if (esc && onCancel) onCancel();
}

void PixelsGateGame::CheckUIInteraction(int mx, int my) {
    int w = GetWindowWidth(); int h = GetWindowHeight();
    int barH = 100;
    
    // Check Action Grid (Left side)
    for(int i=0; i<6; ++i) {
        int row = i/3; int col = i%3;
        SDL_Rect btn = {20 + col*45, h - barH + 25 + row*35, 40, 30};
        if(mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) {
            if(i==0) PerformAttack();
            else if(i==1) { m_ReturnState = m_State; m_State = GameState::TargetingJump; }
            else if(i==2) { 
                auto *p = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
                if(p) {
                    p->isStealthed = !p->isStealthed;
                    SpawnFloatingText(0,0, p->isStealthed ? "Sneak" : "Visible", {150,150,150,255});
                }
            }
            else if(i==3) { m_ReturnState = m_State; m_State = GameState::TargetingShove; }
            else if(i==4) { m_ReturnState = m_State; m_State = GameState::TargetingDash; }
            else if(i==5) { if(m_State == GameState::Combat) NextTurn(); }
            return;
        }
    }

    // Check Items (Right side)
    for(int i=0; i<6; ++i) {
        int row = i/3; int col = i%3;
        SDL_Rect btn = {320 + col*45, h - barH + 25 + row*35, 40, 30};
        if(mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) {
            auto items = GetHotbarItems();
            if (i < items.size() && !items[i].empty()) {
                UseItem(items[i]);
            }
            return;
        }
    }
    
    // Check Spells (Middle)
    std::string spells[] = {"Fireball", "Heal", "Magic Missile", "Shield", "", ""};
    for(int i=0; i<6; ++i) {
        int row = i/3; int col = i%3;
        SDL_Rect btn = {170 + col*45, h - barH + 25 + row*35, 40, 30};
        if(mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) {
            if(!spells[i].empty()) {
                m_PendingSpellName = spells[i];
                if(m_PendingSpellName == "Shield") CastSpell("Shield", m_Player);
                else {
                    m_ReturnState = m_State;
                    m_State = GameState::Targeting;
                }
            }
            return;
        }
    }
    
    // Check System Buttons (Far Right)
    for (int i = 0; i < 4; ++i) {
        SDL_Rect btn = {w - 180 + (i % 2) * 85, h - 100 + 15 + (i / 2) * 40, 75, 35};
        if (mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) {
            if (i==0) { m_ReturnState = m_State; m_State = GameState::Map; m_MapTab = 0; }
            if (i==1) { m_ReturnState = m_State; m_State = GameState::CharacterMenu; m_CharacterTab = 1; }
            if (i==2) { m_ReturnState = m_State; m_State = GameState::RestMenu; }
            if (i==3) { m_ReturnState = m_State; m_State = GameState::CharacterMenu; m_CharacterTab = 0; }
            return;
        }
    }
}

void PixelsGateGame::CheckWorldInteraction(int mx, int my) {
    auto &camera = GetCamera();
    auto *currentMap = GetCurrentMap();
    if (!currentMap) return;

    int worldX = mx + camera.x, worldY = my + camera.y;
    int gridX, gridY;
    currentMap->ScreenToGrid(worldX, worldY, gridX, gridY);

    if (gridX < 0 || gridX >= currentMap->GetWidth() || gridY < 0 || gridY >= currentMap->GetHeight()) return;

    PixelsEngine::Entity clickedEnt = GetEntityAtMouse();

    // Double-click detection
    float currentTime = SDL_GetTicks() / 1000.0f;
    bool isDoubleClick = false;
    if (clickedEnt != PixelsEngine::INVALID_ENTITY) {
        isDoubleClick = (clickedEnt == (PixelsEngine::Entity)m_LastClickedItemIndex && (currentTime - m_LastClickTime) < 0.3f);
        m_LastClickTime = currentTime;
        m_LastClickedItemIndex = (int)clickedEnt;
    }

    if (clickedEnt != PixelsEngine::INVALID_ENTITY && clickedEnt != m_Player) {
        auto *stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(clickedEnt);
        bool isDead = (stats && stats->isDead);

        if (isDoubleClick) {
            auto *interact = GetRegistry().GetComponent<PixelsEngine::InteractionComponent>(clickedEnt);
            if (interact) {
                PickupItem(clickedEnt);
                return;
            }
        }

        auto *diag = GetRegistry().GetComponent<PixelsEngine::DialogueComponent>(clickedEnt);
        if (diag && m_State != GameState::Combat && !isDead) { // Prevent dialogue with dead NPCs or during combat
            m_DialogueWith = clickedEnt;
            m_ReturnState = m_State; 
            m_State = GameState::Dialogue;
            m_DialogueSelection = 0;
            diag->tree->currentNodeId = "start";
            return;
        }
        
        auto *loot = GetRegistry().GetComponent<PixelsEngine::LootComponent>(clickedEnt);
        auto *lock = GetRegistry().GetComponent<PixelsEngine::LockComponent>(clickedEnt);
        
        if (lock && lock->isLocked) {
            auto *pInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
            if (pInv) {
                bool hasKey = false;
                for (auto &item : pInv->items) {
                    if (item.name == lock->keyName) {
                        hasKey = true;
                        break;
                    }
                }
                if (hasKey) {
                    lock->isLocked = false;
                    SpawnFloatingText(0, 0, "Used " + lock->keyName, {0, 255, 0, 255});
                }
            }
        }

        bool isUnlocked = (lock == nullptr || !lock->isLocked);
        if (loot && (stats == nullptr || stats->isDead || isUnlocked)) {
             m_LootingEntity = clickedEnt;
             m_ReturnState = m_State; m_State = GameState::Looting;
             return;
        }
    }

    auto *pt = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    auto *pathComp = GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(m_Player);
    if (pt && pathComp) {
        if (gridX < 0 || gridX >= currentMap->GetWidth() || gridY < 0 || gridY >= currentMap->GetHeight()) return;
        auto p = PixelsEngine::Pathfinding::FindPath(*currentMap, (int)pt->x, (int)pt->y, gridX, gridY);
        if (!p.empty()) {
            pathComp->path = p;
            pathComp->currentPathIndex = 0;
            pathComp->isMoving = true;
            pathComp->targetX = (float)p[0].first;
            pathComp->targetY = (float)p[0].second;
        }
    }
}

// --- Main Input Handler ---

void PixelsGateGame::HandleInput() {
    auto escKey = PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Pause);
    auto invKey = PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Inventory);
    auto mapKey = PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Map);
    auto charKey = PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Character);
    auto spellKey = PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Magic);
    bool shift = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_LSHIFT) || PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RSHIFT);
    bool ctrl = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_LCTRL) || PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RCTRL);

    // Key Toggles
    if (PixelsEngine::Input::IsKeyPressed(escKey)) { m_ReturnState = m_State; m_State = GameState::Paused; m_MenuSelection = 0; return; }
    if (PixelsEngine::Input::IsKeyPressed(invKey)) { m_ReturnState = m_State; m_State = GameState::CharacterMenu; m_CharacterTab = 0; return; }
    if (PixelsEngine::Input::IsKeyPressed(charKey)) { m_ReturnState = m_State; m_State = GameState::CharacterMenu; m_CharacterTab = 1; return; }
    if (PixelsEngine::Input::IsKeyPressed(spellKey)) { m_ReturnState = m_State; m_State = GameState::CharacterMenu; m_CharacterTab = 2; return; }
    if (PixelsEngine::Input::IsKeyPressed(mapKey)) { m_ReturnState = m_State; m_State = GameState::Map; m_MapTab = 0; return; }
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_J)) { m_ReturnState = m_State; m_State = GameState::Map; m_MapTab = 1; return; }

    // Actions
    auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    
    // Weapon Toggle
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_F)) {
        m_SelectedWeaponSlot = (m_SelectedWeaponSlot == 0) ? 1 : 0;
        if(pTrans) SpawnFloatingText(pTrans->x, pTrans->y, (m_SelectedWeaponSlot == 0) ? "Melee" : "Ranged", {200, 200, 255, 255});
    }

    // Jump
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_Z)) {
        m_ReturnState = m_State;
        m_State = GameState::TargetingJump;
    }

    // Sneak
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_C)) {
        auto *p = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
        if(p) {
            if (m_State == GameState::Combat) {
                if (m_Combat.m_ActionsLeft > 0) {
                    p->isStealthed = true;
                    p->hasAdvantage = true;
                    m_Combat.m_ActionsLeft--;
                    if(pTrans) SpawnFloatingText(pTrans->x, pTrans->y, "Hidden!", {150, 150, 150, 255});
                } else if(pTrans) SpawnFloatingText(pTrans->x, pTrans->y, "No Actions!", {255, 0, 0, 255});
            } else {
                p->isStealthed = !p->isStealthed;
                if(pTrans) SpawnFloatingText(pTrans->x, pTrans->y, p->isStealthed ? "Sneaking..." : "Visible", {150, 150, 150, 255});
            }
        }
    }

    // Shove
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_V)) {
        m_ReturnState = m_State;
        m_State = GameState::TargetingShove;
    }

    // Dash
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_B)) {
        if (m_State == GameState::Combat) {
            if (m_Combat.m_ActionsLeft > 0) {
                m_Combat.m_MovementLeft += 5.0f; 
                m_Combat.m_ActionsLeft--;
                if(pTrans) SpawnFloatingText(pTrans->x, pTrans->y, "Dashed!", {255, 255, 0, 255});
            } else if(pTrans) SpawnFloatingText(pTrans->x, pTrans->y, "No Actions!", {255, 0, 0, 255});
        } else if(pTrans) {
            SpawnFloatingText(pTrans->x, pTrans->y, "Dash (Combat Only)", {200, 200, 200, 255});
        }
    }

    // Save/Load
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_F5)) {
        PixelsEngine::SaveSystem::SaveGame("quicksave.dat", GetRegistry(), m_Player, *m_Level, m_State == GameState::Camp, m_LastWorldPos.x, m_LastWorldPos.y);
        PixelsEngine::SaveSystem::SaveWorldFlags("quicksave.dat", m_WorldFlags);
        ShowSaveMessage();
    }
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_F8)) {
        PixelsEngine::SaveSystem::LoadWorldFlags("quicksave.dat", m_WorldFlags);
        TriggerLoadTransition("quicksave.dat");
    }

    // Mouse Interaction
    if (m_ContextMenu.isOpen) {
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
            SDL_Rect menuRect = {m_ContextMenu.x, m_ContextMenu.y, 150, (int)m_ContextMenu.actions.size() * 30 + 10};
            if (mx >= menuRect.x && mx <= menuRect.x + 150 && my >= menuRect.y && my <= menuRect.y + menuRect.h) {
                int index = (my - m_ContextMenu.y - 5) / 30;
                if (index >= 0 && index < m_ContextMenu.actions.size()) {
                    auto action = m_ContextMenu.actions[index];
                    if (action.type == PixelsEngine::ContextActionType::Attack) PerformAttack(m_ContextMenu.targetEntity);
                    else if (action.type == PixelsEngine::ContextActionType::Talk) {
                        m_DialogueWith = m_ContextMenu.targetEntity;
                        m_ReturnState = m_State; m_State = GameState::Dialogue;
                        m_DialogueSelection = 0;
                        auto *d = GetRegistry().GetComponent<PixelsEngine::DialogueComponent>(m_DialogueWith);
                        if (d) d->tree->currentNodeId = "start";
                    }
                    else if (action.type == PixelsEngine::ContextActionType::Pickpocket) {
                        StartDiceRoll(0, 15, "Sleight of Hand", m_ContextMenu.targetEntity, PixelsEngine::ContextActionType::Pickpocket);
                    }
                    else if (action.type == PixelsEngine::ContextActionType::Lockpick) {
                        auto *lock = GetRegistry().GetComponent<PixelsEngine::LockComponent>(m_ContextMenu.targetEntity);
                        if (lock) {
                            StartDiceRoll(0, lock->dc, "Lockpicking", m_ContextMenu.targetEntity, PixelsEngine::ContextActionType::Lockpick);
                        }
                    }
                }
            }
            m_ContextMenu.isOpen = false;
        } else if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_RIGHT)) {
            m_ContextMenu.isOpen = false;
        }
        return;
    }

    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
    // User reported clicking higher than position. Subtracting 4 previously likely made it worse.
    // We will add 8 pixels to Y to shift the internal 'hit' down to the visual cursor.
     

    if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
        if (my > GetWindowHeight() - 100) {
            CheckUIInteraction(mx, my);
        } else {
            // Ctrl + Click to open Context Menu
            if (ctrl) {
                m_ContextMenu.isOpen = true; m_ContextMenu.x = mx; m_ContextMenu.y = my;
                m_ContextMenu.targetEntity = GetEntityAtMouse();
                if (m_ContextMenu.targetEntity != PixelsEngine::INVALID_ENTITY) {
                    m_ContextMenu.actions.clear();
                    m_ContextMenu.actions.push_back({"Attack", PixelsEngine::ContextActionType::Attack});
                    if(GetRegistry().HasComponent<PixelsEngine::DialogueComponent>(m_ContextMenu.targetEntity)) 
                        m_ContextMenu.actions.push_back({"Talk", PixelsEngine::ContextActionType::Talk});
                    if(GetRegistry().HasComponent<PixelsEngine::InventoryComponent>(m_ContextMenu.targetEntity))
                        m_ContextMenu.actions.push_back({"Pickpocket", PixelsEngine::ContextActionType::Pickpocket});
                    
                    auto *lock = GetRegistry().GetComponent<PixelsEngine::LockComponent>(m_ContextMenu.targetEntity);
                    if (lock && lock->isLocked) {
                        m_ContextMenu.actions.push_back({"Lockpick", PixelsEngine::ContextActionType::Lockpick});
                    }
                } else {
                    m_ContextMenu.isOpen = false;
                }
                return;
            }

            // Shift + Click to Attack
            if (shift) {
                PixelsEngine::Entity ent = GetEntityAtMouse();
                if (ent != PixelsEngine::INVALID_ENTITY && ent != m_Player) {
                    PerformAttack(ent);
                    return; 
                }
            }
            CheckWorldInteraction(mx, my);
        }
    }
    if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_RIGHT)) {
        m_ContextMenu.isOpen = true; m_ContextMenu.x = mx; m_ContextMenu.y = my;
        m_ContextMenu.targetEntity = GetEntityAtMouse();
        if (m_ContextMenu.targetEntity != PixelsEngine::INVALID_ENTITY) {
            m_ContextMenu.actions.clear();
            m_ContextMenu.actions.push_back({"Attack", PixelsEngine::ContextActionType::Attack});
            if(GetRegistry().HasComponent<PixelsEngine::DialogueComponent>(m_ContextMenu.targetEntity)) 
                m_ContextMenu.actions.push_back({"Talk", PixelsEngine::ContextActionType::Talk});
            if(GetRegistry().HasComponent<PixelsEngine::InventoryComponent>(m_ContextMenu.targetEntity))
                m_ContextMenu.actions.push_back({"Pickpocket", PixelsEngine::ContextActionType::Pickpocket});
            
            auto *lock = GetRegistry().GetComponent<PixelsEngine::LockComponent>(m_ContextMenu.targetEntity);
            if (lock && lock->isLocked) {
                m_ContextMenu.actions.push_back({"Lockpick", PixelsEngine::ContextActionType::Lockpick});
            }
        } else {
            m_ContextMenu.isOpen = false;
        }
    }

    // WASD Movement
    if (m_State == GameState::Playing || m_State == GameState::Camp) {
        auto *pathComp = GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(m_Player);
        if (!pathComp->isMoving) {
            float dx=0, dy=0;
            if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_W)) dy=-1;
            if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_S)) dy=1;
            if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A)) dx=-1;
            if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D)) dx=1;
            
            if (dx != 0 || dy != 0) {
                auto *trans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
                auto *pc = GetRegistry().GetComponent<PixelsEngine::PlayerComponent>(m_Player);
                auto *currentMap = GetCurrentMap();
                float newX = trans->x + dx * pc->speed * 0.016f; 
                float newY = trans->y + dy * pc->speed * 0.016f;
                if (currentMap->IsWalkable((int)newX, (int)newY)) {
                    trans->x = newX; trans->y = newY;
                }
            }
        }
    }
}

// --- Menu Specific Handlers ---

void PixelsGateGame::HandleCreationInput() {
    static bool wasUp = false, wasDown = false, wasLeft = false, wasRight = false, wasEnter = false;
    bool isUp = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_W);
    bool isDown = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_S);
    bool isLeft = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A);
    bool isRight = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D);
    bool isEnter = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RETURN);

    if (isUp && !wasUp) m_CC_SelectionIndex = (m_CC_SelectionIndex - 1 + 9) % 9;
    if (isDown && !wasDown) m_CC_SelectionIndex = (m_CC_SelectionIndex + 1) % 9;
    
    if ((isLeft && !wasLeft) || (isRight && !wasRight)) {
        int delta = (isRight && !wasRight) ? 1 : -1;
        if (m_CC_SelectionIndex < 6) {
            if (delta > 0 && m_CC_PointsRemaining > 0 && m_CC_TempStats[m_CC_SelectionIndex] < 18) {
                m_CC_TempStats[m_CC_SelectionIndex]++; m_CC_PointsRemaining--;
            } else if (delta < 0 && m_CC_TempStats[m_CC_SelectionIndex] > 8) {
                m_CC_TempStats[m_CC_SelectionIndex]--; m_CC_PointsRemaining++;
            }
        } else if (m_CC_SelectionIndex == 6) m_CC_ClassIndex = (m_CC_ClassIndex + delta + 4) % 4;
        else if (m_CC_SelectionIndex == 7) m_CC_RaceIndex = (m_CC_RaceIndex + delta + 4) % 4;
    }

    if (isEnter && !wasEnter && m_CC_SelectionIndex == 8) {
        auto *stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
        if (stats) {
            stats->strength = m_CC_TempStats[0]; stats->dexterity = m_CC_TempStats[1];
            stats->constitution = m_CC_TempStats[2]; stats->intelligence = m_CC_TempStats[3];
            stats->wisdom = m_CC_TempStats[4]; stats->charisma = m_CC_TempStats[5];
            stats->characterClass = m_CC_Classes[m_CC_ClassIndex];
            stats->race = m_CC_Races[m_CC_RaceIndex];
            stats->maxHealth = 10 + stats->GetModifier(stats->constitution) * 2;
            stats->currentHealth = stats->maxHealth;
        }
        m_State = GameState::Playing;
    }
    wasUp = isUp; wasDown = isDown; wasLeft = isLeft; wasRight = isRight; wasEnter = isEnter;
}

void PixelsGateGame::ResetGame() {
    m_WorldFlags.clear();
    auto &entities = GetRegistry().View<PixelsEngine::TransformComponent>();
    std::vector<PixelsEngine::Entity> toDestroy;
    for (auto &[entity, trans] : entities) {
        toDestroy.push_back(entity);
    }
    for (auto ent : toDestroy) GetRegistry().DestroyEntity(ent);

    SpawnWorldEntities(); 

    auto *trans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    if(trans) { trans->x = 20.0f; trans->y = 20.0f; }
    
    // Snap Camera
    if (trans) {
        auto *currentMap = (m_State == GameState::Camp) ? m_CampLevel.get() : m_Level.get();
        if (currentMap) {
            int screenX, screenY;
            currentMap->GridToScreen(trans->x, trans->y, screenX, screenY);
            auto &cam = GetCamera();
            cam.x = screenX - cam.width / 2;
            cam.y = screenY - cam.height / 2;
        }
    }

    auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if(inv) { 
        inv->items.clear();
        inv->equippedMelee = {"", "", 0, PixelsEngine::ItemType::WeaponMelee, 0};
        inv->equippedRanged = {"", "", 0, PixelsEngine::ItemType::WeaponRanged, 0};
        inv->equippedArmor = {"", "", 0, PixelsEngine::ItemType::Armor, 0};
        inv->AddItem("Potion", 3, PixelsEngine::ItemType::Consumable, 0, "assets/ui/item_potion.png", 50);
        inv->AddItem("Thieves' Tools", 1, PixelsEngine::ItemType::Tool, 0, "assets/thieves_tools.png", 25);
    }
    
    for(int i=0; i<6; ++i) m_CC_TempStats[i] = 10;
    m_CC_ClassIndex = 0; m_CC_RaceIndex = 0; m_CC_SelectionIndex = 0; m_CC_PointsRemaining = 5;
}

void PixelsGateGame::HandleMainMenuInput() { 
    int w = GetWindowWidth(); int h = GetWindowHeight();
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
     // Apply visual offset

    int hovered = -1;
    int y = 250;
    for (int i = 0; i < 6; ++i) {
        SDL_Rect btn = {(w/2) - 150, y - 5, 300, 30};
        if (mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) hovered = i;
        y += 40;
    }

    HandleMenuNavigation(6, [&](int selection){
        switch (selection) {
            case 0: 
                if (std::filesystem::exists("quicksave.dat")) {
                     PixelsEngine::SaveSystem::LoadWorldFlags("quicksave.dat", m_WorldFlags);
                     TriggerLoadTransition("quicksave.dat");
                } else if (std::filesystem::exists("savegame.dat")) {
                     PixelsEngine::SaveSystem::LoadWorldFlags("savegame.dat", m_WorldFlags);
                     TriggerLoadTransition("savegame.dat");
                }
                break;
            case 1: ResetGame(); m_State = GameState::Creation; break;
            case 2: if (std::filesystem::exists("savegame.dat")) TriggerLoadTransition("savegame.dat"); break;
            case 3: m_State = GameState::Options; break;
            case 4: m_State = GameState::Credits; break;
            case 5: m_IsRunning = false; break;
        }
    }, nullptr, hovered);
}

void PixelsGateGame::HandleDialogueInput() {
    if (m_DiceRoll.active) return; // Block input while rolling dice

    auto *d = GetRegistry().GetComponent<PixelsEngine::DialogueComponent>(m_DialogueWith);
    if(!d || d->tree->nodes.find(d->tree->currentNodeId) == d->tree->nodes.end()) {
        m_State = m_ReturnState; 
        return;
    }

    auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    std::vector<PixelsEngine::DialogueOption*> visibleOptions;
    for (auto &opt : d->tree->nodes[d->tree->currentNodeId].options) {
        bool visible = true;
        if (!opt.repeatable && opt.hasBeenChosen) visible = false;
        if (visible && !opt.requiredFlag.empty() && !m_WorldFlags[opt.requiredFlag]) visible = false;
        if (visible && !opt.excludeFlag.empty() && m_WorldFlags[opt.excludeFlag]) visible = false;
        if (visible && !opt.requiredItem.empty()) {
            bool hasItem = false;
            if (inv) for (auto &it : inv->items) if (it.name == opt.requiredItem) hasItem = true;
            if (!hasItem) visible = false;
        }
        if (visible) visibleOptions.push_back(&opt);
    }

    int count = visibleOptions.size();
    int w = GetWindowWidth(); int h = GetWindowHeight();
    int panelW = 600;
    int panelX = (w - panelW) / 2;
    int panelY = h - 350;
    int optY = panelY + 50 + 60 + 20; 
    int hovered = -1;
    
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
     // Match global offset

    for(int i=0; i<count; ++i) {
        SDL_Rect r = {panelX + 20, optY - 5, panelW - 40, 30};
        if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) hovered = i;
        optY += 25; 
    }

    HandleMenuNavigation(count, [&](int i){
        auto &opt = *visibleOptions[i];
        opt.hasBeenChosen = true; // Mark as chosen
        
        // Handle Checks (DC)
        if (opt.requiredStat != "None" && opt.dc > 0) {
            auto *stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
            int mod = 0;
            if (stats) {
                if (opt.requiredStat == "Strength") mod = stats->GetModifier(stats->strength);
                else if (opt.requiredStat == "Dexterity") mod = stats->GetModifier(stats->dexterity);
                else if (opt.requiredStat == "Constitution") mod = stats->GetModifier(stats->constitution);
                else if (opt.requiredStat == "Intelligence") mod = stats->GetModifier(stats->intelligence);
                else if (opt.requiredStat == "Wisdom") mod = stats->GetModifier(stats->wisdom);
                else if (opt.requiredStat == "Charisma") mod = stats->GetModifier(stats->charisma);
            }
            StartDiceRoll(mod, opt.dc, opt.requiredStat, m_DialogueWith, PixelsEngine::ContextActionType::Talk);
            return; 
        }

        if (opt.action == PixelsEngine::DialogueAction::EndConversation) {
            m_State = m_ReturnState;
        } else if (opt.action == PixelsEngine::DialogueAction::StartCombat) {
            m_State = m_ReturnState;
            StartCombat(m_DialogueWith);
        } else if (opt.action == PixelsEngine::DialogueAction::GiveItem) {
            m_TradingWith = m_DialogueWith;
            m_ReturnState = GameState::Playing; 
            m_State = GameState::Trading;
        } else if (opt.action == PixelsEngine::DialogueAction::StartQuest) {
            auto *q = GetRegistry().GetComponent<PixelsEngine::QuestComponent>(m_DialogueWith);
            if(q) q->state = 1;
            m_WorldFlags[opt.actionParam] = true;
            d->tree->currentNodeId = opt.nextNodeId;
        } else if (opt.action == PixelsEngine::DialogueAction::CompleteQuest) {
            auto *q = GetRegistry().GetComponent<PixelsEngine::QuestComponent>(m_DialogueWith);
            if(q) q->state = 2;
            m_WorldFlags[opt.actionParam] = true;
            auto *invComp = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
            auto *stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
            if(invComp) {
                invComp->AddItem("Coins", 400); // Reward
                // Remove the quest item
                for (auto it = invComp->items.begin(); it != invComp->items.end(); ++it) {
                    if (it->name == q->targetItem) {
                        it->quantity--;
                        if (it->quantity <= 0) invComp->items.erase(it);
                        break;
                    }
                }
            }
            if(stats) stats->experience += 500;
            SpawnFloatingText(0, 0, "Quest Complete: +400G, +500 XP", {0, 255, 0, 255});
            d->tree->currentNodeId = opt.nextNodeId;
        } else {
            d->tree->currentNodeId = opt.nextNodeId;
        }
        
        if (d->tree->currentNodeId == "end" || d->tree->nodes.find(d->tree->currentNodeId) == d->tree->nodes.end()) {
            m_State = m_ReturnState;
        }
        m_DialogueSelection = 0; 
        m_MenuSelection = 0;
    }, [&](){ m_State = m_ReturnState; }, hovered);
    
    m_DialogueSelection = m_MenuSelection; // Keep them in sync for rendering
}

void PixelsGateGame::HandleCharacterMenuInput() {
    int w = GetWindowWidth(); int h = GetWindowHeight();
    int panelW = 800, panelH = 500;
    int panelX = (w - panelW) / 2;
    int panelY = (h - panelH) / 2;

    if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
         // Apply visual offset
        int tabW = panelW / 3;
        for (int i = 0; i < 3; ++i) {
            SDL_Rect tabRect = {panelX + i * tabW, panelY - 40, tabW, 40};
            if (mx >= tabRect.x && mx <= tabRect.x + tabRect.w && 
                my >= tabRect.y && my <= tabRect.y + tabRect.h) {
                m_CharacterTab = i;
                return;
            }
        }

        if (m_CharacterTab == 0) { // Inventory
            auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
            if (inv) {
                float currentTime = SDL_GetTicks() / 1000.0f;
                bool isDoubleClick = (currentTime - m_LastClickTime) < 0.3f;
                m_LastClickTime = currentTime;

                if (isDoubleClick) {
                    int sx = panelX + 50, sy = panelY + 80;
                    int iy = sy + 110;
                    int ix = panelX + 50;
                    int count = 0;

                    for (size_t i = 0; i < inv->items.size(); ++i) {
                        if (mx >= ix && mx <= ix + 200 && my >= iy && my <= iy + 32) {
                            auto &item = inv->items[i];
                            PixelsEngine::Item *slot = nullptr;
                            if (item.type == PixelsEngine::ItemType::WeaponMelee) slot = &inv->equippedMelee;
                            else if (item.type == PixelsEngine::ItemType::WeaponRanged) slot = &inv->equippedRanged;
                            else if (item.type == PixelsEngine::ItemType::Armor) slot = &inv->equippedArmor;

                            if (slot) {
                                PixelsEngine::Item oldItem = *slot;
                                *slot = item;
                                slot->quantity = 1;
                                item.quantity--;
                                if (item.quantity <= 0) inv->items.erase(inv->items.begin() + i);
                                if (!oldItem.IsEmpty()) inv->AddItemObject(oldItem);
                                SpawnFloatingText(0, 0, "Equipped " + slot->name, {0, 255, 255, 255});
                            }
                            return;
                        }
                        iy += 45;
                        count++;
                        if (count % 8 == 0) { ix += 250; iy = sy + 110; }
                    }
                }
            }
        }

        if (m_CharacterTab == 2) { // Spellbook
             std::string spells[] = {"Fireball", "Heal", "Magic Missile", "Shield"};
             int y = panelY + 100;
             for (int i = 0; i < 4; ++i) {
                 SDL_Rect row = {panelX + 50, y, panelW - 100, 40};
                 if (mx >= row.x && mx <= row.x + row.w && my >= row.y && my <= row.y + row.h) {
                     m_PendingSpellName = spells[i];
                     if (m_PendingSpellName == "Shield") {
                         CastSpell("Shield", m_Player);
                     } else {
                         m_ReturnState = GameState::Playing; // Default back to playing after spellbook
                         m_State = GameState::Targeting;
                     }
                     return;
                 }
                 y += 50;
             }
        }
    }

    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
        m_State = m_ReturnState;
    }
}

void PixelsGateGame::HandleMapInput() {
    int w = GetWindowWidth(); int h = GetWindowHeight();
    if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        int tabW = 150;
        int startX = w / 2 - tabW;
        SDL_Rect mapT = {startX, 20, tabW, 40};
        SDL_Rect jrnT = {startX+tabW, 20, tabW, 40};
        
        if (mx >= mapT.x && mx <= mapT.x + mapT.w && my >= mapT.y && my <= mapT.y + mapT.h) m_MapTab = 0;
        if (mx >= jrnT.x && mx <= jrnT.x + jrnT.w && my >= jrnT.y && my <= jrnT.y + jrnT.h) m_MapTab = 1;
    }

    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE) || 
        PixelsEngine::Input::IsKeyPressed(PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Map))) {
        m_State = m_ReturnState;
    }
}

void PixelsGateGame::HandleRestMenuInput() {
    int w = GetWindowWidth(); int h = GetWindowHeight();
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
     // Apply visual offset
    
    int boxW = 400; int boxH = 250;
    int boxX = (w - boxW) / 2; int boxY = (h - boxH) / 2;
    int hovered = -1;
    int y = boxY + 80;
    for(int i=0; i<4; ++i) {
        SDL_Rect r = {boxX + 50, y - 15, boxW - 100, 30};
        if(mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) hovered = i;
        y += 40;
    }

    HandleMenuNavigation(4, [&](int selection){
        if (selection == 0) { // Short Rest
            auto *s = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
            if (s && s->shortRestsAvailable > 0) {
                s->shortRestsAvailable--;
                s->currentHealth = std::min(s->maxHealth, s->currentHealth + (s->maxHealth/2));
                m_State = m_ReturnState;
            }
        } else if (selection == 1) { // Long Rest
            auto *s = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
            if(s) {
                s->currentHealth = s->maxHealth;
                s->shortRestsAvailable = s->maxShortRests;
            }
            m_State = m_ReturnState;
        } else if (selection == 2) { // Camp
            if (m_ReturnState == GameState::Camp) {
                auto *t = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
                if(t) { t->x = m_LastWorldPos.x; t->y = m_LastWorldPos.y; }
                m_State = GameState::Playing;
            } else {
                auto *t = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
                if(t) { 
                    m_LastWorldPos.x = t->x; m_LastWorldPos.y = t->y; 
                    t->x = 7.0f; t->y = 7.0f; 
                }
                m_State = GameState::Camp;
            }
        } else { // Back
            m_State = m_ReturnState;
        }
    }, [&](){ m_State = m_ReturnState; }, hovered);
}

void PixelsGateGame::HandleCombatInput() {
    if (PixelsEngine::Input::IsKeyPressed(PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::EndTurn))) {
        NextTurn(); return;
    }
    float dx=0, dy=0;
    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_W)) dy=-1;
    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_S)) dy=1;
    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A)) dx=-1;
    if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D)) dx=1;

    if ((dx!=0 || dy!=0) && m_Combat.m_MovementLeft > 0) {
        float speed = 5.0f; float cost = speed * 0.016f;
        if (m_Combat.m_MovementLeft >= cost) {
            auto *t = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
            auto *currentMap = GetCurrentMap();
            if (currentMap->IsWalkable(t->x + dx*cost, t->y + dy*cost)) {
                t->x += dx*cost; t->y += dy*cost;
                m_Combat.m_MovementLeft -= cost;
            }
        }
    }
}

void PixelsGateGame::HandlePauseMenuInput() { 
    int w = GetWindowWidth(); int h = GetWindowHeight();
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
     // Apply visual offset

    int boxY = (h - 400) / 2;
    int y = boxY + 80;
    int hovered = -1;
    for (int i = 0; i < 6; ++i) {
        SDL_Rect btn = {(w/2) - 100, y - 5, 200, 30};
        if (mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) hovered = i;
        y += 40;
    }
    HandleMenuNavigation(6, [&](int i){ 
        switch(i) {
            case 0: m_State = m_ReturnState; break;
            case 1: PixelsEngine::SaveSystem::SaveGame("savegame.dat", GetRegistry(), m_Player, *m_Level, m_State == GameState::Camp, m_LastWorldPos.x, m_LastWorldPos.y); ShowSaveMessage(); break;
            case 2: TriggerLoadTransition("savegame.dat"); break;
            case 3: m_ReturnState = GameState::Paused; m_State = GameState::KeybindSettings; break;
            case 4: m_ReturnState = GameState::Paused; m_State = GameState::Options; break;
            case 5: m_State = GameState::MainMenu; break;
        }
    }, [&](){ m_State = m_ReturnState; }, hovered); 
}

PixelsEngine::Entity PixelsGateGame::GetEntityAtMouse() {
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
    auto &camera = GetCamera();
    auto *currentMap = GetCurrentMap();
    
    PixelsEngine::Entity bestTarget = PixelsEngine::INVALID_ENTITY;
    float bestDist = 1000.0f;

    auto &transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
    for (auto &[ent, t] : transforms) {
        if (ent == m_Player) continue;

        int sx, sy;
        currentMap->GridToScreen(t.x, t.y, sx, sy);
        
        // Center of the tile/entity
        float cx = (float)(sx - camera.x + 16);
        float cy = (float)(sy - camera.y + 8);
        
        float dx = (float)mx - cx;
        float dy = (float)my - cy;
        float dist = std::sqrt(dx*dx + dy*dy);

        // Within ~1 tile radius (increased for easier clicking)
        if (dist < 32.0f && dist < bestDist) {
            bestDist = dist;
            bestTarget = ent;
        }
    }
    return bestTarget;
}

void PixelsGateGame::HandleOptionsInput() {
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
    
    int hovered = -1;
    int y = GetWindowHeight()/2 - 40;
    for(int i=0; i<2; ++i) {
        SDL_Rect btn = {GetWindowWidth()/2 - 100, y - 5, 200, 30};
        if(mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) hovered = i;
        y += 40;
    }
    HandleMenuNavigation(2, [&](int i){ if(i==0) ToggleFullScreen(); else m_State=m_ReturnState; }, [&](){ m_State=m_ReturnState; }, hovered);
}

void PixelsGateGame::HandleGameOverInput() {
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
    
    int hovered = -1;
    int y = GetWindowHeight()/2;
    for(int i=0; i<2; ++i) {
        SDL_Rect btn = {GetWindowWidth()/2 - 100, y - 5, 200, 30};
        if(mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) hovered = i;
        y += 50;
    }
    HandleMenuNavigation(2, [&](int i){ if(i==0) TriggerLoadTransition("savegame.dat"); else m_State=GameState::MainMenu; }, nullptr, hovered);
}

void PixelsGateGame::HandleTargetingInput() {
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
        m_State = m_ReturnState;
        return;
    }
    if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
        CastSpell(m_PendingSpellName, GetEntityAtMouse());
    }
}

void PixelsGateGame::HandleTargetingJumpInput() {
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
        m_State = m_ReturnState;
        return;
    }
    if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        auto &camera = GetCamera();
        auto *currentMap = GetCurrentMap();
        int gx, gy;
        currentMap->ScreenToGrid(mx + camera.x, my + camera.y, gx, gy);
        PerformJump(gx, gy);
    }
}

void PixelsGateGame::HandleTargetingShoveInput() {
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
        m_State = m_ReturnState;
        return;
    }
    if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
        PerformShove(GetEntityAtMouse());
    }
}

void PixelsGateGame::HandleTargetingDashInput() {

    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {

        m_State = m_ReturnState;

        return;

    }

        if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {

            int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);

            auto &camera = GetCamera();

            auto *currentMap = GetCurrentMap();

            int gx, gy;

            currentMap->ScreenToGrid(mx + camera.x, my + camera.y, gx, gy);

            PerformDash(gx, gy);

        }

}



void PixelsGateGame::HandleTradeInput() {
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
        m_State = m_ReturnState;
        return;
    }

    if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
         // Apply visual offset
        int w = GetWindowWidth();
        int h = GetWindowHeight();

        auto *pInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
        auto *tInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_TradingWith);
        if (!pInv || !tInv) return;

        float currentTime = SDL_GetTicks() / 1000.0f;
        bool isDoubleClick = (currentTime - m_LastClickTime) < 0.3f;
        m_LastClickTime = currentTime;

        if (isDoubleClick) {
            // Check Player Items (Selling)
            if (mx >= 50 && mx <= 300) {
                int y = 140;
                for (size_t i = 0; i < pInv->items.size(); ++i) {
                    if (my >= y && my <= y + 32) {
                        auto &item = pInv->items[i];
                        if (item.name == "Coins") continue; // Can't sell gold

                        int price = item.value;
                        if (tInv->GetGoldCount() >= price) {
                            tInv->RemoveGold(price);
                            pInv->AddItem("Coins", price);
                            tInv->AddItemObject(item);
                            tInv->items.back().quantity = 1; // Only move one
                            item.quantity--;
                            if (item.quantity <= 0) pInv->items.erase(pInv->items.begin() + i);
                            SpawnFloatingText(0, 0, "Sold " + item.name, {255, 255, 0, 255});
                        } else {
                            SpawnFloatingText(0, 0, "Trader too poor!", {255, 0, 0, 255});
                        }
                        return;
                    }
                    y += 40;
                }
            }
            // Check Trader Items (Buying)
            else if (mx >= w / 2 + 50 && mx <= w / 2 + 300) {
                int y = 140;
                for (size_t i = 0; i < tInv->items.size(); ++i) {
                    if (my >= y && my <= y + 32) {
                        auto &item = tInv->items[i];
                        if (item.name == "Coins") continue;

                        int price = item.value;
                        if (pInv->GetGoldCount() >= price) {
                            if (pInv->items.size() < pInv->capacity) {
                                pInv->RemoveGold(price);
                                tInv->AddItem("Coins", price);
                                pInv->AddItemObject(item);
                                pInv->items.back().quantity = 1;
                                item.quantity--;
                                if (item.quantity <= 0) tInv->items.erase(tInv->items.begin() + i);
                                SpawnFloatingText(0, 0, "Bought " + item.name, {0, 255, 0, 255});
                            } else {
                                SpawnFloatingText(0, 0, "Inventory Full!", {255, 0, 0, 255});
                            }
                        } else {
                            SpawnFloatingText(0, 0, "Too expensive!", {255, 0, 0, 255});
                        }
                        return;
                    }
                    y += 40;
                }
            }
        }
    }
}
void PixelsGateGame::HandleKeybindInput() { if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) m_State=GameState::Paused; }
void PixelsGateGame::HandleLootInput() {
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
        m_State = m_ReturnState;
        return;
    }

    if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        
        auto *loot = GetRegistry().GetComponent<PixelsEngine::LootComponent>(m_LootingEntity);
        if (!loot) return;

        int winW = GetWindowWidth();
        int winH = GetWindowHeight();
        int pX = (winW - 400) / 2;
        int pY = (winH - 500) / 2;

        // Take All Button
        SDL_Rect btnRect = {pX + 125, pY + 440, 150, 40};
        if (mx >= btnRect.x && mx <= btnRect.x + btnRect.w && my >= btnRect.y && my <= btnRect.y + btnRect.h) {
            auto *pInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
            if (pInv) {
                bool collectedAny = false;
                auto it = loot->drops.begin();
                while (it != loot->drops.end()) {
                    // Check capacity (simple count check, though stackable items might not increase count if they stack)
                    // AddItemObject handles stacking, so size() check is approximation. 
                    // Ideally AddItemObject should return success/fail or we check specific item stacking.
                    // For now, assume if size < capacity we can try. 
                    // Actually AddItemObject just pushes back if not found.
                    // Let's rely on pInv->items.size() check inside the loop effectively.
                    
                    // Check if item stacks with existing
                    bool stacks = false;
                    for(auto &invItem : pInv->items) {
                        if(invItem.name == it->name) { stacks = true; break; }
                    }

                    if (stacks || pInv->items.size() < pInv->capacity) {
                        pInv->AddItemObject(*it);
                        collectedAny = true;
                        it = loot->drops.erase(it);
                    } else {
                        SpawnFloatingText(0, 0, "Inventory Full!", {255, 0, 0, 255});
                        ++it;
                    }
                }
                if (collectedAny) SpawnFloatingText(0, 0, "Looted All", {200, 255, 200, 255});
                if (loot->drops.empty()) m_State = m_ReturnState;
            }
            return;
        }

        float currentTime = SDL_GetTicks() / 1000.0f;
        bool isDoubleClick = (currentTime - m_LastClickTime) < 0.3f;
        m_LastClickTime = currentTime;

        if (isDoubleClick) {
            int y = pY + 80;
            int x = pX + 30;
            for (size_t i = 0; i < loot->drops.size(); ++i) {
                if (my >= y && my <= y + 32) {
                    auto *pInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
                    if (pInv && pInv->items.size() < pInv->capacity) {
                        pInv->AddItemObject(loot->drops[i]);
                        SpawnFloatingText(0, 0, "Looted " + loot->drops[i].name, {200, 255, 200, 255});
                        loot->drops.erase(loot->drops.begin() + i);
                        if (loot->drops.empty()) m_State = m_ReturnState;
                    }
                    return;
                }
                y += 45;
            }
        }
    }
}

void PixelsGateGame::HandleInventoryInput() {}