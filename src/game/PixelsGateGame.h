#pragma once
#include "../engine/Application.h"
#include "../engine/Texture.h"
#include "../engine/Tilemap.h"
#include "../engine/ECS.h"
#include "../engine/TextRenderer.h"
#include "../engine/UIComponents.h"
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
    void PerformAttack();
    
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
        Paused,
        Options,
        Credits,
        Controls
    };
    GameState m_State = GameState::MainMenu;

    std::unique_ptr<PixelsEngine::Tilemap> m_Level;
    std::unique_ptr<PixelsEngine::TextRenderer> m_TextRenderer;
    PixelsEngine::Entity m_Player;
    PixelsEngine::Entity m_SelectedNPC = PixelsEngine::INVALID_ENTITY; // For walking to NPC
    
    PixelsEngine::ContextMenu m_ContextMenu;
    PixelsEngine::DiceRollAnimation m_DiceRoll;

    // Menu Navigation
    int m_MenuSelection = 0;
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

    // Menu Input Handlers
    void HandleMainMenuInput();
    void HandlePauseMenuInput();
    void HandleMenuNavigation(int numOptions, std::function<void(int)> onSelect, std::function<void()> onCancel = nullptr, int forceSelection = -1);

    void ShowSaveMessage() { m_SaveMessageTimer = 2.0f; }
    void TriggerLoadTransition(const std::string& filename) { 
        m_PendingLoadFile = filename; 
        m_FadeState = FadeState::FadingOut; 
        m_FadeTimer = m_FadeDuration; 
    }
};
