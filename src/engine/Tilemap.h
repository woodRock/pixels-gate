#pragma once
#include "Texture.h"
#include "Camera.h"
#include <vector>
#include <string>
#include <memory>

namespace PixelsEngine {

    enum class Projection {
        TopDown,
        Isometric
    };

    enum class VisibilityState {
        Hidden,   // Never seen (Black)
        Explored, // Seen previously (Darkened)
        Visible   // Currently seeing (Normal)
    };

    class Tilemap {
    public:
        Tilemap(SDL_Renderer* renderer, const std::string& texturePath, int tileWidth, int tileHeight, int mapWidth, int mapHeight);
        ~Tilemap() = default;

        void Render(const Camera& camera) const;
        void SetTile(int x, int y, int tileIndex);
        int GetTile(int x, int y) const;
        bool IsWalkable(int x, int y) const;
        void SetProjection(Projection projection) { m_Projection = projection; }

        // Fog of War
        void UpdateVisibility(int centerX, int centerY, int radius);
        bool IsVisible(int x, int y) const;
        bool IsExplored(int x, int y) const;

        // Helper to convert Grid Coords to Screen Coords
        void GridToScreen(float gridX, float gridY, int& screenX, int& screenY) const;
        // Helper to convert Screen Coords to Grid Coords
        void ScreenToGrid(int screenX, int screenY, int& gridX, int& gridY) const;

    private:
        std::unique_ptr<Texture> m_Tileset;
        int m_TileWidth;
        int m_TileHeight;
        int m_MapWidth; 
        int m_MapHeight;
        std::vector<int> m_MapData;
        std::vector<VisibilityState> m_VisibilityMap; // Stores fog state
        SDL_Renderer* m_Renderer;
        Projection m_Projection = Projection::TopDown;
    };

}
