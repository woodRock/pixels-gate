#pragma once
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include "ECS.h"

namespace PixelsEngine {

    enum class ContextActionType {
        Attack,
        Talk,
        Pickpocket,
        Trade
    };

    struct ContextAction {
        std::string label;
        ContextActionType type;
    };

    struct ContextMenu {
        bool isOpen = false;
        int x, y;
        Entity targetEntity = INVALID_ENTITY;
        std::vector<ContextAction> actions;
    };

    struct DiceRollAnimation {
        bool active = false;
        int finalValue = 0;
        int modifier = 0;
        int dc = 0;
        int currentFrameValue = 1;
        float timer = 0.0f;
        float duration = 1.0f; // Roll for 1 second
        bool resultShown = false;
        bool success = false;
        
        std::string skillName;
        Entity target = INVALID_ENTITY;
        ContextActionType actionType;
    };

}
