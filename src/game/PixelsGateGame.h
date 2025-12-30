#pragma once
#include "../engine/Application.h"
#include "../engine/Texture.h"
#include "../engine/Tilemap.h"
#include "../engine/ECS.h"
#include "../engine/TextRenderer.h"
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
    void HandleInput();
    void CheckUIInteraction(int mx, int my);
    void CheckWorldInteraction(int mx, int my);
    void PerformAttack();

private:
    std::unique_ptr<PixelsEngine::Tilemap> m_Level;
    std::unique_ptr<PixelsEngine::TextRenderer> m_TextRenderer;
    PixelsEngine::Entity m_Player;
    PixelsEngine::Entity m_SelectedNPC = PixelsEngine::INVALID_ENTITY; // For walking to NPC
};
