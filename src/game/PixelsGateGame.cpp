#include "PixelsGateGame.h"
#include "../engine/Dice.h"
#include "../engine/SaveSystem.h"
#include "../engine/Input.h"
#include <iostream>

PixelsGateGame::PixelsGateGame()
    : PixelsEngine::Application("Pixels Gate", 800, 600) {
  PixelsEngine::Dice::Init();
}

void PixelsGateGame::OnStart() {
  PixelsEngine::Config::Init();
  m_TextRenderer = std::make_unique<PixelsEngine::TextRenderer>(
      GetRenderer(), "assets/font.ttf", 16);

  // Initialize Tooltips
  m_Tooltips["Atk"] = {"Attack", "Strike an enemy.", "Action", "Melee/Ranged", "Weapon Dmg", "None"};
  m_Tooltips["Jmp"] = {"Jump", "Jump to target.", "Bonus Action + 3m", "5m", "Utility", "None"};
  m_Tooltips["Snk"] = {"Sneak", "Hide from enemies.", "Action", "Self", "Stealth", "None"};
  m_Tooltips["Shv"] = {"Shove", "Push creature.", "Bonus Action", "Melee", "1.5m Push", "ATH/ACR"};
  m_Tooltips["Dsh"] = {"Dash", "Double movement.", "Action", "Self", "Speed x2", "None"};
  m_Tooltips["End"] = {"End Turn", "End combat turn.", "None", "N/A", "None", "None"};
  m_Tooltips["Fir"] = {"Fireball", "Explosion of flame.", "Action", "18m", "8d6 Fire", "DEX Save"};
  m_Tooltips["Hel"] = {"Heal", "Regain hit points.", "Action", "Melee", "1d8 + Mod", "None"};
  m_Tooltips["Mis"] = {"Magic Missile", "3 darts of force.", "Action", "18m", "3x (1d4+1)", "None"};
  m_Tooltips["Shd"] = {"Shield", "+5 AC reaction.", "Reaction", "Self", "+5 AC", "None"};
  m_Tooltips["Potion"] = {"Healing Potion", "Heal 2d4+2 HP.", "Bonus Action", "Self", "Healing", "None"};
  m_Tooltips["Bread"] = {"Bread", "Heal 5 HP.", "Bonus Action", "Self", "Healing", "None"};
  m_Tooltips["Thieves' Tools"] = {"Thieves' Tools", "Pick locks.", "Action", "Melee", "Utility", "DEX Check"};
  
  InitCampMap();
  GenerateMainLevelTerrain();
  SpawnWorldEntities();
}

void PixelsGateGame::OnUpdate(float deltaTime) {
  if (m_State == GameState::Creation) {
    HandleCreationInput();
    return;
  }

  if (m_State == GameState::Loading) {
    if (m_LoadFuture.valid() && m_LoadFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      m_LoadFuture.get();
      m_FadeState = FadeState::FadingIn;
      m_FadeTimer = m_FadeDuration;
      m_State = m_LoadedIsCamp ? GameState::Camp : GameState::Playing;
      
      auto *pStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
      if (pStats && pStats->currentHealth > 0) pStats->isDead = false;
    }
    return;
  }

  if (m_SaveMessageTimer > 0.0f) m_SaveMessageTimer -= deltaTime;

  if (m_DiceRoll.active) {
      if (!m_DiceRoll.resultShown) {
          m_DiceRoll.timer += deltaTime;
          if (m_DiceRoll.timer >= m_DiceRoll.duration) {
              m_DiceRoll.resultShown = true;
              ResolveDiceRoll();
          }
      } else {
          if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
              m_DiceRoll.active = false;
              // Transition to success/fail node for dialogue
              if (m_DiceRoll.actionType == PixelsEngine::ContextActionType::Talk) {
                  auto *d = GetRegistry().GetComponent<PixelsEngine::DialogueComponent>(m_DiceRoll.target);
                  if (d) {
                      auto &node = d->tree->nodes[d->tree->currentNodeId];
                      for (auto &opt : node.options) {
                          if (opt.requiredStat == m_DiceRoll.skillName && opt.dc == m_DiceRoll.dc) {
                              d->tree->currentNodeId = m_DiceRoll.success ? opt.onSuccessNodeId : opt.onFailureNodeId;
                              m_DialogueSelection = 0;
                              m_MenuSelection = 0;
                              break;
                          }
                      }
                  }
              }
          }
      }
  }

  auto &hazards = GetRegistry().View<PixelsEngine::HazardComponent>();
  std::vector<PixelsEngine::Entity> hazardsDestroy;
  for (auto &[hEnt, haz] : hazards) {
      haz.duration -= deltaTime;
      if (haz.duration <= 0.0f) { hazardsDestroy.push_back(hEnt); continue; }
      haz.tickTimer += deltaTime;
      if (haz.tickTimer >= 1.0f) {
          haz.tickTimer = 0.0f;
          auto *hTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(hEnt);
          if (hTrans) {
              auto &victims = GetRegistry().View<PixelsEngine::StatsComponent>();
              for (auto &[vEnt, vStats] : victims) {
                  if (vStats.isDead) continue;
                  auto *vTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(vEnt);
                  if (vTrans) {
                      float d = std::sqrt(std::pow(vTrans->x - hTrans->x, 2) + std::pow(vTrans->y - hTrans->y, 2));
                      if (d < 1.5f) {
                          vStats.currentHealth -= haz.damage;
                          SpawnFloatingText(vTrans->x, vTrans->y, "-" + std::to_string(haz.damage) + " (Fire)", {255, 100, 0, 255});
                          if (vStats.currentHealth <= 0) vStats.isDead = true;
                      }
                  }
              }
          }
      }
  }
  for (auto h : hazardsDestroy) GetRegistry().DestroyEntity(h);

  UpdateDayNight(deltaTime);
  m_FloatingText.Update(deltaTime);

  if (m_FadeState != FadeState::None) {
      m_FadeTimer -= deltaTime;
      if (m_FadeState == FadeState::FadingOut && m_FadeTimer <= 0.0f) {
          m_State = GameState::Loading;
          m_LoadFuture = std::async(std::launch::async, [this]() {
              float lx = 0.0f, ly = 0.0f;
              PixelsEngine::SaveSystem::LoadGame(m_PendingLoadFile, GetRegistry(), m_Player, *m_Level, m_LoadedIsCamp, lx, ly);
              m_LastWorldPos.x = lx; m_LastWorldPos.y = ly;
              PixelsEngine::SaveSystem::LoadWorldFlags(m_PendingLoadFile, m_WorldFlags);
          });
          return;
      } else if (m_FadeState == FadeState::FadingIn && m_FadeTimer <= 0.0f) {
          m_FadeState = FadeState::None;
      }
      return;
  }

  switch (m_State) {
      case GameState::MainMenu: HandleMainMenuInput(); break;
      case GameState::Creation: HandleCreationInput(); break;
      case GameState::Paused: HandlePauseMenuInput(); break;
      case GameState::Options: HandleOptionsInput(); break;
      case GameState::GameOver: HandleGameOverInput(); break;
      case GameState::Map: HandleMapInput(); break;
      case GameState::CharacterMenu: HandleCharacterMenuInput(); break;
      case GameState::RestMenu: HandleRestMenuInput(); break;
      case GameState::Trading: HandleTradeInput(); break;
      case GameState::KeybindSettings: HandleKeybindInput(); break;
      case GameState::Looting: HandleLootInput(); break;
      case GameState::Dialogue: HandleDialogueInput(); break;
      case GameState::Targeting: HandleTargetingInput(); break;
      case GameState::TargetingJump: HandleTargetingJumpInput(); break;
      case GameState::TargetingShove: HandleTargetingShoveInput(); break;
      case GameState::TargetingDash: HandleTargetingDashInput(); break;
      
      case GameState::Credits:
      case GameState::Controls:
          if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) m_State = GameState::MainMenu;
          break;
      case GameState::Loading: break;

      case GameState::Camp:
      case GameState::Playing:
      case GameState::Combat:
          // 1. Handle Game Over
          auto *pStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
          if (pStats && pStats->currentHealth <= 0) {
              m_State = GameState::GameOver;
              m_MenuSelection = 0;
              return;
          }

          // 2. Handle Input
          HandleInput();

          // 3. Update Systems
          if (m_State == GameState::Combat) UpdateCombat(deltaTime);
          else UpdateAI(deltaTime);
          
          UpdateMovement(deltaTime);

          // 4. Update Camera & Visibility (Fog of War)
          auto *pTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
          if (pTrans) {
              auto *currentMap = (m_State == GameState::Camp) ? m_CampLevel.get() : m_Level.get();
              if (currentMap) {
                  // --- CRITICAL FIX: Update Visibility ---
                  // This reveals the tiles around the player. Without this, IsVisible returns false and tiles are black.
                  currentMap->UpdateVisibility((int)pTrans->x, (int)pTrans->y, 6); 
                  // ---------------------------------------

                  int screenX, screenY;
                  currentMap->GridToScreen(pTrans->x, pTrans->y, screenX, screenY);
                  auto &cam = GetCamera();
                  cam.x = screenX - cam.width / 2;
                  cam.y = screenY - cam.height / 2;
              }
          }
          break;
  }
}

PixelsEngine::Tilemap* PixelsGateGame::GetCurrentMap() {
    if (m_State == GameState::Camp) return m_CampLevel.get();
    if (m_State == GameState::Playing || m_State == GameState::Combat) return m_Level.get();
    // For menu states, check m_ReturnState
    if (m_ReturnState == GameState::Camp) return m_CampLevel.get();
    return m_Level.get();
}

void PixelsGateGame::OnRender() {
    switch (m_State) {
    case GameState::MainMenu: RenderMainMenu(); break;
    case GameState::Creation: RenderCharacterCreation(); break;
    case GameState::Options: RenderOptions(); break;
    case GameState::Credits: RenderCredits(); break;
    case GameState::Controls: RenderControls(); break;
    case GameState::Loading:
        SDL_SetRenderDrawColor(GetRenderer(), 0, 0, 0, 255);
        SDL_RenderClear(GetRenderer());
        m_TextRenderer->RenderTextCentered("Loading...", GetWindowWidth()/2, GetWindowHeight()/2, {255, 255, 255, 255});
        break;
    default:
        auto *currentMap = GetCurrentMap();
        auto &camera = GetCamera();
        
        RenderEnemyCones(camera);

        struct Renderable { float depth; PixelsEngine::Entity entity; int tileX, tileY; bool isTile; };
        std::vector<Renderable> renderQueue;

        if (currentMap) {
            for (int y = 0; y < currentMap->GetHeight(); ++y)
                for (int x = 0; x < currentMap->GetWidth(); ++x)
                    renderQueue.push_back({(float)(x + y), PixelsEngine::INVALID_ENTITY, x, y, true});
        }

        auto &sprites = GetRegistry().View<PixelsEngine::SpriteComponent>();
        for (auto &[entity, sprite] : sprites) {
            auto *transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
            if (transform && currentMap) {
                if ((m_State == GameState::Camp || m_ReturnState == GameState::Camp) && entity != m_Player) {
                    auto *tag = GetRegistry().GetComponent<PixelsEngine::TagComponent>(entity);
                    if (!tag || tag->tag != PixelsEngine::EntityTag::Companion) continue;
                }
                // Only render if visible (Fog of War)
                bool isVisible = currentMap->IsVisible((int)transform->x, (int)transform->y);
                if (entity != m_Player && !IsInTurnOrder(entity) && !isVisible) continue;
                
                renderQueue.push_back({transform->x + transform->y, entity, -1, -1, false});
            }
        }

        std::sort(renderQueue.begin(), renderQueue.end(), [](const Renderable &a, const Renderable &b) {
            if (std::abs(a.depth - b.depth) < 0.001f) return a.isTile && !b.isTile;
            return a.depth < b.depth;
        });

        for (const auto &item : renderQueue) {
            if (item.isTile) currentMap->RenderTile(item.tileX, item.tileY, camera);
            else {
                auto *transform = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(item.entity);
                auto *sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(item.entity);
                if (transform && sprite && sprite->texture) {
                    int screenX, screenY;
                    currentMap->GridToScreen(transform->x, transform->y, screenX, screenY);
                    screenX -= (int)camera.x; screenY -= (int)camera.y;
                    
                    auto *entStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(item.entity);
                    if (entStats && entStats->isDead) sprite->texture->SetColorMod(100, 100, 100);
                    if (item.entity == m_Player && entStats && entStats->isStealthed) sprite->texture->SetColorMod(150, 150, 255);

                    sprite->texture->RenderRect(screenX + 16 - sprite->pivotX, screenY + 8 - sprite->pivotY, &sprite->srcRect, -1, -1, sprite->flip);
                    if (entStats && (entStats->isDead || entStats->isStealthed)) sprite->texture->SetColorMod(255, 255, 255);

                    if (entStats && (m_State == GameState::Combat || entStats->currentHealth < entStats->maxHealth) && !entStats->isDead) {
                        SDL_Rect bg = {screenX, screenY - 8, 32, 4};
                        SDL_SetRenderDrawColor(GetRenderer(), 50, 50, 50, 255);
                        SDL_RenderFillRect(GetRenderer(), &bg);
                        SDL_Rect fg = {screenX, screenY - 8, (int)(32 * ((float)entStats->currentHealth / entStats->maxHealth)), 4};
                        SDL_SetRenderDrawColor(GetRenderer(), 255, 0, 0, 255);
                        SDL_RenderFillRect(GetRenderer(), &fg);
                    }
                }
            }
        }

        // Floating Text
        for (const auto &ft : m_FloatingText.m_Texts) {
            int sx, sy;
            currentMap->GridToScreen(ft.x, ft.y, sx, sy);
            m_TextRenderer->RenderTextCentered(ft.text, sx - (int)camera.x + 16, sy - (int)camera.y - 20, ft.color);
        }

        RenderOverlays();
        if (m_State == GameState::Combat) RenderCombatUI();
        RenderContextMenu();
        RenderDayNightCycle();
        RenderHUD(); // Draw HUD last

        if (m_SaveMessageTimer > 0.0f) m_TextRenderer->RenderText("Saving...", 20, 80, {255, 255, 0, 255});

        if (m_FadeState != FadeState::None) {
            SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_BLEND);
            float a = (m_FadeState == FadeState::FadingOut) ? 1.0f - (m_FadeTimer/m_FadeDuration) : (m_FadeTimer/m_FadeDuration);
            SDL_SetRenderDrawColor(GetRenderer(), 0, 0, 0, (Uint8)(std::clamp(a, 0.0f, 1.0f) * 255));
            SDL_Rect s = {0,0,GetWindowWidth(),GetWindowHeight()};
            SDL_RenderFillRect(GetRenderer(), &s);
            SDL_SetRenderDrawBlendMode(GetRenderer(), SDL_BLENDMODE_NONE);
        }

        if (m_State == GameState::Paused) RenderPauseMenu();
        if (m_State == GameState::RestMenu) RenderRestMenu();
        if (m_State == GameState::GameOver) RenderGameOver();
        if (m_State == GameState::Map) RenderMapScreen();
        if (m_State == GameState::CharacterMenu) RenderCharacterMenu();
        if (m_State == GameState::Trading) RenderTradeScreen();
        if (m_State == GameState::KeybindSettings) RenderKeybindSettings();
        if (m_State == GameState::Looting) RenderLootScreen();
        if (m_State == GameState::Dialogue) RenderDialogueScreen();
        
        RenderDiceRoll(); // Draw dice rolls over everything else
        break;
    }
}

void PixelsGateGame::TriggerLoadTransition(const std::string &filename) {
    m_PendingLoadFile = filename;
    m_FadeState = FadeState::FadingOut;
    m_FadeTimer = m_FadeDuration;
}