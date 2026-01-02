#include "PixelsGateGame.h"
#include "../engine/TextureManager.h"
#include "../engine/Dice.h"
#include "../engine/Tiles.h"
#include "../engine/Input.h"

static const char *statNames[] = {"Strength", "Dexterity", "Constitution", "Intelligence", "Wisdom", "Charisma"};

void PixelsGateGame::InitCharacterCreation() {
    // Logic handled in ResetGame(), this stub keeps the linker happy.
}

void PixelsGateGame::RenderCharacterCreation() {
    SDL_Renderer *renderer = GetRenderer();
    m_TextRenderer->RenderTextCentered("CHARACTER CREATION", 400, 50, {255, 215, 0, 255});
    m_TextRenderer->RenderTextCentered("Points Remaining: " + std::to_string(m_CC_PointsRemaining), 400, 90, {200, 255, 200, 255});

    int y = 130;
    SDL_Color selectedColor = {50, 255, 50, 255};
    SDL_Color normalColor = {255, 255, 255, 255};

    for (int i = 0; i < 6; ++i) {
        std::string text = std::string(statNames[i]) + ": " + std::to_string(m_CC_TempStats[i]);
        m_TextRenderer->RenderTextCentered(text, 400, y, (m_CC_SelectionIndex == i) ? selectedColor : normalColor);
        if (m_CC_SelectionIndex == i) {
            m_TextRenderer->RenderTextCentered(">", 280, y, selectedColor);
            m_TextRenderer->RenderTextCentered("<", 520, y, selectedColor);
        }
        y += 35;
    }
    y += 20;
    
    // Class
    m_TextRenderer->RenderTextCentered("Class: " + m_CC_Classes[m_CC_ClassIndex], 400, y, (m_CC_SelectionIndex == 6) ? selectedColor : normalColor);
    if (m_CC_SelectionIndex == 6) { 
        m_TextRenderer->RenderTextCentered(">", 250, y, selectedColor); 
        m_TextRenderer->RenderTextCentered("<", 550, y, selectedColor); 
    }
    y += 35;

    // Race
    m_TextRenderer->RenderTextCentered("Race: " + m_CC_Races[m_CC_RaceIndex], 400, y, (m_CC_SelectionIndex == 7) ? selectedColor : normalColor);
    if (m_CC_SelectionIndex == 7) { 
        m_TextRenderer->RenderTextCentered(">", 250, y, selectedColor); 
        m_TextRenderer->RenderTextCentered("<", 550, y, selectedColor); 
    }
    y += 50;

    // Start Button
    SDL_Rect btnRect = {300, y, 200, 50};
    SDL_SetRenderDrawColor(renderer, (m_CC_SelectionIndex == 8) ? 100 : 50, (m_CC_SelectionIndex == 8) ? 200 : 150, 50, 255);
    SDL_RenderFillRect(renderer, &btnRect);
    m_TextRenderer->RenderTextCentered("START ADVENTURE", 400, y + 25, {255, 255, 255, 255});
    m_TextRenderer->RenderTextCentered("Controls: W/S to Select, A/D to Change, ENTER to Start", 400, 550, {150, 150, 150, 255});
}

void PixelsGateGame::RenderHUD() {
    SDL_Renderer *renderer = GetRenderer();
    int winW = GetWindowWidth(); int winH = GetWindowHeight();

    if (!m_TooltipPinned) m_HoveredItemName = "";

    // 1. Health Bar
    auto *stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    if (stats) {
        int barW = 200; int x = 20; int y = 20;
        SDL_Rect bg = {x, y, barW, 20};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &bg);
        float pct = (float)stats->currentHealth / (float)stats->maxHealth;
        SDL_Rect fg = {x, y, (int)(barW * (pct < 0 ? 0 : pct)), 20};
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        SDL_RenderFillRect(renderer, &fg);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &bg);
        m_TextRenderer->RenderText("HP: " + std::to_string(stats->currentHealth) + "/" + std::to_string(stats->maxHealth), x + 10, y + 25, {255, 255, 255, 255});
    }

    // 2. Action Bar Background
    int barH = 100;
    SDL_Rect hudRect = {0, winH - barH, winW, barH};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &hudRect);
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderDrawRect(renderer, &hudRect);

    // --- Helper: Render Action Grids ---
    auto DrawGrid = [&](const std::string &title, int startX, 
                        const std::vector<std::string> &labels, 
                        const std::vector<std::string> &keys, 
                        const std::vector<std::string> &icons) {
        m_TextRenderer->RenderText(title, startX, winH - barH + 5, {200, 200, 200, 255});
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        for (int i = 0; i < 6; ++i) {
            int row = i / 3; int col = i % 3;
            SDL_Rect btn = {startX + col * 45, winH - barH + 25 + row * 35, 40, 30};
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
            SDL_RenderFillRect(renderer, &btn);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &btn);

            // Hover Logic for Tooltips
            if (mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) {
                if (title == "ACTIONS" || title == "SPELLS") { 
                    if (i < (int)labels.size()) m_HoveredItemName = labels[i]; 
                }
                else if (title == "ITEMS") { 
                    auto items = GetHotbarItems(); 
                    if (i < (int)items.size()) m_HoveredItemName = items[i]; 
                }
            }

            // Draw Icon
            bool iconDrawn = false;
            std::string iconPath = (i < (int)icons.size()) ? icons[i] : "";
            
            // Dynamic Weapon Icon for "Atk" (Index 0 in Actions)
            if (title == "ACTIONS" && i == 0) {
                if (m_SelectedWeaponSlot == 0) iconPath = "assets/sword.png";
                else iconPath = "assets/bow.png";
            }

            if (!iconPath.empty()) {
                auto tex = PixelsEngine::TextureManager::LoadTexture(renderer, iconPath);
                if (tex) { tex->Render(btn.x + 8, btn.y + 3, 24, 24); iconDrawn = true; }
            }
            // Draw Label if no icon
            if (!iconDrawn && i < (int)labels.size() && !labels[i].empty()) {
                m_TextRenderer->RenderTextCentered(labels[i], btn.x + 20, btn.y + 15, {255, 255, 255, 255});
            }
            
            // Draw Keybind Hint
            if (i < (int)keys.size() && !keys[i].empty()) {
                m_TextRenderer->RenderTextSmall(keys[i], btn.x + 2, btn.y + 20, {255, 255, 0, 200});
            }
            
            // Weapon Toggle Indicators
            if (title == "ACTIONS" && i == 0) {
                SDL_Rect mRect = {btn.x - 15, btn.y, 12, 14};
                SDL_SetRenderDrawColor(renderer, (m_SelectedWeaponSlot == 0) ? 200 : 50, 50, 50, 255);
                SDL_RenderFillRect(renderer, &mRect);
                SDL_Rect rRect = {btn.x - 15, btn.y + 16, 12, 14};
                SDL_SetRenderDrawColor(renderer, (m_SelectedWeaponSlot == 1) ? 200 : 50, 50, 50, 255);
                SDL_RenderFillRect(renderer, &rRect);
            }
        }
    };

    // Actions Data
    std::vector<std::string> actions = {"Atk", "Jmp", "Snk", "Shv", "Dsh", "End"};
    std::vector<std::string> actKeys = {"SHT/F", "Z", "C", "V", "B", "SPC"};
    std::vector<std::string> actIcons = {"", "assets/ui/action_jump.png", "assets/ui/action_sneak.png", "assets/ui/action_shove.png", "assets/ui/action_dash.png", "assets/ui/action_endturn.png"};
    DrawGrid("ACTIONS", 20, actions, actKeys, actIcons);

    // Spells Data
    std::vector<std::string> spells = {"Fir", "Hel", "Mis", "Shd", "", ""};
    std::vector<std::string> spellKeys = {"", "", "", "", "", ""};
    std::vector<std::string> spellIcons = {"assets/ui/spell_fireball.png", "assets/ui/spell_heal.png", "assets/ui/spell_magicmissile.png", "assets/ui/spell_shield.png", "", ""};
    DrawGrid("SPELLS", 170, spells, spellKeys, spellIcons);

    // Items Data
    std::vector<std::string> hotbarItems = GetHotbarItems();
    std::vector<std::string> itemKeys = {"1", "2", "3", "4", "5", "6"};
    std::vector<std::string> hIcons(6, ""); 
    auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if (inv) {
        for(int i=0; i<6; ++i) {
            if (hotbarItems[i].empty()) continue;
            for(auto &it : inv->items) {
                if(it.name == hotbarItems[i]) hIcons[i] = it.iconPath;
            }
            // Fallbacks
            if(hIcons[i].empty() && hotbarItems[i] == "Potion") hIcons[i] = "assets/ui/item_potion.png";
            if(hIcons[i].empty() && hotbarItems[i] == "Bread") hIcons[i] = "assets/ui/item_bread.png";
            if(hIcons[i].empty() && hotbarItems[i] == "Boar Meat") hIcons[i] = "assets/ui/item_boarmeat.png";
        }
    }
    DrawGrid("ITEMS", 320, hotbarItems, itemKeys, hIcons);

    // Right-Side System Buttons (Map, Character, Rest, Inventory)
    const char *sysLabels[] = {"Map", "Chr", "", "Inv"};
    const char *sysKeys[] = {"M", "O", "ESC", "I"};
    const char *sysIcons[] = {"", "", "assets/ui/action_rest.png", ""}; // Use Rest icon for ESC menu
    
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
    for (int i = 0; i < 4; ++i) {
        SDL_Rect btn = {winW - 180 + (i % 2) * 85, winH - 100 + 15 + (i / 2) * 40, 75, 35};
        
        bool hover = (mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h);
        SDL_SetRenderDrawColor(renderer, hover ? 120 : 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &btn);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &btn);

        bool iconDrawn = false;
        if (sysIcons[i][0] != '\0') {
            auto tex = PixelsEngine::TextureManager::LoadTexture(renderer, sysIcons[i]);
            if (tex) {
                tex->Render(btn.x + (btn.w - 24) / 2, btn.y + (btn.h - 24) / 2, 24, 24);
                iconDrawn = true;
            }
        }

        if (!iconDrawn) {
            m_TextRenderer->RenderTextCentered(sysLabels[i], btn.x + 37, btn.y + 17, {255, 255, 255, 255});
        }
        m_TextRenderer->RenderTextSmall(sysKeys[i], btn.x + 5, btn.y + 22, {255, 255, 0, 200});
    }

    // Tooltip Rendering
    if (!m_HoveredItemName.empty()) {
        auto it = m_Tooltips.find(m_HoveredItemName);
        if (it != m_Tooltips.end()) {
            RenderTooltip(it->second, m_TooltipPinned ? m_PinnedTooltipX : mx + 15, m_TooltipPinned ? m_PinnedTooltipY : my - 150);
        }
    }
}

void PixelsGateGame::RenderInventory() {
    auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if (!inv) return;
    
    SDL_Renderer *r = GetRenderer();
    int w=800, h=500; // Match CharacterMenu panel size
    SDL_Rect p = {(GetWindowWidth()-w)/2, (GetWindowHeight()-h)/2, w, h};
    
    // Header
    m_TextRenderer->RenderTextCentered("INVENTORY", p.x+w/2, p.y+20, {255,255,255,255});

    // Slots (Equipment)
    auto DrawSlot = [&](const std::string &l, PixelsEngine::Item &i, int x, int y) {
        SDL_Rect s = {x, y, 48, 48};
        SDL_SetRenderDrawColor(r, 30, 20, 10, 255); SDL_RenderFillRect(r, &s);
        SDL_SetRenderDrawColor(r, 200, 200, 200, 255); SDL_RenderDrawRect(r, &s);
        m_TextRenderer->RenderTextCentered(l, x+24, y-15, {200,200,200,255});
        if(!i.IsEmpty()) {
            std::string path = i.iconPath;
            if (path.empty()) {
                if (i.name == "Potion") path = "assets/ui/item_potion.png";
                else if (i.name == "Bread") path = "assets/ui/item_bread.png";
                else if (i.name == "Boar Meat") path = "assets/ui/item_boarmeat.png";
                else if (i.name == "Coins") path = "assets/ui/item_coins.png";
                else path = "assets/ui/item_potion.png"; // General fallback
            }
            auto t = PixelsEngine::TextureManager::LoadTexture(r, path);
            if(t) t->Render(x+8, y+8, 32, 32);
        }
    };
    
    int sx = p.x + 50, sy = p.y + 80;
    DrawSlot("Melee", inv->equippedMelee, sx, sy);
    DrawSlot("Ranged", inv->equippedRanged, sx + 100, sy);
    DrawSlot("Armor", inv->equippedArmor, sx + 200, sy);

    // Items List
    m_TextRenderer->RenderText("Items", p.x + 50, sy + 80, {255, 215, 0, 255});
    int iy = sy + 110;
    int ix = p.x + 50;
    int count = 0;
    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);

    for(auto &it : inv->items) {
        // Hover Detection
        if (mx >= ix && mx <= ix + 220 && my >= iy && my <= iy + 40) {
            SDL_Rect highlight = {ix - 5, iy, 230, 40};
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(r, 255, 255, 255, 40);
            SDL_RenderFillRect(r, &highlight);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
            m_HoveredItemName = it.name;
        }

        RenderInventoryItem(it, ix, iy);
        iy += 45;
        count++;
        if (count % 8 == 0) { // Column wrap
            ix += 250;
            iy = sy + 110;
        }
    }
}

void PixelsGateGame::RenderInventoryItem(const PixelsEngine::Item &item, int x, int y) {
    std::string path = item.iconPath;
    if (path.empty()) {
        if (item.name == "Potion") path = "assets/ui/item_potion.png";
        else if (item.name == "Bread") path = "assets/ui/item_bread.png";
        else if (item.name == "Boar Meat") path = "assets/ui/item_boarmeat.png";
        else if (item.name == "Coins") path = "assets/ui/item_coins.png";
        else path = "assets/ui/item_potion.png"; // General fallback
    }
    auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), path);
    if(tex) tex->Render(x, y, 32, 32);
    m_TextRenderer->RenderText(item.name + " x" + std::to_string(item.quantity), x+40, y+8, {255,255,255,255});
}

void PixelsGateGame::RenderContextMenu() {
    if (!m_ContextMenu.isOpen) return;
    SDL_Renderer *r = GetRenderer();
    SDL_Rect m = {m_ContextMenu.x, m_ContextMenu.y, 150, (int)m_ContextMenu.actions.size()*30 + 10};
    SDL_SetRenderDrawColor(r, 50, 50, 50, 255); SDL_RenderFillRect(r, &m);
    SDL_SetRenderDrawColor(r, 200, 200, 200, 255); SDL_RenderDrawRect(r, &m);
    int y = m_ContextMenu.y + 5;
    for(auto &a : m_ContextMenu.actions) {
        m_TextRenderer->RenderText(a.label, m_ContextMenu.x + 10, y, {255,255,255,255});
        y += 30;
    }
}

void PixelsGateGame::RenderDiceRoll() {
    if (!m_DiceRoll.active) return;
    int w=GetWindowWidth(), h=GetWindowHeight();
    SDL_SetRenderDrawColor(GetRenderer(), 0,0,0,150);
    SDL_Rect o = {0,0,w,h}; SDL_RenderFillRect(GetRenderer(), &o);
    SDL_Rect b = {(w-400)/2, (h-300)/2, 400, 300};
    SDL_SetRenderDrawColor(GetRenderer(), 40,40,40,255); SDL_RenderFillRect(GetRenderer(), &b);
    SDL_SetRenderDrawColor(GetRenderer(), 200,200,200,255); SDL_RenderDrawRect(GetRenderer(), &b);
    
    m_TextRenderer->RenderTextCentered("Skill Check: " + m_DiceRoll.skillName, b.x+200, b.y+30, {255,255,255,255});
    m_TextRenderer->RenderTextCentered("DC: " + std::to_string(m_DiceRoll.dc), b.x+200, b.y+60, {200,200,200,255});
    int val = m_DiceRoll.resultShown ? m_DiceRoll.finalValue : PixelsEngine::Dice::Roll(20);
    m_TextRenderer->RenderTextCentered(std::to_string(val), b.x+200, b.y+120, {255,255,0,255});
    
    if (m_DiceRoll.resultShown) {
        std::string res = m_DiceRoll.success ? "SUCCESS" : "FAILURE";
        SDL_Color c = m_DiceRoll.success ? SDL_Color{50,255,50,255} : SDL_Color{255,50,50,255};
        m_TextRenderer->RenderTextCentered(res, b.x+200, b.y+220, c);
        m_TextRenderer->RenderTextCentered("Click to Continue", b.x+200, b.y+260, {255,255,255,255});
    }
}

void PixelsGateGame::RenderTooltip(const TooltipData &data, int x, int y) {
    SDL_Renderer *r = GetRenderer();
    int w = 320;
    int descH = m_TextRenderer->MeasureTextWrapped(data.description, w-20);
    int h = 40 + 30 + descH + 35 + 25 + 20;
    if (x+w > GetWindowWidth()) x = GetWindowWidth()-w-10;
    if (y+h > GetWindowHeight()) y = GetWindowHeight()-h-10;
    if (x < 10) x = 10;
    if (y < 10) y = 10;
    
    SDL_Rect box = {x, y, w, h};
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 20, 20, 25, 240); SDL_RenderFillRect(r, &box);
    SDL_SetRenderDrawColor(r, 100, 100, 120, 255); SDL_RenderDrawRect(r, &box);
    if(m_TooltipPinned) { SDL_SetRenderDrawColor(r, 255, 215, 0, 255); SDL_RenderDrawRect(r, &box); }
    
    int cy = y+10;
    m_TextRenderer->RenderText(data.name, x+10, cy, {255,255,255,255}); cy+=30;
    SDL_SetRenderDrawColor(r, 80,80,80,255); SDL_RenderDrawLine(r, x+10, cy, x+w-10, cy); cy+=10;
    m_TextRenderer->RenderTextSmall("Cost: " + data.cost, x+10, cy, {200,200,200,255});
    m_TextRenderer->RenderTextSmall("Range: " + data.range, x+160, cy, {200,200,255,255}); cy+=30;
    m_TextRenderer->RenderTextWrapped(data.description, x+10, cy, w-20, {180,180,180,255}); cy+=descH+15;
    m_TextRenderer->RenderTextSmall("Effect: " + data.effect, x+10, cy, {255,100,100,255});
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void PixelsGateGame::RenderMainMenu() {
    SDL_SetRenderDrawColor(GetRenderer(), 0, 0, 0, 255);
    SDL_RenderClear(GetRenderer());
    m_TextRenderer->RenderTextCentered("PIXELS GATE", GetWindowWidth()/2, 100, {255, 215, 0, 255});
    std::string opts[] = {"Continue", "New Game", "Load Game", "Options", "Credits", "Quit"};
    int y = 250;
    for(int i=0; i<6; ++i) {
        SDL_Color c = (i==m_MenuSelection) ? SDL_Color{50,255,50,255} : SDL_Color{200,200,200,255};
        m_TextRenderer->RenderTextCentered(opts[i], GetWindowWidth()/2, y, c);
        if(i==m_MenuSelection) {
            m_TextRenderer->RenderTextCentered(">", GetWindowWidth()/2 - 100, y, c);
            m_TextRenderer->RenderTextCentered("<", GetWindowWidth()/2 + 100, y, c);
        }
        y+=40;
    }
}

void PixelsGateGame::RenderPauseMenu() {
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(GetRenderer(), 0, 0, 0, 150);
    SDL_Rect o = {0,0,GetWindowWidth(),GetWindowHeight()}; SDL_RenderFillRect(GetRenderer(), &o);
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);

    SDL_Rect box = {(GetWindowWidth()-300)/2, (GetWindowHeight()-400)/2, 300, 400};
    SDL_SetRenderDrawColor(GetRenderer(), 50, 50, 50, 255); SDL_RenderFillRect(GetRenderer(), &box);
    SDL_SetRenderDrawColor(GetRenderer(), 200, 200, 200, 255); SDL_RenderDrawRect(GetRenderer(), &box);
    
    m_TextRenderer->RenderTextCentered("PAUSED", GetWindowWidth()/2, box.y+30, {255,255,255,255});
    std::string opts[] = {"Resume", "Save", "Load", "Controls", "Options", "Main Menu"};
    int y = box.y+80;
    for(int i=0; i<6; ++i) {
        SDL_Color c = (i==m_MenuSelection) ? SDL_Color{50,255,50,255} : SDL_Color{200,200,200,255};
        m_TextRenderer->RenderTextCentered(opts[i], GetWindowWidth()/2, y, c);
        y+=40;
    }
}

void PixelsGateGame::RenderOptions() {
    SDL_SetRenderDrawColor(GetRenderer(), 20, 20, 20, 255); SDL_RenderClear(GetRenderer());
    m_TextRenderer->RenderTextCentered("OPTIONS", GetWindowWidth()/2, 50, {255, 255, 255, 255});
    std::string opts[] = {"Toggle Fullscreen", "Back"};
    int y = GetWindowHeight()/2 - 40;
    for(int i=0; i<2; ++i) {
        SDL_Color c = (m_MenuSelection==i) ? SDL_Color{50,255,50,255} : SDL_Color{200,200,200,255};
        m_TextRenderer->RenderTextCentered(opts[i], GetWindowWidth()/2, y, c);
        y+=40;
    }
}

void PixelsGateGame::RenderCredits() {
    SDL_SetRenderDrawColor(GetRenderer(), 10, 10, 30, 255); SDL_RenderClear(GetRenderer());
    m_TextRenderer->RenderTextCentered("CREDITS", GetWindowWidth()/2, 50, {255, 255, 255, 255});
    m_TextRenderer->RenderTextCentered("Created by Jesse Wood", GetWindowWidth()/2, 200, {200, 200, 255, 255});
    m_TextRenderer->RenderTextCentered("Press ESC to Back", GetWindowWidth()/2, GetWindowHeight()-50, {100, 100, 100, 255});
}

void PixelsGateGame::RenderControls() {
    SDL_SetRenderDrawColor(GetRenderer(), 20, 20, 20, 255); SDL_RenderClear(GetRenderer());
    m_TextRenderer->RenderTextCentered("CONTROLS", GetWindowWidth()/2, 50, {255, 255, 255, 255});
    m_TextRenderer->RenderTextCentered("W/A/S/D - Move", GetWindowWidth()/2, 150, {200, 200, 200, 255});
    m_TextRenderer->RenderTextCentered("Left Click - Interact", GetWindowWidth()/2, 190, {200, 200, 200, 255});
    m_TextRenderer->RenderTextCentered("Right Click - Menu", GetWindowWidth()/2, 230, {200, 200, 200, 255});
}

void PixelsGateGame::RenderGameOver() {
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(GetRenderer(), 50, 0, 0, 200);
    SDL_Rect o = {0,0,GetWindowWidth(),GetWindowHeight()}; SDL_RenderFillRect(GetRenderer(), &o);
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
    m_TextRenderer->RenderTextCentered("YOU DIED", GetWindowWidth()/2, GetWindowHeight()/3, {255,0,0,255});
    
    std::string opts[] = {"Load Last Save", "Quit"};
    int y = GetWindowHeight()/2;
    for(int i=0; i<2; ++i) {
        SDL_Color c = (m_MenuSelection==i) ? SDL_Color{255,255,0,255} : SDL_Color{255,255,255,255};
        m_TextRenderer->RenderTextCentered(opts[i], GetWindowWidth()/2, y, c);
        y+=50;
    }
}

void PixelsGateGame::RenderMapScreen() {
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(GetRenderer(), 20, 20, 20, 230);
    SDL_Rect o = {0,0,GetWindowWidth(),GetWindowHeight()}; SDL_RenderFillRect(GetRenderer(), &o);
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
    
    int tabW = 150, startX = GetWindowWidth()/2 - tabW;
    SDL_Rect mapT = {startX, 20, tabW, 40};
    SDL_Rect jrnT = {startX+tabW, 20, tabW, 40};
    
    SDL_SetRenderDrawColor(GetRenderer(), (m_MapTab==0)?100:50, (m_MapTab==0)?100:50, (m_MapTab==0)?150:50, 255);
    SDL_RenderFillRect(GetRenderer(), &mapT);
    m_TextRenderer->RenderTextCentered("MAP", mapT.x+tabW/2, mapT.y+10, {255,255,255,255});
    
    SDL_SetRenderDrawColor(GetRenderer(), (m_MapTab==1)?100:50, (m_MapTab==1)?100:50, (m_MapTab==1)?150:50, 255);
    SDL_RenderFillRect(GetRenderer(), &jrnT);
    m_TextRenderer->RenderTextCentered("JOURNAL", jrnT.x+tabW/2, jrnT.y+10, {255,255,255,255});
    
    if (m_MapTab == 1) { // Journal
        auto &quests = GetRegistry().View<PixelsEngine::QuestComponent>();
        int qy = 160;
        for(auto &[e, q] : quests) {
            if(q.state > 0) {
                std::string status = (q.state == 2) ? " (Done)" : "";
                m_TextRenderer->RenderText("- " + q.questId + status, GetWindowWidth()/2 - 150, qy, {255,255,255,255});
                qy+=40;
            }
        }
    }
    else {
        // --- Full Map Render ---
        auto *currentMap = (m_State == GameState::Camp) ? m_CampLevel.get() : m_Level.get();
        if (currentMap) {
            int tileSize = 10;
            int mapW = currentMap->GetWidth() * tileSize;
            int mapH = currentMap->GetHeight() * tileSize;
            int startX = (GetWindowWidth() - mapW) / 2;
            int startY = (GetWindowHeight() - mapH) / 2 + 30;

            SDL_Rect mapRect = {startX - 5, startY - 5, mapW + 10, mapH + 10};
            SDL_SetRenderDrawColor(GetRenderer(), 50, 50, 50, 255);
            SDL_RenderFillRect(GetRenderer(), &mapRect);

            // Draw Tiles
            for (int y = 0; y < currentMap->GetHeight(); ++y) {
                for (int x = 0; x < currentMap->GetWidth(); ++x) {
                    if (!currentMap->IsExplored(x, y)) continue;
                    
                    int tile = currentMap->GetTile(x, y);
                    SDL_Color c = {0,0,0,255}; 
                    using namespace PixelsEngine::Tiles;
                    if (tile >= DIRT && tile <= DIRT_VARIANT_18) c = {139, 69, 19, 255};
                    else if (tile == DIRT_VARIANT_19 || tile == DIRT_WITH_PARTIAL_GRASS) c = {120, 80, 30, 255};
                    else if (tile >= GRASS && tile <= GRASS_BLOCK_FULL_VARIANT_01) c = {34, 139, 34, 255};
                    else if (tile >= GRASS_WITH_BUSH && tile <= GRASS_VARIANT_06) c = {30, 120, 30, 255};
                    else if (tile >= FLOWER && tile <= FLOWERS_WITHOUT_LEAVES) c = {200, 100, 200, 255};
                    else if (tile >= BUSH && tile <= BUSH_VARIANT_01) c = {20, 100, 20, 255};
                    else if (tile >= LOG && tile <= LOG_WITH_LEAVES_VARIANT_02) c = {100, 70, 20, 255};
                    else if (tile >= COBBLESTONE && tile <= SMOOTH_STONE) c = {150, 150, 150, 255};
                    else if (tile >= ROCK && tile <= ROCK_VARIANT_03) c = {105, 105, 105, 255};
                    else if (tile >= WATER_DROPLETS && tile <= WATER_VARIANT_01) c = {100, 149, 237, 255};
                    else if (tile >= DEEP_WATER && tile <= DEEP_WATER_VARIANT_07) c = {0, 0, 139, 255};
                    else if (tile >= OCEAN && tile <= OCEAN_ROUGH) c = {0, 0, 128, 255};
                    else if (tile >= STONES_ON_WATER && tile <= STONES_ON_WATER_VARIANT_11) c = {100, 149, 237, 255};
                    else if (tile == ROCK_ON_WATER || tile == SMOOTH_STONE_ON_WATER) c = {100, 149, 237, 255};

                    SDL_SetRenderDrawColor(GetRenderer(), c.r, c.g, c.b, c.a);
                    SDL_Rect r = {startX + x * tileSize, startY + y * tileSize, tileSize, tileSize};
                    SDL_RenderFillRect(GetRenderer(), &r);
                }
            }

            // Draw Player
            auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
            if (pTrans) {
                SDL_SetRenderDrawColor(GetRenderer(), 255, 255, 255, 255);
                SDL_Rect pr = {startX + (int)pTrans->x * tileSize, startY + (int)pTrans->y * tileSize, tileSize, tileSize};
                SDL_RenderFillRect(GetRenderer(), &pr);
            }
        }
    }
}

void PixelsGateGame::RenderCharacterMenu() {
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(GetRenderer(), 20, 20, 30, 230);
    SDL_Rect o = {0,0,GetWindowWidth(),GetWindowHeight()}; SDL_RenderFillRect(GetRenderer(), &o);
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
    
    int w=800, h=500;
    SDL_Rect p = {(GetWindowWidth()-w)/2, (GetWindowHeight()-h)/2, w, h};
    SDL_SetRenderDrawColor(GetRenderer(), 45,45,55,255); SDL_RenderFillRect(GetRenderer(), &p);
    
    // Tabs
    const char* tabs[] = {"INVENTORY", "CHARACTER", "SPELLBOOK"};
    int tw = w/3;
    for(int i=0; i<3; ++i) {
        SDL_Rect tr = {p.x + i*tw, p.y-40, tw, 40};
        SDL_SetRenderDrawColor(GetRenderer(), (m_CharacterTab==i)?60:35, (m_CharacterTab==i)?60:35, (m_CharacterTab==i)?80:45, 255);
        SDL_RenderFillRect(GetRenderer(), &tr);
        m_TextRenderer->RenderTextCentered(tabs[i], tr.x+tw/2, tr.y+20, {255,255,255,255});
    }
    
    if (m_CharacterTab == 0) {
        RenderInventory(); 
    } else if (m_CharacterTab == 1) {
        auto *s = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
        if(s) {
            m_TextRenderer->RenderText("Level " + std::to_string(s->level) + " " + s->characterClass, p.x+50, p.y+50, {255,255,0,255});
            
            auto DrawStat = [&](const std::string& name, int val, int y, SDL_Color c) {
                m_TextRenderer->RenderText(name + ": " + std::to_string(val) + " (" + std::to_string((val-10)/2) + ")", p.x+50, y, c);
            };
            DrawStat("STR", s->strength, p.y+100, {255,150,150,255});
            DrawStat("DEX", s->dexterity, p.y+140, {150,255,150,255});
            DrawStat("CON", s->constitution, p.y+180, {255,200,150,255});
            DrawStat("INT", s->intelligence, p.y+220, {150,200,255,255});
            DrawStat("WIS", s->wisdom, p.y+260, {255,255,255,255});
            DrawStat("CHA", s->charisma, p.y+300, {255,150,255,255});
            
            m_TextRenderer->RenderText("HP: " + std::to_string(s->currentHealth) + "/" + std::to_string(s->maxHealth), p.x+300, p.y+100, {255,50,50,255});
            m_TextRenderer->RenderText("XP: " + std::to_string(s->experience), p.x+300, p.y+140, {200,200,255,255});
        }
    } else if (m_CharacterTab == 2) {
        m_TextRenderer->RenderTextCentered("Known Spells", p.x+w/2, p.y+50, {200,100,255,255});
        std::string sp[] = {"Fireball", "Heal", "Magic Missile", "Shield"};
        std::string tooltipKeys[] = {"Fir", "Hel", "Mis", "Shd"};
        std::string desc[] = {"Deals fire damage in area.", "Restores HP.", "Force damage, never miss.", "Boosts AC."};
        
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        int y = p.y+100;
        
        for(int i=0; i<4; ++i) {
            SDL_Rect row = {p.x+50, y, w-100, 40};
            if(mx >= row.x && mx <= row.x + row.w && my >= row.y && my <= row.y + row.h) {
                SDL_SetRenderDrawColor(GetRenderer(), 70, 70, 90, 255);
                SDL_RenderFillRect(GetRenderer(), &row);
                m_HoveredItemName = tooltipKeys[i];
            }
            m_TextRenderer->RenderText(sp[i], p.x+60, y+10, {255,255,255,255});
            m_TextRenderer->RenderText(desc[i], p.x+250, y+10, {180,180,180,255});
            y+=50;
        }
    }
}

void PixelsGateGame::RenderTradeScreen() {
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(GetRenderer(), 20, 40, 20, 230);
    SDL_Rect overlay = {0,0,GetWindowWidth(),GetWindowHeight()}; SDL_RenderFillRect(GetRenderer(), &overlay);
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
    
    int w = GetWindowWidth(); int h = GetWindowHeight();
    m_TextRenderer->RenderTextCentered("TRADING", w/2, 50, {100,255,100,255});
    m_TextRenderer->RenderTextCentered("Double-Click to Buy/Sell", w/2, 80, {200,200,200,255});
    m_TextRenderer->RenderTextCentered("Press ESC to Close", w/2, h - 50, {150,150,150,255});

    auto *pInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    auto *tInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_TradingWith);

    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);

    // Player Section
    m_TextRenderer->RenderText("Your Items", 50, 100, {255,255,255,255});
    if (pInv) {
        m_TextRenderer->RenderText("Gold: " + std::to_string(pInv->GetGoldCount()), 50, 120, {255, 215, 0, 255});
        int y = 140;
        for (auto &it : pInv->items) {
            if (it.name == "Coins") continue;
            
            // Hover Animation
            if (mx >= 45 && mx <= 350 && my >= y && my <= y + 35) {
                SDL_Rect highlight = {45, y, 305, 35};
                SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(GetRenderer(), 255, 255, 255, 30);
                SDL_RenderFillRect(GetRenderer(), &highlight);
                SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
            }

            RenderInventoryItem(it, 50, y);
            // Increased spacing: Move price further right (from 250 to 300)
            m_TextRenderer->RenderTextSmall(std::to_string(it.value) + "G", 300, y + 8, {255, 215, 0, 255});
            y += 40;
        }
    }

    // Trader Section
    m_TextRenderer->RenderText("Trader Wares", w/2 + 50, 100, {255,255,255,255});
    if (tInv) {
        m_TextRenderer->RenderText("Gold: " + std::to_string(tInv->GetGoldCount()), w/2 + 50, 120, {255, 215, 0, 255});
        int y = 140;
        for (auto &it : tInv->items) {
            if (it.name == "Coins") continue;

            // Hover Animation
            if (mx >= w/2 + 45 && mx <= w/2 + 350 && my >= y && my <= y + 35) {
                SDL_Rect highlight = {w/2 + 45, y, 305, 35};
                SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(GetRenderer(), 255, 255, 255, 30);
                SDL_RenderFillRect(GetRenderer(), &highlight);
                SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
            }

            RenderInventoryItem(it, w/2 + 50, y);
            // Increased spacing: Move price further right
            m_TextRenderer->RenderTextSmall(std::to_string(it.value) + "G", w/2 + 300, y + 8, {255, 215, 0, 255});
            y += 40;
        }
    }
}

void PixelsGateGame::RenderKeybindSettings() {
    SDL_SetRenderDrawColor(GetRenderer(), 20, 20, 30, 255); SDL_RenderClear(GetRenderer());
    m_TextRenderer->RenderTextCentered("CONTROLS", GetWindowWidth()/2, 50, {255,255,0,255});
    m_TextRenderer->RenderTextCentered("Press ESC to Back", GetWindowWidth()/2, GetWindowHeight()-50, {150,150,150,255});
}

void PixelsGateGame::RenderLootScreen() {
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(GetRenderer(), 30, 30, 30, 230);
    SDL_Rect o = {0,0,GetWindowWidth(),GetWindowHeight()}; SDL_RenderFillRect(GetRenderer(), &o);
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
    
    SDL_Rect p = {(GetWindowWidth()-400)/2, (GetWindowHeight()-500)/2, 400, 500};
    SDL_SetRenderDrawColor(GetRenderer(), 60, 60, 60, 255); SDL_RenderFillRect(GetRenderer(), &p);
    SDL_SetRenderDrawColor(GetRenderer(), 200, 200, 200, 255); SDL_RenderDrawRect(GetRenderer(), &p);
    m_TextRenderer->RenderTextCentered("LOOT", p.x+200, p.y+30, {255,215,0,255});
    
    auto *loot = GetRegistry().GetComponent<PixelsEngine::LootComponent>(m_LootingEntity);
    m_TextRenderer->RenderTextCentered("Double-Click to Collect", p.x+200, p.y+60, {200,200,200,255});

    if(loot) {
        int y = p.y+80;
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        for(auto &it : loot->drops) {
            // Hover Detection
            if (mx >= p.x + 30 && mx <= p.x + 370 && my >= y && my <= y + 40) {
                SDL_Rect highlight = {p.x + 25, y, 350, 40};
                SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(GetRenderer(), 255, 255, 255, 40);
                SDL_RenderFillRect(GetRenderer(), &highlight);
                SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
                m_HoveredItemName = it.name;
            }

            RenderInventoryItem(it, p.x+30, y);
            y+=45;
        }
    }
}

void PixelsGateGame::RenderDialogueScreen() {
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(GetRenderer(), 0, 0, 0, 220); // More opaque
    SDL_Rect overlay = {0,0,GetWindowWidth(),GetWindowHeight()}; SDL_RenderFillRect(GetRenderer(), &overlay);
    
    SDL_Rect p = {(GetWindowWidth()-600)/2, GetWindowHeight()-350, 600, 250};
    SDL_SetRenderDrawColor(GetRenderer(), 40, 40, 45, 255); SDL_RenderFillRect(GetRenderer(), &p);
    SDL_SetRenderDrawColor(GetRenderer(), 150, 150, 160, 255); SDL_RenderDrawRect(GetRenderer(), &p);
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
    
    auto *d = GetRegistry().GetComponent<PixelsEngine::DialogueComponent>(m_DialogueWith);
    if(d && d->tree->nodes.find(d->tree->currentNodeId) != d->tree->nodes.end()) {
        auto &node = d->tree->nodes[d->tree->currentNodeId];
        m_TextRenderer->RenderText(d->tree->currentEntityName, p.x+20, p.y+20, {255,215,0,255});
        int th = m_TextRenderer->RenderTextWrapped(node.npcText, p.x+20, p.y+50, 560, {255,255,255,255});
        
        auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
        std::vector<PixelsEngine::DialogueOption*> visibleOptions;
        for (auto &opt : node.options) {
            bool visible = true;
            if (!opt.repeatable && opt.hasBeenChosen) visible = false;
            if (visible && !opt.requiredFlag.empty() && !m_WorldFlags[opt.requiredFlag]) visible = false;
            if (visible && !opt.excludeFlag.empty() && m_WorldFlags[opt.excludeFlag]) visible = false;
            if (visible && !opt.requiredItem.empty()) {
                bool hasItem = false;
                if (inv) {
                    for (auto &it : inv->items) if (it.name == opt.requiredItem) hasItem = true;
                }
                if (!hasItem) visible = false;
            }
            if (visible) visibleOptions.push_back(&opt);
        }

        int y = p.y + 50 + th + 20;
        int idx = 0;
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        for(auto *opt : visibleOptions) {
            std::string prefix = std::to_string(idx+1) + ". ";
            
            // Hover Detection
            bool hovered = (mx >= p.x + 20 && mx <= p.x + 580 && my >= y && my <= y + 25);
            if (hovered) {
                m_MenuSelection = idx; // Link to menu navigation selection
            }

            SDL_Color c = (m_DialogueSelection == idx) ? SDL_Color{50,255,50,255} : SDL_Color{200,200,200,255};
            m_TextRenderer->RenderText(prefix + opt->text, p.x+20, y, c);
            y += 25; idx++;
        }
    } else if (d) {
        m_TextRenderer->RenderTextCentered("Dialogue Error: Node '" + d->tree->currentNodeId + "' not found.", GetWindowWidth()/2, GetWindowHeight()/2, {255,0,0,255});
    }
}

void DrawCircleOutline(SDL_Renderer* renderer, int centerX, int centerY, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (float angle = 0; angle < 6.28f; angle += 0.05f) {
        int x1 = centerX + (int)(cos(angle) * radius);
        int y1 = centerY + (int)(sin(angle) * radius);
        int x2 = centerX + (int)(cos(angle + 0.05f) * radius);
        int y2 = centerY + (int)(sin(angle + 0.05f) * radius);
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
}

void PixelsGateGame::RenderOverlays() {
    auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    if (!pTrans) return;

    auto *currentMap = (m_State == GameState::Camp) ? m_CampLevel.get() : m_Level.get();
    auto &camera = GetCamera();
    int px, py;
    currentMap->GridToScreen(pTrans->x, pTrans->y, px, py);
    px = px - (int)camera.x + 16;
    py = py - (int)camera.y + 8;

    int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
    bool shift = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_LSHIFT) || PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RSHIFT);

    // Circles and Lines based on state
    if (m_State == GameState::TargetingJump) {
        DrawCircleOutline(GetRenderer(), px, py, (int)(3.0f * 32.0f), {255, 255, 255, 255}); // White circle 3M
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        int gx, gy; currentMap->ScreenToGrid(mx + camera.x, my + camera.y, gx, gy);
        float gridDist = std::sqrt(std::pow(pTrans->x - gx, 2) + std::pow(pTrans->y - gy, 2));

        SDL_Color lineColor = (gridDist <= 3.0f) ? SDL_Color{255, 255, 255, 255} : SDL_Color{255, 0, 0, 255};
        SDL_SetRenderDrawColor(GetRenderer(), lineColor.r, lineColor.g, lineColor.b, lineColor.a);
        SDL_RenderDrawLine(GetRenderer(), px, py, mx, my);
    } 
    else if (m_State == GameState::Targeting) {
        DrawCircleOutline(GetRenderer(), px, py, (int)(6.0f * 32.0f), {200, 100, 255, 255}); // Purple circle
    }
    else if (m_State == GameState::TargetingDash) {
        int mx, my; PixelsEngine::Input::GetMousePosition(mx, my);
        int gx, gy; currentMap->ScreenToGrid(mx + camera.x, my + camera.y, gx, gy);
        float gridDist = std::sqrt(std::pow(pTrans->x - gx, 2) + std::pow(pTrans->y - gy, 2));
        SDL_Color lineColor = (gridDist <= 5.0f) ? SDL_Color{255, 255, 255, 255} : SDL_Color{255, 0, 0, 255};
        SDL_SetRenderDrawColor(GetRenderer(), lineColor.r, lineColor.g, lineColor.b, lineColor.a);
        SDL_RenderDrawLine(GetRenderer(), px, py, mx, my);
    }
    
    if (shift) {
        float range = 1.5f; // Fists/Melee default
        auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
        if (inv) {
             if (m_SelectedWeaponSlot == 0 && !inv->equippedMelee.IsEmpty()) range = 3.0f;
             else if (m_SelectedWeaponSlot == 1 && !inv->equippedRanged.IsEmpty()) range = 10.0f;
        }
        DrawCircleOutline(GetRenderer(), px, py, (int)(range * 32.0f), {255, 0, 0, 255}); // Red circle
        m_TextRenderer->RenderTextCentered("ATTACK MODE", GetWindowWidth() / 2, 100, {255, 0, 0, 255});
    }

    // Target Highlight Circles
    if (m_State == GameState::Targeting || m_State == GameState::TargetingShove || shift) {
        PixelsEngine::Entity hovered = GetEntityAtMouse();
        
        auto &transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
        for (auto &[ent, t] : transforms) {
            if (ent == m_Player) continue;
            
            // Only draw for things that have stats (can be attacked/targeted) or interaction
            if (!GetRegistry().HasComponent<PixelsEngine::StatsComponent>(ent) && 
                !GetRegistry().HasComponent<PixelsEngine::InteractionComponent>(ent)) continue;

            auto *stats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(ent);
            if (stats && stats->isDead) continue;

            int tx, ty;
            currentMap->GridToScreen(t.x, t.y, tx, ty);
            tx = tx - (int)camera.x + 16;
            ty = ty - (int)camera.y + 8;

            bool isHovered = (ent == hovered);
            SDL_Color circleColor = {255, 0, 0, (Uint8)(isHovered ? 255 : 150)};
            DrawCircleOutline(GetRenderer(), tx, ty, isHovered ? 22 : 20, circleColor);
            if (isHovered) {
                DrawCircleOutline(GetRenderer(), tx, ty, 21, circleColor); // Thicker if hovered
            }
        }
    }
}

void PixelsGateGame::RenderRestMenu() {
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(GetRenderer(), 0, 0, 0, 180);
    SDL_Rect o = {0,0,GetWindowWidth(),GetWindowHeight()}; SDL_RenderFillRect(GetRenderer(), &o);
    SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
    
    SDL_Rect b = {(GetWindowWidth()-400)/2, (GetWindowHeight()-250)/2, 400, 250};
    SDL_SetRenderDrawColor(GetRenderer(), 40, 30, 20, 255); SDL_RenderFillRect(GetRenderer(), &b);
    m_TextRenderer->RenderTextCentered("REST MENU", b.x+200, b.y+30, {255,215,0,255});
    
    std::string campOpt = (m_ReturnState == GameState::Camp) ? "Leave Camp" : "Go to Camp";
    std::string opts[] = {"Short Rest", "Long Rest", campOpt, "Back"};
    int y = b.y+80;
    for(int i=0; i<4; ++i) {
        SDL_Color c = (m_MenuSelection==i) ? SDL_Color{255,255,0,255} : SDL_Color{255,255,255,255};
        m_TextRenderer->RenderTextCentered(opts[i], b.x+200, y, c);
        y+=40;
    }
}