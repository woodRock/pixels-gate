#pragma once
#include "Camera.h"
#include "ECS.h"
#include <SDL2/SDL.h>
#include <memory>

namespace PixelsEngine {

class Application {
public:
  Application(const char *title, int width, int height);
  virtual ~Application();

  void Run();
  void Step();
  void ToggleFullScreen();

  SDL_Renderer *GetRenderer() const { return m_Renderer; }
  Camera &GetCamera() { return *m_Camera; }
  Registry &GetRegistry() { return m_Registry; }

  int GetWindowWidth() const { return m_Width; }
  int GetWindowHeight() const { return m_Height; }

protected:
  virtual void OnStart() {}
  virtual void OnUpdate(float deltaTime) {}
  virtual void OnRender() {}

  SDL_Window *m_Window = nullptr;
  SDL_Renderer *m_Renderer = nullptr;
  int m_Width;
  int m_Height;
  Uint32 m_LastTime = 0;
  bool m_IsRunning = false;
  std::unique_ptr<Camera> m_Camera;
  Registry m_Registry;
};

} // namespace PixelsEngine
