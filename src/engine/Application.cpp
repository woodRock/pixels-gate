#include "Application.h"
#include "Input.h"
#include "AudioManager.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace PixelsEngine {

Application::Application(const char *title, int width, int height)
    : m_Width(width), m_Height(height) {
  // Initialize Camera
  m_Camera = std::make_unique<Camera>(width, height);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError()
              << std::endl;
    return;
  }

  // Initialize SDL_mixer
  if (!AudioManager::Init()) {
    std::cerr << "AudioManager could not initialize!" << std::endl;
  }

  // Initialize PNG and JPG loading
  int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    std::cerr << "SDL_image could not initialize! SDL_image Error: "
              << IMG_GetError() << std::endl;
    return;
  }

  // Initialize SDL_ttf
  if (TTF_Init() == -1) {
    std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: "
              << TTF_GetError() << std::endl;
    return;
  }

  m_Window =
      SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       width, height, SDL_WINDOW_SHOWN);
  if (!m_Window) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError()
              << std::endl;
    return;
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // 0 = nearest pixel sampling

  m_Renderer = SDL_CreateRenderer(
      m_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!m_Renderer) {
    std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError()
              << std::endl;
    return;
  }

  SDL_RenderSetLogicalSize(m_Renderer, width, height);

  m_IsRunning = true;
}

Application::~Application() {
  if (m_Renderer)
    SDL_DestroyRenderer(m_Renderer);
  if (m_Window)
    SDL_DestroyWindow(m_Window);
  AudioManager::Quit();
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
}

void Application::ToggleFullScreen() {
  Uint32 fullscreenFlag = SDL_WINDOW_FULLSCREEN_DESKTOP;
  bool isFullscreen = SDL_GetWindowFlags(m_Window) & fullscreenFlag;
  SDL_SetWindowFullscreen(m_Window, isFullscreen ? 0 : fullscreenFlag);
  SDL_RenderSetLogicalSize(m_Renderer, m_Width, m_Height);
}

void Application::Run() {
  OnStart();
  m_LastTime = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg([](void* arg){
      static_cast<Application*>(arg)->Step();
  }, this, 0, 1);
#else
  while (m_IsRunning) {
      Step();
  }
#endif
}

void Application::Step() {
    Input::SetRenderer(m_Renderer);
    Input::BeginFrame();

    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - m_LastTime) / 1000.0f;
    m_LastTime = currentTime;

    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
      Input::ProcessEvent(e);

      if (e.type == SDL_QUIT) {
        m_IsRunning = false;
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
      } else if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_RESIZED ||
            e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
          SDL_RenderSetLogicalSize(m_Renderer, m_Width, m_Height);
        }
      }
    }

    OnUpdate(deltaTime);

    SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_Renderer);

    OnRender();

    SDL_RenderPresent(m_Renderer);
}

} // namespace PixelsEngine
