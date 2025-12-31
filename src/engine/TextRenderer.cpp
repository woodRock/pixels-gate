#include "TextRenderer.h"

namespace PixelsEngine {

    TextRenderer::TextRenderer(SDL_Renderer* renderer, const std::string& fontPath, int fontSize)
        : m_Renderer(renderer) {
        m_Font = TTF_OpenFont(fontPath.c_str(), fontSize);
        m_SmallFont = TTF_OpenFont(fontPath.c_str(), fontSize - 6); // Smaller font
        if (!m_Font) {
            std::cerr << "Failed to load font: " << fontPath << " Error: " << TTF_GetError() << std::endl;
        }
    }

    TextRenderer::~TextRenderer() {
        if (m_Font) {
            TTF_CloseFont(m_Font);
        }
        if (m_SmallFont) {
            TTF_CloseFont(m_SmallFont);
        }
    }

    void TextRenderer::RenderText(const std::string& text, int x, int y, SDL_Color color) {
        if (!m_Font) return;

        SDL_Surface* surface = TTF_RenderText_Solid(m_Font, text.c_str(), color);
        if (!surface) return;

        SDL_Texture* texture = SDL_CreateTextureFromSurface(m_Renderer, surface);
        if (texture) {
            SDL_Rect destRect = { x, y, surface->w, surface->h };
            SDL_RenderCopy(m_Renderer, texture, NULL, &destRect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }

    void TextRenderer::RenderTextSmall(const std::string& text, int x, int y, SDL_Color color) {
        if (!m_SmallFont) return;

        SDL_Surface* surface = TTF_RenderText_Solid(m_SmallFont, text.c_str(), color);
        if (!surface) return;

        SDL_Texture* texture = SDL_CreateTextureFromSurface(m_Renderer, surface);
        if (texture) {
            SDL_Rect destRect = { x, y, surface->w, surface->h };
            SDL_RenderCopy(m_Renderer, texture, NULL, &destRect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }

    int TextRenderer::RenderTextWrapped(const std::string& text, int x, int y, int wrapWidth, SDL_Color color) {
        if (!m_Font) return 0;

        SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(m_Font, text.c_str(), color, wrapWidth);
        if (!surface) return 0;

        int h = surface->h;
        SDL_Texture* texture = SDL_CreateTextureFromSurface(m_Renderer, surface);
        if (texture) {
            SDL_Rect destRect = { x, y, surface->w, surface->h };
            SDL_RenderCopy(m_Renderer, texture, NULL, &destRect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
        return h;
    }

    void TextRenderer::RenderTextCentered(const std::string& text, int x, int y, SDL_Color color) {
        if (!m_Font) return;

        SDL_Surface* surface = TTF_RenderText_Solid(m_Font, text.c_str(), color);
        if (!surface) return;

        SDL_Texture* texture = SDL_CreateTextureFromSurface(m_Renderer, surface);
        if (texture) {
            SDL_Rect destRect = { x - surface->w / 2, y - surface->h / 2, surface->w, surface->h };
            SDL_RenderCopy(m_Renderer, texture, NULL, &destRect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }

}
