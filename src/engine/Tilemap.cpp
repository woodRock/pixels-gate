#include "Tilemap.h"
#include <iostream>

namespace PixelsEngine {

    Tilemap::Tilemap(SDL_Renderer* renderer, const std::string& texturePath, int tileWidth, int tileHeight, int mapWidth, int mapHeight)
        : m_Renderer(renderer), m_TileWidth(tileWidth), m_TileHeight(tileHeight), m_MapWidth(mapWidth), m_MapHeight(mapHeight) {
        
        m_Tileset = std::make_unique<Texture>(renderer, texturePath);
        m_MapData.resize(mapWidth * mapHeight, 0);
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
        if (tile == -1) return false; // Cannot walk out of bounds
        
        // Define Walkable Indices
        // Grass (0, 1) and Dirt (2) are walkable. 
        // Walls (10) and Water (11) are not.
        return tile < 10; 
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
                int tileIndex = m_MapData[y * m_MapWidth + x];
                
                int srcX = (tileIndex % tilesetWidth) * m_TileWidth;
                int srcY = (tileIndex / tilesetWidth) * m_TileHeight;
                SDL_Rect srcRect = { srcX, srcY, m_TileWidth, m_TileHeight };
                
                int destX, destY;
                GridToScreen((float)x, (float)y, destX, destY);
                
                destX -= (int)camera.x;
                destY -= (int)camera.y;

                m_Tileset->RenderRect(destX, destY, &srcRect);
            }
        }
    }

}
