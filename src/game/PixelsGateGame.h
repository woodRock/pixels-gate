#pragma once
#include "../engine/Application.h"
#include "../engine/Texture.h"
#include "../engine/Tilemap.h"
#include "../engine/ECS.h"
#include "../engine/TextRenderer.h"
#include "../engine/UIComponents.h"
#include "../engine/Inventory.h"
#include "../engine/Config.h"
#include <memory>

class PixelsGateGame : public PixelsEngine::Application {
public:
    PixelsGateGame();
    virtual ~PixelsGateGame() = default;

protected:
    void OnStart() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;

    void RenderHUD();
    void RenderInventory();
    void RenderContextMenu();
    void RenderDiceRoll();
    void HandleInput();
    void CheckUIInteraction(int mx, int my);
    void CheckWorldInteraction(int mx, int my);
    void PerformAttack(PixelsEngine::Entity forcedTarget = PixelsEngine::INVALID_ENTITY);
    
    void CreateBoar(float x, float y);

    // Character Creation
    void InitCharacterCreation();
    void RenderCharacterCreation();
    void HandleCreationInput();

    // Dice & Actions
    void StartDiceRoll(int modifier, int dc, const std::string& skill, PixelsEngine::Entity target, PixelsEngine::ContextActionType type);
    void ResolveDiceRoll();

private:
    enum class GameState {
        MainMenu,
        Creation,
        Playing,
        Combat,
        Paused,
        Options,
        Credits,
        Controls,
        GameOver,
        Map,
        Character,
        Magic,
        Targeting,
        Trading,
        KeybindSettings,
        Looting,
        TargetingJump,
        TargetingShove,
        Dialogue
    };
    GameState m_State = GameState::MainMenu;
    GameState m_ReturnState = GameState::Playing; 

    PixelsEngine::Entity m_TradingWith = PixelsEngine::INVALID_ENTITY;
    PixelsEngine::Entity m_LootingEntity = PixelsEngine::INVALID_ENTITY;
    PixelsEngine::Entity m_DialogueWith = PixelsEngine::INVALID_ENTITY;
    
    std::unordered_map<std::string, bool> m_WorldFlags; // Persisted choices
    int m_DialogueSelection = 0;

    PixelsEngine::GameAction m_BindingAction = PixelsEngine::GameAction::Pause;
    bool m_IsWaitingForKey = false;

    std::string m_PendingSpellName = "";

    int m_SelectedWeaponSlot = 0; // 0: Melee, 1: Ranged

    std::unique_ptr<PixelsEngine::Tilemap> m_Level;
    std::unique_ptr<PixelsEngine::TextRenderer> m_TextRenderer;
    PixelsEngine::Entity m_Player;
    PixelsEngine::Entity m_SelectedNPC = PixelsEngine::INVALID_ENTITY; // For walking to NPC
    
    PixelsEngine::ContextMenu m_ContextMenu;
    PixelsEngine::DiceRollAnimation m_DiceRoll;

    // Combat System
    struct CombatTurn {
        PixelsEngine::Entity entity;
        int initiative;
        bool isPlayer;
        bool isSurprised = false;
    };
    std::vector<CombatTurn> m_TurnOrder;
    int m_CurrentTurnIndex = -1;
    int m_ActionsLeft = 0;
    int m_BonusActionsLeft = 0;
    float m_MovementLeft = 0.0f;
    float m_CombatTurnTimer = 0.0f; // AI delay

    void StartCombat(PixelsEngine::Entity enemy);
    void EndCombat();
    void NextTurn();
    void UpdateCombat(float deltaTime);
    void RenderCombatUI();
    void HandleCombatInput();

    // Menu Navigation
    int m_MenuSelection = 0;
    int m_MapTab = 0; // 0: Map, 1: Journal
    float m_MenuTimer = 0.0f; // for debouncing or animations

    // Visual Feedback
    float m_SaveMessageTimer = 0.0f;
    float m_FadeTimer = 0.0f;
    const float m_FadeDuration = 0.5f;

    enum class FadeState {
        None,
        FadingOut, // Screen going black
        FadingIn   // Screen revealing game
    };
    FadeState m_FadeState = FadeState::None;
    std::string m_PendingLoadFile;

    // Menu Renderers
    void RenderMainMenu();
    void RenderPauseMenu();
    void RenderOptions();
    void RenderCredits();
    void RenderControls();
    void RenderGameOver();
    void RenderMapScreen();
    void RenderCharacterScreen();
    void RenderMagicScreen();
    void RenderTradeScreen();
    void RenderKeybindSettings();
    void RenderLootScreen();
    void RenderDialogueScreen();

    // Menu Input Handlers
    void HandleMainMenuInput();
    void HandlePauseMenuInput();
    void HandleGameOverInput();
    void HandleMapInput();
    void HandleCharacterInput();
    void HandleMagicInput();
    void HandleTargetingInput();
    void HandleTargetingJumpInput();
    void HandleTargetingShoveInput();
    void HandleTradeInput();
    void HandleKeybindInput();
    void HandleLootInput();
    void HandleDialogueInput();
    void CastSpell(const std::string& spellName, PixelsEngine::Entity target);
    bool IsInTurnOrder(PixelsEngine::Entity entity);
    void HandleMenuNavigation(int numOptions, std::function<void(int)> onSelect, std::function<void()> onCancel = nullptr, int forceSelection = -1);

    void ShowSaveMessage() { m_SaveMessageTimer = 2.0f; }
    void TriggerLoadTransition(const std::string& filename) { 
        m_PendingLoadFile = filename; 
        m_FadeState = FadeState::FadingOut; 
        m_FadeTimer = m_FadeDuration; 
    }

    // --- New Features ---
    float m_EnvironmentDamageTimer = 0.0f;
    struct FloatingText {
        float x, y;
        std::string text;
        float life;
        SDL_Color color;
    };
    std::vector<FloatingText> m_FloatingTexts;
    
    // Day/Night Cycle (0.0 to 24.0, starts at 8.0)
    float m_TimeOfDay = 8.0f; 
    const float m_TimeSpeed = 0.1f; // Reduced from 1.0f to slow down the cycle

    void UpdateAI(float deltaTime);
    void UpdateDayNight(float deltaTime);
    void RenderDayNightCycle();
    void RenderEnemyCones(const PixelsEngine::Camera& camera);
    void SpawnFloatingText(float x, float y, const std::string& text, SDL_Color color);
    void SpawnLootBag(float x, float y, const std::vector<PixelsEngine::Item>& items);
    
    // Inventory State
    int m_DraggingItemIndex = -1;
    float m_LastClickTime = 0.0f;
    int m_LastClickedItemIndex = -1;

    // Inventory UI Helpers
    void HandleInventoryInput();
    void RenderInventoryItem(const PixelsEngine::Item& item, int x, int y);
};
