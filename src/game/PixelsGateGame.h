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

#include "GameComponents.h"

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
  void PerformAttack(
      PixelsEngine::Entity forcedTarget = PixelsEngine::INVALID_ENTITY);

  void CreateBoar(float x, float y);

  // Character Creation
  void InitCharacterCreation();
  void RenderCharacterCreation();
  void HandleCreationInput();

  // Dice & Actions
  void StartDiceRoll(int modifier, int dc, const std::string &skill,
                     PixelsEngine::Entity target,
                     PixelsEngine::ContextActionType type);
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
    CharacterMenu,
    Targeting,
    Trading,
    KeybindSettings,
    Looting,
    TargetingJump,
    TargetingShove,
    Dialogue,
    Loading,
    RestMenu,
    Camp
  };
  GameState m_State = GameState::MainMenu;
  GameState m_ReturnState = GameState::Playing;

  PixelsEngine::TransformComponent
      m_LastWorldPos; // Position before going to camp

  int m_CharacterTab = 0; // 0: Inventory, 1: Character, 2: Spellbook

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
  std::unique_ptr<PixelsEngine::Tilemap> m_CampLevel;
  std::unique_ptr<PixelsEngine::TextRenderer> m_TextRenderer;
  PixelsEngine::Entity m_Player;
  PixelsEngine::Entity m_SelectedNPC =
      PixelsEngine::INVALID_ENTITY; // For walking to NPC

  PixelsEngine::ContextMenu m_ContextMenu;
  PixelsEngine::DiceRollAnimation m_DiceRoll;

  // --- Components ---
  CombatManager m_Combat;
  TimeManager m_Time;
  FloatingTextManager m_FloatingText;

  std::vector<std::pair<int, int>> m_CurrentAIPath;
  int m_CurrentAIPathIndex = -1;

  void StartCombat(PixelsEngine::Entity enemy);
  void EndCombat();
  void NextTurn();
  void UpdateCombat(float deltaTime);
  void RenderCombatUI();
  void HandleCombatInput();

  // Menu Navigation
  int m_MenuSelection = 0;
  int m_MapTab = 0;         // 0: Map, 1: Journal
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
  std::future<void> m_LoadFuture;

  // Menu Renderers
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

  // Menu Input Handlers
  void HandleMainMenuInput();
  void HandlePauseMenuInput();
  void HandleGameOverInput();
  void HandleMapInput();
  void HandleCharacterMenuInput();
  void HandleTargetingInput();
  void HandleTargetingJumpInput();
  void HandleTargetingShoveInput();
  void HandleTradeInput();
  void HandleKeybindInput();
  void HandleLootInput();
  void HandleDialogueInput();
  void HandleRestMenuInput();
  void CastSpell(const std::string &spellName, PixelsEngine::Entity target);
  bool IsInTurnOrder(PixelsEngine::Entity entity);
  void HandleMenuNavigation(int numOptions, std::function<void(int)> onSelect,
                            std::function<void()> onCancel = nullptr,
                            int forceSelection = -1);

  void InitCampMap();
  void SpawnWorldEntities();
  void ShowSaveMessage() { m_SaveMessageTimer = 2.0f; }
  void TriggerLoadTransition(const std::string &filename) {
    m_PendingLoadFile = filename;
    m_FadeState = FadeState::FadingOut;
    m_FadeTimer = m_FadeDuration;
  }

  // --- New Features ---
  float m_EnvironmentDamageTimer = 0.0f;

  void UpdateAI(float deltaTime);
  void UpdateDayNight(float deltaTime);
  void RenderDayNightCycle();
  void RenderEnemyCones(const PixelsEngine::Camera &camera);
  void SpawnFloatingText(float x, float y, const std::string &text,
                         SDL_Color color);
  void SpawnLootBag(float x, float y,
                    const std::vector<PixelsEngine::Item> &items);

  // Inventory State
  int m_DraggingItemIndex = -1;
  float m_LastClickTime = 0.0f;
  int m_LastClickedItemIndex = -1;

  // Inventory UI Helpers
  void HandleInventoryInput();
  void RenderInventoryItem(const PixelsEngine::Item &item, int x, int y);
  void UseItem(const std::string &itemName);
  std::vector<std::string> GetHotbarItems();

  struct TooltipData {
    std::string name;
    std::string description;
    std::string cost;   // "Action", "Bonus Action", "1st Level Slot", etc.
    std::string range;  // "Melee", "18m", etc.
    std::string effect; // "1d8 + 3 Slashing", "2d4 + 2 Healing", etc.
    std::string save;   // "None", "STR", "DEX", etc.
  };
  std::unordered_map<std::string, TooltipData> m_Tooltips;
  std::string m_HoveredItemName = "";
  bool m_TooltipPinned = false;
  int m_PinnedTooltipX = 0;
  int m_PinnedTooltipY = 0;

  void RenderTooltip(const TooltipData &data, int x, int y);
};
