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
        float timer = 0.0f;
        float duration = 1.0f;
        int finalValue = 0;
        int modifier = 0;
        int dc = 10;
        bool resultShown = false;
        bool success = false;
        std::string skillName = "";
        Entity target = INVALID_ENTITY;
        ContextActionType actionType = ContextActionType::Attack;
    };

    enum class DialogueAction {
        None,
        StartCombat,
        GiveItem,
        SetFlag,
        EndConversation
    };

    struct DialogueOption {
        std::string text;
        std::string nextNodeId;
        std::string requiredStat = "None"; // "Intelligence", "Charisma", etc.
        int dc = 0;
        std::string onSuccessNodeId;
        std::string onFailureNodeId;
        DialogueAction action = DialogueAction::None;
        std::string actionParam = ""; // Flag name or Item name
    };

    struct DialogueNode {
        std::string id;
        std::string npcText;
        std::vector<DialogueOption> options;
    };

    struct DialogueTree {
        std::string currentEntityName;
        std::string currentNodeId;
        std::unordered_map<std::string, DialogueNode> nodes;
    };

}
