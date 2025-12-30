#pragma once
#include <string>
#include <memory>
#include <SDL2/SDL.h>
#include "Texture.h"

namespace PixelsEngine {

    struct TransformComponent {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct SpriteComponent {
        std::shared_ptr<Texture> texture;
        SDL_Rect srcRect = {0, 0, 0, 0};
        int pivotX = 0;
        int pivotY = 0;
        SDL_RendererFlip flip = SDL_FLIP_NONE;
    };

    struct PathMovementComponent {
        std::vector<std::pair<int, int>> path;
        int currentPathIndex = 0;
        bool isMoving = false;
        float targetX, targetY; // Pixel coordinates of next tile center
    };

    struct InteractionComponent {
        std::string dialogueText;
        bool showDialogue = false;
        float dialogueTimer = 0.0f;
    };

    struct PlayerComponent {
        float speed = 5.0f;
    };

}
