#pragma once
#include "ECS.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>

namespace PixelsEngine {

enum class ContextActionType { Attack, Talk, Pickpocket, Trade, Lockpick };

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
  EndConversation,
  StartQuest,
  CompleteQuest
};

struct DialogueOption {
  std::string text;
  std::string nextNodeId;
  std::string requiredStat = "None"; // "Intelligence", "Charisma", etc.
  int dc = 0;
  std::string excludeFlag = "";  // Hides option if flag is TRUE
  std::string requiredFlag = ""; // Shows option ONLY if flag is TRUE
  std::string requiredItem = ""; // Shows option ONLY if player has item
  std::string onSuccessNodeId = "";
  std::string onFailureNodeId = "";
  DialogueAction action = DialogueAction::None;
  std::string actionParam = ""; // Flag name or Item name
  bool repeatable = true;
  bool hasBeenChosen = false;

  DialogueOption(std::string t, std::string next, std::string stat = "None",
                 int d = 0, std::string success = "", std::string fail = "",
                 DialogueAction act = DialogueAction::None,
                 std::string param = "", std::string exclFlag = "",
                 bool rep = true, std::string reqFlag = "",
                 std::string reqItem = "")
      : text(t), nextNodeId(next), requiredStat(stat), dc(d),
        excludeFlag(exclFlag), requiredFlag(reqFlag), requiredItem(reqItem),
        onSuccessNodeId(success), onFailureNodeId(fail), action(act),
        actionParam(param), repeatable(rep) {}
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

} // namespace PixelsEngine
