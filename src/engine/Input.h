#pragma once
#include <SDL2/SDL.h>
#include <unordered_map>

namespace PixelsEngine {

class Input {
public:
  static void BeginFrame();
  static void ProcessEvent(const SDL_Event& e);
  static void SetRenderer(SDL_Renderer* renderer);

  static bool IsKeyDown(SDL_Scancode key);
  static bool IsKeyPressed(SDL_Scancode key);
  static bool IsKeyReleased(SDL_Scancode key);

  static bool IsMouseButtonDown(int button);
  static bool IsMouseButtonPressed(int button);
  static void GetMousePosition(int &x, int &y);

private:
  static Uint8 m_KeyboardState[SDL_NUM_SCANCODES];
  static Uint8 m_PrevKeyboardState[SDL_NUM_SCANCODES];
  static Uint32 m_MouseState;
  static Uint32 m_PrevMouseState;
  static int m_MouseX, m_MouseY;
  static SDL_Renderer* m_Renderer;
};

} // namespace PixelsEngine
