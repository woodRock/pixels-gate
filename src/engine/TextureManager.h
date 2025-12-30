#pragma once
#include "Texture.h"
#include <unordered_map>
#include <string>
#include <memory>

namespace PixelsEngine {

    class TextureManager {
    public:
        static std::shared_ptr<Texture> LoadTexture(SDL_Renderer* renderer, const std::string& path) {
            if (m_Textures.find(path) != m_Textures.end()) {
                return m_Textures[path];
            }
            auto texture = std::make_shared<Texture>(renderer, path);
            m_Textures[path] = texture;
            return texture;
        }

        static void Clear() {
            m_Textures.clear();
        }

    private:
        static std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
    };

}
