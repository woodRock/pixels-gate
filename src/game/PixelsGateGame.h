#pragma once

#include "../engine/Application.h"
#include "../engine/Components.h"
#include "../engine/Config.h"
#include "../engine/ECS.h"
#include "../engine/Inventory.h"
#include "../engine/TextRenderer.h"
#include "../engine/Texture.h"
#include "../engine/Tilemap.h"
#include "../engine/UIComponents.h"
#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <SDL2/SDL.h>

// ... (Keep CombatManager, TimeManager, FloatingTextManager classes as they were) ...
class CombatManager {
public:
    struct CombatTurn {
        PixelsEngine::Entity entity;
        int initiative;
        bool isPlayer;
        bool isSurprised = false;
    };
    CombatManager() = default;
    void Reset() {
        m_TurnOrder.clear();
        m_CurrentTurnIndex = -1;
        m_ActionsLeft = 0;
        m_BonusActionsLeft = 0;
        m_MovementLeft = 0.0f;
        m_CombatTurnTimer = 0.0f;
    }
    std::vector<CombatTurn> m_TurnOrder;
    int m_CurrentTurnIndex = -1;
    int m_ActionsLeft = 0;
    int m_BonusActionsLeft = 0;
    float m_MovementLeft = 0.0f;
    float m_CombatTurnTimer = 0.0f; 
    bool IsCombatActive() const { return !m_TurnOrder.empty(); }
};

class TimeManager {
public:
    float m_TimeOfDay = 8.0f;
    const float m_TimeSpeed = 0.1f;
    void Update(float deltaTime) {
        m_TimeOfDay += m_TimeSpeed * deltaTime;
        if (m_TimeOfDay >= 24.0f) m_TimeOfDay -= 24.0f;
    }
};

class FloatingTextManager {
public:
    struct FloatingText {
        float x, y;
        std::string text;
        float life;
        SDL_Color color;
    };
    std::vector<FloatingText> m_Texts;
    void Spawn(float x, float y, const std::string &text, SDL_Color color) {
        m_Texts.push_back({x, y, text, 1.5f, color});
    }
    void Update(float deltaTime) {
        for (auto &ft : m_Texts) {
            ft.y -= 20.0f * deltaTime;
            ft.life -= deltaTime;
        }
        m_Texts.erase(std::remove_if(m_Texts.begin(), m_Texts.end(),
            [](const FloatingText &ft) { return ft.life <= 0.0f; }),
            m_Texts.end());
    }
};

class PixelsGateGame : public PixelsEngine::Application {
public:
    PixelsGateGame();
    virtual ~PixelsGateGame() = default;

protected:
    void OnStart() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;

public:
    // UI
    void RenderHUD();
    void RenderInventory();
    void RenderContextMenu();
    void RenderDiceRoll();
    void RenderOverlays();
    void RenderMainMenu();
    void RenderPauseMenu();
    void RenderOptions();
    void RenderCredits();
    void RenderControls();
    void RenderGameOver();
    void RenderMapScreen();
    void RenderCharacterMenu();
    void RenderTradeScreen();
    void RenderKeybindSettings();
    void RenderLootScreen();
    void RenderDialogueScreen();
    void RenderRestMenu();
    void RenderCharacterCreation();
    void InitCharacterCreation();

    struct TooltipData { std::string name, description, cost, range, effect, save; };
    void RenderTooltip(const TooltipData &data, int x, int y);
    void RenderInventoryItem(const PixelsEngine::Item &item, int x, int y);

    // Input
    void HandleInput();
    void HandleCreationInput();
    void HandleMainMenuInput();
    void HandlePauseMenuInput();
    void HandleOptionsInput();
    void HandleGameOverInput();
    void HandleMapInput();
    void HandleCharacterMenuInput();
    void HandleTargetingInput();
    void HandleTargetingJumpInput();
    void HandleTargetingShoveInput();
    void HandleTargetingDashInput();
    void HandleTradeInput();
    void HandleKeybindInput();
    void HandleLootInput();
    void HandleDialogueInput();
    void HandleRestMenuInput();
    void HandleCombatInput(PixelsEngine::Entity activeEntity);
    void HandleInventoryInput();
    
    void CheckUIInteraction(int mx, int my);
    void CheckWorldInteraction(int mx, int my);
    void HandleMenuNavigation(int numOptions, std::function<void(int)> onSelect,
                              std::function<void()> onCancel = nullptr, int forceSelection = -1);

    // Combat
    void StartCombat(PixelsEngine::Entity enemy);
    void EndCombat();
    void NextTurn();
    void UpdateCombat(float deltaTime);
    void RenderCombatUI();
    void RenderEnemyCones(const PixelsEngine::Camera &camera);
    void PerformAttack(PixelsEngine::Entity forcedTarget = PixelsEngine::INVALID_ENTITY);
    void CastSpell(const std::string &spellName, PixelsEngine::Entity target);
    void PerformJump(int targetX, int targetY);
    void PerformShove(PixelsEngine::Entity target);
    void PerformDash(int targetX, int targetY);
    bool IsInTurnOrder(PixelsEngine::Entity entity);
    PixelsEngine::Entity GetEntityAtMouse();

    // World
    void InitCampMap();
    void GenerateMainLevelTerrain();
    void SpawnWorldEntities();
    void CreateBoar(float x, float y);
    void CreateWolf(float x, float y);
    void CreateStag(float x, float y);
    void CreateBadger(float x, float y);
    void CreateWolfBoss(float x, float y);
    void CreateDeadManAndSon(float x, float y);
    void ResetGame(); // NEW Helper

    // Systems
    void UpdateAI(float deltaTime);
    void UpdateMovement(float deltaTime);
    void UpdateAnimations(float deltaTime);
    void UpdateDayNight(float deltaTime);
    void RenderDayNightCycle();
    void SpawnFloatingText(float x, float y, const std::string &text, SDL_Color color);
    void SpawnLootBag(float x, float y, const std::vector<PixelsEngine::Item> &items);
    void PickupItem(PixelsEngine::Entity entity);
    void UseItem(const std::string &itemName);
    std::vector<std::string> GetHotbarItems();
    void StartDiceRoll(int modifier, int dc, const std::string &skill,
                       PixelsEngine::Entity target, PixelsEngine::ContextActionType type);
    void ResolveDiceRoll();
    void ShowSaveMessage() { m_SaveMessageTimer = 2.0f; }
    void TriggerLoadTransition(const std::string &filename);
    PixelsEngine::Tilemap* GetCurrentMap();

private:
    enum class GameState {
        MainMenu, Creation, Playing, Combat, Paused, Options, Credits, Controls,
        GameOver, Map, CharacterMenu, Targeting, Trading, KeybindSettings, Looting,
        TargetingJump, TargetingShove, TargetingDash, Dialogue, Loading, RestMenu, Camp
    };

    GameState m_State = GameState::MainMenu;
    GameState m_ReturnState = GameState::Playing;
    GameState m_PauseOriginalState = GameState::Playing;

    PixelsEngine::TransformComponent m_LastWorldPos;
    int m_CharacterTab = 0; 
    
    PixelsEngine::Entity m_TradingWith = PixelsEngine::INVALID_ENTITY;
    PixelsEngine::Entity m_LootingEntity = PixelsEngine::INVALID_ENTITY;
    PixelsEngine::Entity m_DialogueWith = PixelsEngine::INVALID_ENTITY;
    
    std::unordered_map<std::string, bool> m_WorldFlags;
    int m_DialogueSelection = 0;
    
    PixelsEngine::GameAction m_BindingAction = PixelsEngine::GameAction::Pause;
    bool m_IsWaitingForKey = false;
    std::string m_PendingSpellName = "";
    int m_SelectedWeaponSlot = 0;

    std::unique_ptr<PixelsEngine::Tilemap> m_Level;
    std::unique_ptr<PixelsEngine::Tilemap> m_CampLevel;
    std::unique_ptr<PixelsEngine::TextRenderer> m_TextRenderer;
    
    PixelsEngine::Entity m_Player;
    PixelsEngine::Entity m_SelectedNPC = PixelsEngine::INVALID_ENTITY;

    PixelsEngine::ContextMenu m_ContextMenu;
    PixelsEngine::DiceRollAnimation m_DiceRoll;

    CombatManager m_Combat;
    TimeManager m_Time;
    FloatingTextManager m_FloatingText;

    std::vector<std::pair<int, int>> m_CurrentAIPath;
    int m_CurrentAIPathIndex = -1;

    int m_MenuSelection = 0;
    int m_MapTab = 0;
    float m_MenuTimer = 0.0f;
    float m_SaveMessageTimer = 0.0f;
    float m_FadeTimer = 0.0f;
    const float m_FadeDuration = 0.5f;

    enum class FadeState { None, FadingOut, FadingIn };
    FadeState m_FadeState = FadeState::None;
    std::string m_PendingLoadFile;
    bool m_LoadedIsCamp = false;
    std::future<void> m_LoadFuture;
    float m_EnvironmentDamageTimer = 0.0f;

    int m_DraggingItemIndex = -1;
    float m_LastClickTime = 0.0f;
    int m_LastClickedItemIndex = -1;

    struct DelayedSound {
        std::string path;
        float delay;
    };
    std::vector<DelayedSound> m_DelayedSounds;

    std::unordered_map<std::string, TooltipData> m_Tooltips;
    std::string m_HoveredItemName = "";
    bool m_TooltipPinned = false;
    int m_PinnedTooltipX = 0;
    int m_PinnedTooltipY = 0;

    float m_StateTimer = 0.0f;

    // --- Character Creation State (Moved from Static) ---
    int m_CC_TempStats[6] = {10, 10, 10, 10, 10, 10};
    int m_CC_ClassIndex = 0;
    int m_CC_RaceIndex = 0;
    int m_CC_SelectionIndex = 0;
    int m_CC_PointsRemaining = 5;
    const std::vector<std::string> m_CC_Classes = {"Fighter", "Rogue", "Wizard", "Cleric"};
    const std::vector<std::string> m_CC_Races = {"Human", "Elf", "Dwarf", "Halfling"};
};