#include "TextureManager.h"

namespace PixelsEngine {
std::unordered_map<std::string, std::shared_ptr<Texture>>
    TextureManager::m_Textures;
}
