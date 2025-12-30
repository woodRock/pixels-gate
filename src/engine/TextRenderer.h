#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <iostream>

namespace PixelsEngine {

    class TextRenderer {
    public:
        TextRenderer(SDL_Renderer* renderer, const std::string& fontPath, int fontSize);
        ~TextRenderer();

        void RenderText(const std::string& text, int x, int y, SDL_Color color);
        // Render centered relative to a position (good for names/bubbles)
        void RenderTextCentered(const std::string& text, int x, int y, SDL_Color color);

    private:
        SDL_Renderer* m_Renderer;
        TTF_Font* m_Font = nullptr;
    };

}
