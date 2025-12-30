#include "Application.h"
#include "Input.h"
#include <iostream>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

namespace PixelsEngine {

    Application::Application(const char* title, int width, int height) {
        // Initialize Camera
        m_Camera = std::make_unique<Camera>(width, height);

        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }

        // Initialize PNG and JPG loading
        int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
        if( !( IMG_Init( imgFlags ) & imgFlags ) )
        {
            std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
            return;
        }

        // Initialize SDL_ttf
        if (TTF_Init() == -1) {
            std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
            return;
        }

        m_Window = SDL_CreateWindow(title,
                                    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                    width, height,
                                    SDL_WINDOW_SHOWN);
        if (!m_Window) {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // 0 = nearest pixel sampling

        m_Renderer = SDL_CreateRenderer(m_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!m_Renderer) {
            std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }

        m_IsRunning = true;
    }

    Application::~Application() {
        if (m_Renderer) SDL_DestroyRenderer(m_Renderer);
        if (m_Window) SDL_DestroyWindow(m_Window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
    }

    void Application::Run() {
        OnStart();

        SDL_Event e;
        Uint32 lastTime = SDL_GetTicks();

        while (m_IsRunning) {
            // Update input state (copy current to previous)
            Input::Update();

            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;

            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT) {
                    m_IsRunning = false;
                }
            }

            OnUpdate(deltaTime);

            // Basic clear to black
            SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 255);
            SDL_RenderClear(m_Renderer);

            OnRender();

            SDL_RenderPresent(m_Renderer);
        }
    }

}
