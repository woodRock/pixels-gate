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
  void ToggleFullScreen();

  SDL_Renderer *GetRenderer() const { return m_Renderer; }
  Camera &GetCamera() { return *m_Camera; }
  Registry &GetRegistry() { return m_Registry; }

protected:
  virtual void OnStart() {}
  virtual void OnUpdate(float deltaTime) {}
  virtual void OnRender() {}

  SDL_Window *m_Window = nullptr;
  SDL_Renderer *m_Renderer = nullptr;
  bool m_IsRunning = false;
  std::unique_ptr<Camera> m_Camera;
  Registry m_Registry;
};

} // namespace PixelsEngine
