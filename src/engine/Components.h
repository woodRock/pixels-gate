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

    struct StatsComponent {
        int maxHealth = 100;
        int currentHealth = 100;
        int damage = 10;
        bool isDead = false;
        
        int inspiration = 1; // Default 1 inspiration point

        // D&D Stats
        int strength = 10;
        int dexterity = 10;
        int constitution = 10;
        int intelligence = 10;
        int wisdom = 10;
        int charisma = 10;

        std::string characterClass = "Commoner";
        std::string race = "Human";

        int GetModifier(int score) const {
            return (score - 10) / 2;
        }
    };

    struct QuestComponent {
        std::string questId;
        int state = 0; // 0: None, 1: Active, 2: Completed
        std::string targetItem;
    };

    struct PlayerComponent {
        float speed = 5.0f;
    };

}
