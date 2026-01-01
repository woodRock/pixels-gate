#pragma once

#include "../engine/ECS.h"
#include <vector>
#include <string>
#include <algorithm>
#include <SDL2/SDL.h>

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
  float m_CombatTurnTimer = 0.0f; // AI delay
  
  // Helpers
  bool IsCombatActive() const { return !m_TurnOrder.empty(); }
  bool IsPlayerTurn() const {
      if (m_CurrentTurnIndex >= 0 && m_CurrentTurnIndex < (int)m_TurnOrder.size()) {
          return m_TurnOrder[m_CurrentTurnIndex].isPlayer;
      }
      return false;
  }
};

class TimeManager {
public:
    // Day/Night Cycle (0.0 to 24.0, starts at 8.0)
    float m_TimeOfDay = 8.0f;
    const float m_TimeSpeed = 0.1f;

    void Update(float deltaTime) {
        m_TimeOfDay += m_TimeSpeed * deltaTime;
        if (m_TimeOfDay >= 24.0f)
            m_TimeOfDay -= 24.0f;
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
        m_Texts.push_back({x, y, text, 1.0f, color});
    }

    void Update(float deltaTime) {
        for (auto &ft : m_Texts) {
            ft.y -= 20.0f * deltaTime; // Float up
            ft.life -= deltaTime;
        }
        m_Texts.erase(std::remove_if(m_Texts.begin(), m_Texts.end(),
                                     [](const FloatingText &ft) { return ft.life <= 0.0f; }),
                      m_Texts.end());
    }
};
