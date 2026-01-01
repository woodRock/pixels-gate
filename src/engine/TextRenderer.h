#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>

namespace PixelsEngine {

class TextRenderer {
public:
  TextRenderer(SDL_Renderer *renderer, const std::string &fontPath,
               int fontSize);
  ~TextRenderer();

  void RenderText(const std::string &text, int x, int y, SDL_Color color);
  void RenderTextSmall(const std::string &text, int x, int y, SDL_Color color);
    void RenderTextRightAlignedSmall(const std::string& text, int rightX, int y, SDL_Color color);
    int RenderTextWrapped(const std::string& text, int x, int y, int wrapWidth, SDL_Color color);
    int MeasureTextWrapped(const std::string& text, int wrapWidth);
    // Render centered relative to a position (good for names/bubbles)
  void RenderTextCentered(const std::string &text, int x, int y,
                          SDL_Color color);

private:
  TTF_Font *m_Font = nullptr;
  TTF_Font *m_SmallFont = nullptr;
  SDL_Renderer *m_Renderer;
};

} // namespace PixelsEngine
