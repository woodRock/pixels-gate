#include "Tilemap.h"
#include "Tiles.h"
#include <iostream>

namespace PixelsEngine {

Tilemap::Tilemap(SDL_Renderer *renderer, const std::string &texturePath,
                 int tileWidth, int tileHeight, int mapWidth, int mapHeight)
    : m_Renderer(renderer), m_TileWidth(tileWidth), m_TileHeight(tileHeight),
      m_MapWidth(mapWidth), m_MapHeight(mapHeight) {

  m_Tileset = std::make_unique<Texture>(renderer, texturePath);
  m_MapData.resize(mapWidth * mapHeight, 0);
  m_VisibilityMap.resize(mapWidth * mapHeight,
                         VisibilityState::Hidden); // Default Hidden
}

void Tilemap::UpdateVisibility(int centerX, int centerY, int radius) {
  // Reset Visible to Explored first?
  // In standard RTS/RPG Fog, current vision is recalculated every frame.
  // Previously Visible tiles become Explored.
  // Hidden remain Hidden unless revealed.

  for (auto &state : m_VisibilityMap) {
    if (state == VisibilityState::Visible) {
      state = VisibilityState::Explored;
    }
  }

  // Raycasting or simple radius check?
  // Simple radius check for now (Circle).
  int r2 = radius * radius;

  for (int y = -radius; y <= radius; ++y) {
    for (int x = -radius; x <= radius; ++x) {
      if (x * x + y * y <= r2) {
        int targetX = centerX + x;
        int targetY = centerY + y;

        if (targetX >= 0 && targetX < m_MapWidth && targetY >= 0 &&
            targetY < m_MapHeight) {
          m_VisibilityMap[targetY * m_MapWidth + targetX] =
              VisibilityState::Visible;
        }
      }
    }
  }
}

bool Tilemap::IsVisible(int x, int y) const {
  if (x >= 0 && x < m_MapWidth && y >= 0 && y < m_MapHeight) {
    return m_VisibilityMap[y * m_MapWidth + x] == VisibilityState::Visible;
  }
  return false;
}

bool Tilemap::IsExplored(int x, int y) const {
  if (x >= 0 && x < m_MapWidth && y >= 0 && y < m_MapHeight) {
    auto state = m_VisibilityMap[y * m_MapWidth + x];
    return state == VisibilityState::Visible ||
           state == VisibilityState::Explored;
  }
  return false;
}

void Tilemap::LoadFog(const std::vector<int> &fogData) {
  if (fogData.size() != m_VisibilityMap.size())
    return;

  for (size_t i = 0; i < fogData.size(); ++i) {
    if (fogData[i] == 2)
      m_VisibilityMap[i] = VisibilityState::Visible;
    else if (fogData[i] == 1)
      m_VisibilityMap[i] = VisibilityState::Explored;
    else
      m_VisibilityMap[i] = VisibilityState::Hidden;
  }
}

void Tilemap::SetTile(int x, int y, int tileIndex) {
  if (x >= 0 && x < m_MapWidth && y >= 0 && y < m_MapHeight) {
    m_MapData[y * m_MapWidth + x] = tileIndex;
  }
}

int Tilemap::GetTile(int x, int y) const {
  if (x >= 0 && x < m_MapWidth && y >= 0 && y < m_MapHeight) {
    return m_MapData[y * m_MapWidth + x];
  }
  return -1; // Out of bounds
}

int Tilemap::GetTileHeight(int x, int y) const {
  int tile = GetTile(x, y);
  if (tile == -1)
    return -1;

  using namespace Tiles;
  // Deep Water / Ocean (Level 0)
  if (tile >= DEEP_WATER && tile <= OCEAN_ROUGH)
    return 0;
  // Regular Water (Level 1)
  if ((tile >= WATER_DROPLETS && tile <= WATER_VARIANT_01) ||
      (tile >= STONES_ON_WATER && tile <= STONES_ON_WATER_VARIANT_11) ||
      tile == ROCK_ON_WATER || tile == SMOOTH_STONE_ON_WATER)
    return 1;

  // Land (Level 2)
  return 2;
}

bool Tilemap::IsWalkable(int x, int y) const {
  int tile = GetTile(x, y);
  if (tile == -1)
    return false;

  using namespace Tiles;

  // Block Water, Rocks, Ocean
  if (tile >= ROCK && tile <= ROCK_VARIANT_03)
    return false;
  if (tile >= ROCK_ON_WATER && tile <= STONES_ON_WATER_VARIANT_11)
    return false;
  if (tile >= WATER && tile <= OCEAN_ROUGH)
    return false;
  if (tile == LOG || tile == LOGS)
    return false;

  return true;
}

void Tilemap::GridToScreen(float gridX, float gridY, int &screenX,
                           int &screenY) const {
  if (m_Projection == Projection::Isometric) {
    screenX = (int)((gridX - gridY) * (m_TileWidth / 2.0f));
    screenY = (int)((gridX + gridY) * (m_TileHeight / 4.0f));

    // Elevation (each level is 8 pixels high)
    int h = GetTileHeight((int)gridX, (int)gridY);
    if (h != -1) {
      screenY -= h * 8;
    }

    // Offset to center isometric map
    screenX += (m_MapWidth * m_TileWidth) / 2;
  } else {
    screenX = (int)(gridX * m_TileWidth);
    screenY = (int)(gridY * m_TileHeight);
  }
}

void Tilemap::ScreenToGrid(int screenX, int screenY, int &gridX,
                           int &gridY) const {
  if (m_Projection == Projection::Isometric) {
    // Undo centering offset
    screenX -= (m_MapWidth * m_TileWidth) / 2;

    float halfW = m_TileWidth / 2.0f;
    float quarterH = m_TileHeight / 4.0f;

    // Tiered picking: Check each possible height from top to bottom
    for (int h = 2; h >= 0; --h) {
      float adjustedY = (float)screenY + (h * 8);
      float term1 = adjustedY / quarterH;
      float term2 = (float)screenX / halfW;

      int gx = (int)std::floor((term1 + term2) / 2.0f);
      int gy = (int)std::floor((term1 - term2) / 2.0f);

      if (gx >= 0 && gx < m_MapWidth && gy >= 0 && gy < m_MapHeight) {
        if (GetTileHeight(gx, gy) == h) {
          gridX = gx;
          gridY = gy;
          return;
        }
      }
    }

    // Fallback to floor picking (h=0) if no elevated tile matches
    float term1 = (float)screenY / quarterH;
    float term2 = (float)screenX / halfW;
    gridX = (int)std::floor((term1 + term2) / 2.0f);
    gridY = (int)std::floor((term1 - term2) / 2.0f);
  } else {
    gridX = screenX / m_TileWidth;
    gridY = screenY / m_TileHeight;
  }
}

void Tilemap::Render(const Camera &camera) const {
  if (!m_Tileset)
    return;

  // Render loop
  for (int y = 0; y < m_MapHeight; ++y) {
    for (int x = 0; x < m_MapWidth; ++x) {
      RenderTile(x, y, camera);
    }
  }
}

void Tilemap::RenderTile(int x, int y, const Camera &camera) const {
  if (!m_Tileset || x < 0 || x >= m_MapWidth || y < 0 || y >= m_MapHeight)
    return;

  // Check Visibility
  VisibilityState state = m_VisibilityMap[y * m_MapWidth + x];
  if (state == VisibilityState::Hidden)
    return;

  int tilesetWidth = m_Tileset->GetWidth() / m_TileWidth;
  int tileIndex = m_MapData[y * m_MapWidth + x];

  int srcX = (tileIndex % tilesetWidth) * m_TileWidth;
  int srcY = (tileIndex / tilesetWidth) * m_TileHeight;

  int destX, destY;
  GridToScreen((float)x, (float)y, destX, destY);

  destX -= (int)camera.x;
  destY -= (int)camera.y;

  // Color Mod for Fog
  if (state == VisibilityState::Explored) {
    m_Tileset->SetColorMod(100, 100, 100); // Darken
  } else {
    m_Tileset->SetColorMod(255, 255, 255); // Normal
  }

  if (m_Projection == Projection::Isometric) {
    // 1. Render Top Surface (Full Diamond: Rows 8-23)
    // This includes both the top and bottom triangles of the face.
    SDL_Rect srcTop = {srcX, srcY + 8, m_TileWidth, 16};
    m_Tileset->RenderRect(destX, destY, &srcTop);

    int myHeight = GetTileHeight(x, y);

    // 2. Render Left Side (Rows 24-31)
    // Shown if front-left neighbor at (x, y+1) is lower
    if (myHeight > GetTileHeight(x, y + 1)) {
      SDL_Rect srcLeft = {srcX, srcY + 24, m_TileWidth / 2, 8};
      m_Tileset->RenderRect(destX, destY + 16, &srcLeft);
    }

    // 3. Render Right Side (Rows 24-31)
    // Shown if front-right neighbor at (x+1, y) is lower
    if (myHeight > GetTileHeight(x + 1, y)) {
      SDL_Rect srcRight = {srcX + m_TileWidth / 2, srcY + 24, m_TileWidth / 2, 8};
      m_Tileset->RenderRect(destX + m_TileWidth / 2, destY + 16, &srcRight);
    }
  } else {
    // Standard TopDown rendering
    SDL_Rect srcRect = {srcX, srcY, m_TileWidth, m_TileHeight};
    m_Tileset->RenderRect(destX, destY, &srcRect);
  }

  // Reset color mod
  if (state == VisibilityState::Explored) {
    m_Tileset->SetColorMod(255, 255, 255);
  }
}

} // namespace PixelsEngine
