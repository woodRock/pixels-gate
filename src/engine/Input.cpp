#include "Input.h"
#include <cstring>

namespace PixelsEngine {

const Uint8 *Input::m_KeyboardState = nullptr;
Uint8 Input::m_PrevKeyboardState[SDL_NUM_SCANCODES] = {0};
Uint32 Input::m_MouseState = 0;
Uint32 Input::m_PrevMouseState = 0;
int Input::m_MouseX = 0;
int Input::m_MouseY = 0;

void Input::Update(SDL_Renderer *renderer) {
  if (!m_KeyboardState) {
    m_KeyboardState = SDL_GetKeyboardState(NULL);
  }

  memcpy(m_PrevKeyboardState, m_KeyboardState, SDL_NUM_SCANCODES);
  m_PrevMouseState = m_MouseState;

  int rawX, rawY;
  m_MouseState = SDL_GetMouseState(&rawX, &rawY);

  if (renderer) {
    float logX, logY;
    SDL_RenderWindowToLogical(renderer, rawX, rawY, &logX, &logY);
    m_MouseX = (int)logX;
    m_MouseY = (int)logY;
  } else {
    m_MouseX = rawX;
    m_MouseY = rawY;
  }
}

bool Input::IsKeyDown(SDL_Scancode key) {
  if (!m_KeyboardState)
    return false;
  return m_KeyboardState[key];
}

bool Input::IsKeyPressed(SDL_Scancode key) {
  if (!m_KeyboardState)
    return false;
  return m_KeyboardState[key] && !m_PrevKeyboardState[key];
}

bool Input::IsKeyReleased(SDL_Scancode key) {
  if (!m_KeyboardState)
    return false;
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