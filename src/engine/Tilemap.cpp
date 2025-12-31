#include "Tilemap.h"
#include "Tiles.h"
#include <iostream>

namespace PixelsEngine {

    Tilemap::Tilemap(SDL_Renderer* renderer, const std::string& texturePath, int tileWidth, int tileHeight, int mapWidth, int mapHeight)
        : m_Renderer(renderer), m_TileWidth(tileWidth), m_TileHeight(tileHeight), m_MapWidth(mapWidth), m_MapHeight(mapHeight) {
        
        m_Tileset = std::make_unique<Texture>(renderer, texturePath);
        m_MapData.resize(mapWidth * mapHeight, 0);
        m_VisibilityMap.resize(mapWidth * mapHeight, VisibilityState::Hidden); // Default Hidden
    }

    void Tilemap::UpdateVisibility(int centerX, int centerY, int radius) {
        // Reset Visible to Explored first?
        // In standard RTS/RPG Fog, current vision is recalculated every frame.
        // Previously Visible tiles become Explored.
        // Hidden remain Hidden unless revealed.
        
        for (auto& state : m_VisibilityMap) {
            if (state == VisibilityState::Visible) {
                state = VisibilityState::Explored;
            }
        }

        // Raycasting or simple radius check?
        // Simple radius check for now (Circle).
        int r2 = radius * radius;
        
        for (int y = -radius; y <= radius; ++y) {
            for (int x = -radius; x <= radius; ++x) {
                if (x*x + y*y <= r2) {
                    int targetX = centerX + x;
                    int targetY = centerY + y;
                    
                    if (targetX >= 0 && targetX < m_MapWidth && targetY >= 0 && targetY < m_MapHeight) {
                        m_VisibilityMap[targetY * m_MapWidth + targetX] = VisibilityState::Visible;
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
            return state == VisibilityState::Visible || state == VisibilityState::Explored;
        }
        return false;
    }

    void Tilemap::LoadFog(const std::vector<int>& fogData) {
        if (fogData.size() != m_VisibilityMap.size()) return;
        
        for (size_t i = 0; i < fogData.size(); ++i) {
            if (fogData[i] == 2) m_VisibilityMap[i] = VisibilityState::Visible;
            else if (fogData[i] == 1) m_VisibilityMap[i] = VisibilityState::Explored;
            else m_VisibilityMap[i] = VisibilityState::Hidden;
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

    bool Tilemap::IsWalkable(int x, int y) const {
        int tile = GetTile(x, y);
        if (tile == -1) return false; 
        
        using namespace Tiles;
        
        // Block Water, Rocks, Ocean
        if (tile >= ROCK && tile <= ROCK_VARIANT_03) return false;
        if (tile >= ROCK_ON_WATER && tile <= STONES_ON_WATER_VARIANT_11) return false;
        if (tile >= WATER && tile <= OCEAN_ROUGH) return false;
        if (tile == LOG || tile == LOGS) return false;
        
        return true; 
    }

    void Tilemap::GridToScreen(float gridX, float gridY, int& screenX, int& screenY) const {
        if (m_Projection == Projection::Isometric) {
            screenX = (int)((gridX - gridY) * (m_TileWidth / 2.0f));
            screenY = (int)((gridX + gridY) * (m_TileHeight / 2.0f));
            // Offset to center isometric map
            screenX += (m_MapWidth * m_TileWidth) / 2;
        } else {
            screenX = (int)(gridX * m_TileWidth);
            screenY = (int)(gridY * m_TileHeight);
        }
    }

    void Tilemap::ScreenToGrid(int screenX, int screenY, int& gridX, int& gridY) const {
        if (m_Projection == Projection::Isometric) {
            // Undo offset
            screenX -= (m_MapWidth * m_TileWidth) / 2;

            float halfW = m_TileWidth / 2.0f;
            float halfH = m_TileHeight / 2.0f;

            // Derived from GridToScreen formulas
            // sx = (gx - gy) * halfW  => sx/halfW = gx - gy
            // sy = (gx + gy) * halfH  => sy/halfH = gx + gy
            
            float term1 = screenY / halfH;
            float term2 = screenX / halfW;
            
            gridX = (int)((term1 + term2) / 2.0f);
            gridY = (int)((term1 - term2) / 2.0f);
        } else {
            gridX = screenX / m_TileWidth;
            gridY = screenY / m_TileHeight;
        }
    }

    void Tilemap::Render(const Camera& camera) const {
        if (!m_Tileset) return;

        int tilesetWidth = m_Tileset->GetWidth() / m_TileWidth;
        
        // Simple rendering loop (Disable culling for now to ensure correctness)
        // Proper ISO culling requires coordinate transformation which is complex.
        
        for (int y = 0; y < m_MapHeight; ++y) {
            for (int x = 0; x < m_MapWidth; ++x) {
                // Check Visibility
                VisibilityState state = m_VisibilityMap[y * m_MapWidth + x];
                
                if (state == VisibilityState::Hidden) {
                    continue; // Don't draw
                }

                int tileIndex = m_MapData[y * m_MapWidth + x];
                
                int srcX = (tileIndex % tilesetWidth) * m_TileWidth;
                int srcY = (tileIndex / tilesetWidth) * m_TileHeight;
                SDL_Rect srcRect = { srcX, srcY, m_TileWidth, m_TileHeight };
                
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
                
                m_Tileset->RenderRect(destX, destY, &srcRect);
            }
        }
        // Reset color mod to normal just in case
        m_Tileset->SetColorMod(255, 255, 255);
    }

}
