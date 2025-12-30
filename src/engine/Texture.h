#pragma once
#include <SDL2/SDL.h>
#include <string>

namespace PixelsEngine {

    class Texture {
    public:
        Texture(SDL_Renderer* renderer, const std::string& path);
        ~Texture();

        void Render(int x, int y, int w = -1, int h = -1) const;
        void RenderRect(int x, int y, const SDL_Rect* srcRect, int w = -1, int h = -1, SDL_RendererFlip flip = SDL_FLIP_NONE) const;

        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

    private:
        SDL_Renderer* m_Renderer = nullptr;
        SDL_Texture* m_Texture = nullptr;
        int m_Width = 0;
        int m_Height = 0;
    };

}
