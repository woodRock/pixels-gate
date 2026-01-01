#include "Texture.h"
#include <SDL2/SDL_image.h>
#include <iostream>

namespace PixelsEngine {

Texture::Texture(SDL_Renderer *renderer, const std::string &path)
    : m_Renderer(renderer) {

  // Load image at specified path using SDL_image
  SDL_Surface *surface = IMG_Load(path.c_str());
  if (!surface) {
    std::cerr << "Failed to load image: " << path
              << " IMG_Error: " << IMG_GetError() << std::endl;
    return;
  }

  // SDL_image handles alpha channels automatically for PNGs,
  // so we don't strictly need SetColorKey unless we are using BMPs with a
  // specific key. For backwards compatibility with our BMP test assets, we can
  // keep it if format is not alpha. But usually, IMG_Load handles everything
  // nicely.

  m_Texture = SDL_CreateTextureFromSurface(m_Renderer, surface);
  m_Width = surface->w;
  m_Height = surface->h;

  SDL_FreeSurface(surface);

  if (!m_Texture) {
    std::cerr << "Failed to create texture from " << path
              << " SDL_Error: " << SDL_GetError() << std::endl;
  }
}

Texture::~Texture() {
  if (m_Texture) {
    SDL_DestroyTexture(m_Texture);
  }
}

void Texture::Render(int x, int y, int w, int h) const {
  RenderRect(x, y, NULL, w, h);
}

void Texture::RenderRect(int x, int y, const SDL_Rect *srcRect, int w, int h,
                         SDL_RendererFlip flip) const {
  if (!m_Texture)
    return;

  int destW = (w == -1) ? (srcRect ? srcRect->w : m_Width) : w;
  int destH = (h == -1) ? (srcRect ? srcRect->h : m_Height) : h;

  SDL_Rect destRect = {x, y, destW, destH};
  SDL_RenderCopyEx(m_Renderer, m_Texture, srcRect, &destRect, 0.0, NULL, flip);
}

void Texture::SetColorMod(Uint8 r, Uint8 g, Uint8 b) {
  if (m_Texture) {
    SDL_SetTextureColorMod(m_Texture, r, g, b);
  }
}

} // namespace PixelsEngine
