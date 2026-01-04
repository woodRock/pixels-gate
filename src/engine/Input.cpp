#include "Input.h"
#include <cstring>

namespace PixelsEngine {

Uint8 Input::m_KeyboardState[SDL_NUM_SCANCODES] = {0};
Uint8 Input::m_PrevKeyboardState[SDL_NUM_SCANCODES] = {0};
Uint32 Input::m_MouseState = 0;
Uint32 Input::m_PrevMouseState = 0;
int Input::m_MouseX = 0;
int Input::m_MouseY = 0;
SDL_Renderer* Input::m_Renderer = nullptr;

void Input::SetRenderer(SDL_Renderer* renderer) {
    m_Renderer = renderer;
}

void Input::BeginFrame() {
  memcpy(m_PrevKeyboardState, m_KeyboardState, SDL_NUM_SCANCODES);
  m_PrevMouseState = m_MouseState;

  // Sync mouse position (handles startup/no-event cases)
  int rawX, rawY;
  SDL_GetMouseState(&rawX, &rawY);
  if (m_Renderer) {
      float logX, logY;
      SDL_RenderWindowToLogical(m_Renderer, rawX, rawY, &logX, &logY);
      m_MouseX = (int)logX;
      m_MouseY = (int)logY;
  } else {
      m_MouseX = rawX;
      m_MouseY = rawY;
  }
}

void Input::ProcessEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        m_KeyboardState[e.key.keysym.scancode] = 1;
    } else if (e.type == SDL_KEYUP) {
        m_KeyboardState[e.key.keysym.scancode] = 0;
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
        m_MouseState |= SDL_BUTTON(e.button.button);
        // Update pos on click too
        int rawX = e.button.x;
        int rawY = e.button.y;
        if (m_Renderer) {
            float logX, logY;
            SDL_RenderWindowToLogical(m_Renderer, rawX, rawY, &logX, &logY);
            m_MouseX = (int)logX;
            m_MouseY = (int)logY;
        } else {
            m_MouseX = rawX;
            m_MouseY = rawY;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP) {
        m_MouseState &= ~SDL_BUTTON(e.button.button);
        // Update pos on click release too
        int rawX = e.button.x;
        int rawY = e.button.y;
        if (m_Renderer) {
            float logX, logY;
            SDL_RenderWindowToLogical(m_Renderer, rawX, rawY, &logX, &logY);
            m_MouseX = (int)logX;
            m_MouseY = (int)logY;
        } else {
            m_MouseX = rawX;
            m_MouseY = rawY;
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        int rawX = e.motion.x;
        int rawY = e.motion.y;
        if (m_Renderer) {
            float logX, logY;
            SDL_RenderWindowToLogical(m_Renderer, rawX, rawY, &logX, &logY);
            m_MouseX = (int)logX;
            m_MouseY = (int)logY;
        } else {
            m_MouseX = rawX;
            m_MouseY = rawY;
        }
    }
}

bool Input::IsKeyDown(SDL_Scancode key) {
  return m_KeyboardState[key];
}

bool Input::IsKeyPressed(SDL_Scancode key) {
  return m_KeyboardState[key] && !m_PrevKeyboardState[key];
}

bool Input::IsKeyReleased(SDL_Scancode key) {
  return !m_KeyboardState[key] && m_PrevKeyboardState[key];
}

bool Input::IsMouseButtonDown(int button) {
  return (m_MouseState & SDL_BUTTON(button));
}

bool Input::IsMouseButtonPressed(int button) {
  return (m_MouseState & SDL_BUTTON(button)) &&
         !(m_PrevMouseState & SDL_BUTTON(button));
}

void Input::GetMousePosition(int &x, int &y) {
  x = m_MouseX;
  y = m_MouseY;
}

} // namespace PixelsEngine
