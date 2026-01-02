#pragma once
#include "Inventory.h"
#include "Texture.h"
#include <SDL2/SDL.h>
#include <memory>
#include <string>

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
  std::string uniqueId = ""; // Stable ID for saving/loading
  bool showDialogue = false;
  float dialogueTimer = 0.0f;
};

struct StatsComponent {
  int maxHealth = 100;
  int currentHealth = 100;
  int damage = 10;
  bool isDead = false;

  int experience = 0;
  int level = 1;
  int inspiration = 1; // Default 1 inspiration point
  bool isStealthed = false;
  bool hasAdvantage = false;
  int shortRestsAvailable = 2;
  int maxShortRests = 2;

  // D&D Stats
  int strength = 10;
  int dexterity = 10;
  int constitution = 10;
  int intelligence = 10;
  int wisdom = 10;
  int charisma = 10;

  int sleightOfHand = 0; // Proficiency bonus or skill level

  std::string characterClass = "Commoner";
  std::string race = "Human";

  int GetModifier(int score) const { return (score - 10) / 2; }
};

struct QuestComponent {
  std::string questId;
  std::string description;
  int state = 0; // 0: None, 1: Active, 2: Completed
  std::string targetItem;
};

struct PlayerComponent {
  float speed = 5.0f;
};

struct AIComponent {
  float sightRange = 10.0f;
  float attackRange = 1.5f;
  float attackCooldown = 2.0f;
  float attackTimer = 0.0f;
  bool isAggressive = true;
  float hostileTimer = 0.0f; // Temporary hostility
  // Cone of vision
  float facingDir = 0.0f;  // in degrees, 0 = East
  float coneAngle = 60.0f; // Total angle of the cone
};

struct LootComponent {
  std::vector<PixelsEngine::Item> drops;
};

enum class EntityTag { None, Hostile, NPC, Companion, Trader, Quest };

struct TagComponent {
  EntityTag tag = EntityTag::None;
};

struct LockComponent {
  bool isLocked = true;
  std::string keyName = ""; // Name of the key item required
  int dc = 10;              // Difficulty Class for lockpicking
};

struct HazardComponent {
  enum class Type { Fire };
  Type type = Type::Fire;
  int damage = 0;
  float duration = 10.0f;
  float tickTimer = 0.0f;
};

struct DialogueTree;

struct DialogueComponent {
  std::shared_ptr<DialogueTree> tree;
};

} // namespace PixelsEngine