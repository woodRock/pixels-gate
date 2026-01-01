#include "PixelsGateGame.h"
#include <cmath>
#include <iostream>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "../engine/AnimationSystem.h"
#include "../engine/Components.h"
#include "../engine/Dice.h"
#include "../engine/Input.h"
#include "../engine/Pathfinding.h"
#include "../engine/SaveSystem.h"
#include "../engine/TextureManager.h"
#include "../engine/Tiles.h"
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <filesystem>
#include <functional>

PixelsGateGame::PixelsGateGame()
    : PixelsEngine::Application("Pixels Gate", 800, 600) {
  PixelsEngine::Dice::Init();
}

void PixelsGateGame::OnStart() {
  // Init Config
  PixelsEngine::Config::Init();
  // Init Font
  m_TextRenderer = std::make_unique<PixelsEngine::TextRenderer>(
      GetRenderer(), "assets/font.ttf", 16);

  // Initialize Tooltips
  m_Tooltips["Atk"] = {
      "Attack",     "Strike an enemy with your equipped weapon.",
      "Action",     "Melee/Ranged",
      "Weapon Dmg", "None"};
  m_Tooltips["Jmp"] = {"Jump",
                       "Jump to a target location.",
                       "Bonus Action + 3m",
                       "5m",
                       "Utility",
                       "None"};
  m_Tooltips["Snk"] = {"Sneak",   "Attempt to hide from enemies.",
                       "Action",  "Self",
                       "Stealth", "None"};
  m_Tooltips["Shv"] = {
      "Shove",        "Push a creature away or knock them prone.",
      "Bonus Action", "Melee",
      "1.5m Push",    "ATH/ACR"};
  m_Tooltips["Dsh"] = {"Dash",     "Double your movement speed for this turn.",
                       "Action",   "Self",
                       "Speed x2", "None"};
  m_Tooltips["End"] = {"End Turn", "Conclude your turn in combat.",
                       "None",     "N/A",
                       "None",     "None"};

  m_Tooltips["Fir"] = {"Fireball",
                       "A bright streak flashes from your pointing finger to a "
                       "point you choose within range and then blossoms with a "
                       "low roar into an explosion of flame.",
                       "Action",
                       "18m",
                       "8d6 Fire",
                       "DEX Save"};
  m_Tooltips["Hel"] = {"Heal",
                       "A creature you touch regains a number of hit points "
                       "equal to 1d8 + your spellcasting ability modifier.",
                       "Action",
                       "Melee",
                       "1d8 + Mod Healing",
                       "None"};
  m_Tooltips["Mis"] = {
      "Magic Missile",
      "You create three glowing darts of magical force. Each dart hits a "
      "creature of your choice that you can see within range.",
      "Action",
      "18m",
      "3x (1d4 + 1) Force",
      "None"};
  m_Tooltips["Shd"] = {
      "Shield",
      "An invisible barrier of magical force appears and protects you.",
      "Reaction",
      "Self",
      "+5 AC",
      "None"};

  m_Tooltips["Potion"] = {"Healing Potion",
                          "A character who drinks the magical red liquid in "
                          "this vial regains 2d4 + 2 hit points.",
                          "Bonus Action",
                          "Self",
                          "2d4 + 2 Healing",
                          "None"};
  m_Tooltips["Bread"] = {
      "Bread",        "Simple sustenance. Regains 5 hit points when consumed.",
      "Bonus Action", "Self",
      "5 Healing",    "None"};
  m_Tooltips["Thieves' Tools"] = {
      "Thieves' Tools", "Used to pick locks and disarm traps.",
      "Action",         "Melee",
      "Utility",        "DEX Check"};

  // 1. Setup Level with New Isometric Tileset

  std::string isoTileset = "assets/isometric tileset/spritesheet.png";
  int mapW = 40;
  int mapH = 40;
  m_Level = std::make_unique<PixelsEngine::Tilemap>(GetRenderer(), isoTileset,
                                                    32, 32, mapW, mapH);
  m_Level->SetProjection(PixelsEngine::Projection::Isometric);

  InitCampMap();

  using namespace PixelsEngine::Tiles;

  // --- Structured World Generation ---
  for (int y = 0; y < mapH; ++y) {
    for (int x = 0; x < mapW; ++x) {
      int tile = GRASS;

      // 1. The Inn (Center 20,20) - Highest Priority
      if (x >= 17 && x <= 23 && y >= 17 && y <= 23) {
        // Inn Walls
        if (x == 17 || x == 23 || y == 17 || y == 23) {
          tile = LOGS;
          // Entrance
          if (x == 20 && y == 23)
            tile = DIRT;
        } else {
          tile = SMOOTH_STONE; // Inn Floor (Fixed from water)
        }
      }
      // 2. Non-linear Path Network (DIRT)
      else if ((x == 20) || (y == 20) ||    // Main cross paths
               (x == y) || (x + y == 40) || // Diagonal paths
               (std::sqrt(std::pow(x - 20, 2) + std::pow(y - 20, 2)) < 12.0f &&
                std::sqrt(std::pow(x - 20, 2) + std::pow(y - 20, 2)) >
                    10.5f)) // Circular courtyard path
      {
        tile = DIRT;
      }
      // 3. River (Diagonal)
      else if (std::abs(x - y) < 3) {
        tile = (std::abs(x - y) == 0) ? DEEP_WATER : WATER;
        if ((x + y) % 7 == 0)
          tile = ROCK_ON_WATER;
      }
      // 4. Natural Terrain
      else {
        float noise = std::sin(x * 0.2f) + std::cos(y * 0.2f);
        if (noise > 1.0f)
          tile = GRASS_WITH_BUSH;
        else if (noise < -1.2f)
          tile = DIRT_VARIANT_19;

        // Add variety
        if (tile == GRASS && (x * y) % 13 == 0)
          tile = GRASS_VARIANT_01;
        if (tile == GRASS && (x * y) % 17 == 0)
          tile = FLOWER;
        if (tile == GRASS_WITH_BUSH && (x * y) % 11 == 0)
          tile = BUSH;
      }

      // Boundaries
      if (x == 0 || x == mapW - 1 || y == 0 || y == mapH - 1)
        tile = ROCK;

      m_Level->SetTile(x, y, tile);
    }
  }

  // 2. Setup Player Entity
  m_Player = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(m_Player,
                             PixelsEngine::TransformComponent{20.0f, 20.0f});
  GetRegistry().AddComponent(m_Player, PixelsEngine::PlayerComponent{5.0f});
  GetRegistry().AddComponent(m_Player, PixelsEngine::PathMovementComponent{});
  GetRegistry().AddComponent(
      m_Player, PixelsEngine::StatsComponent{100, 100, 15, false, 0, 1, 1,
                                             false, false, 2, 2});

  auto &inv =
      GetRegistry().AddComponent(m_Player, PixelsEngine::InventoryComponent{});
  inv.AddItem("Potion", 3, PixelsEngine::ItemType::Consumable, 0,
              "assets/ui/item_potion.png", 50);
  inv.AddItem("Thieves' Tools", 1, PixelsEngine::ItemType::Tool, 0,
              "assets/thieves_tools.png", 25);

  std::string playerSheet = "assets/knight.png";
  auto playerTexture =
      PixelsEngine::TextureManager::LoadTexture(GetRenderer(), playerSheet);

  GetRegistry().AddComponent(
      m_Player,
      PixelsEngine::SpriteComponent{playerTexture, {0, 0, 32, 32}, 16, 30});

  auto &anim =
      GetRegistry().AddComponent(m_Player, PixelsEngine::AnimationComponent{});
  anim.AddAnimation("Idle", 0, 0, 32, 32, 1);
  anim.AddAnimation("WalkDown", 0, 0, 32, 32, 1);
  anim.AddAnimation("WalkRight", 0, 0, 32, 32, 1);
  anim.AddAnimation("WalkUp", 0, 0, 32, 32, 1);
  anim.AddAnimation("WalkLeft", 0, 0, 32, 32, 1);

  // Load NPC Textures
  auto innkeeperTex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/npc_innkeeper.png");
  auto guardianTex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/npc_guardian.png");
  auto companionTex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/npc_companion.png");
  auto traderTex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/npc_trader.png");

  // 3. Spawn Boars (Far from Inn)
  CreateBoar(35.0f, 35.0f);
  CreateBoar(32.0f, 5.0f);
  CreateBoar(5.0f, 32.0f);

  // 4. NPCs
  auto npc1 = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(
      npc1, PixelsEngine::TransformComponent{19.0f, 19.0f}); // In Inn
  GetRegistry().AddComponent(npc1, PixelsEngine::SpriteComponent{
                                       playerTexture, {0, 0, 32, 32}, 16, 30});
  GetRegistry().AddComponent(
      npc1, PixelsEngine::InteractionComponent{"Innkeeper", "npc_innkeeper",
                                               false, 0.0f});
  GetRegistry().AddComponent(npc1,
                             PixelsEngine::StatsComponent{50, 50, 5, false});
  GetRegistry().AddComponent(
      npc1, PixelsEngine::QuestComponent{"FetchOrb", 0, "Gold Orb"});
  GetRegistry().AddComponent(
      npc1, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Quest});
  GetRegistry().AddComponent(npc1, PixelsEngine::AIComponent{8.0f, 1.5f, 2.0f,
                                                             0.0f, false, 0.0f,
                                                             90.0f, 90.0f});

  // --- Build Complex Dialogue Tree for Innkeeper ---
  PixelsEngine::DialogueTree innTree;
  innTree.currentEntityName = "Innkeeper";
  innTree.currentNodeId = "start";

  PixelsEngine::DialogueNode startNode;
  startNode.id = "start";
  startNode.npcText =
      "Welcome to the Pixel Inn! What can I do for you today, traveler?";

  // Work Option: Hidden if work topic is closed (accepted or refused)
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "I'm looking for work. [Intelligence DC 10]", "work_check",
      "Intelligence", 10, "work_success", "work_fail",
      PixelsEngine::DialogueAction::None, "", "Inn_Work_Topic_Closed", false));

  // Turn In Option: Visible if Quest Active, Not Done, and Has Item
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "I found the Gold Orb.", "quest_complete", "None", 0, "", "",
      PixelsEngine::DialogueAction::CompleteQuest, "Quest_FetchOrb_Done",
      "Quest_FetchOrb_Done", true, "Quest_FetchOrb_Active", "Gold Orb"));

  // Post-Quest Chatter
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "How are things?", "quest_chat", "None", 0, "", "",
      PixelsEngine::DialogueAction::None, "", "", true, "Quest_FetchOrb_Done"));

  // Standard Options
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "Can I get a discount on a room? [Charisma DC 12]", "room_check",
      "Charisma", 12, "discount_success", "discount_fail",
      PixelsEngine::DialogueAction::None, "", "discount_active", false));
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "I don't like your face. [Attack]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::StartCombat));
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "Just looking around. [End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  innTree.nodes["start"] = startNode;

  PixelsEngine::DialogueNode workSuccess;
  workSuccess.id = "work_success";
  workSuccess.npcText = "A sharp eye! Indeed, the local boars have been "
                        "raiding our stores. Hunt them... wait, actually, I "
                        "lost my lucky Gold Orb in the woods. Find it for me?";
  // Start Quest: Sets Active flag AND Work Topic Closed flag (chained via
  // logic? No, DialogueAction::StartQuest sets ONE flag. I need to set
  // "Inn_Work_Topic_Closed" manually or map "Quest_FetchOrb_Active" to it?
  // Actually, I can just use "Quest_FetchOrb_Active" as the exclude flag for
  // the start option! But what if I refuse? Let's rely on
  // "Quest_FetchOrb_Active" for acceptance. And "refused_work" for refusal. I
  // can't exclude on TWO flags. Workaround: "I'll get right on it." sets
  // "Quest_FetchOrb_Active". AND we can set "Inn_Work_Topic_Closed" via
  // `actionParam`? No, param is used for the flag name. I will simplify: If
  // "Quest_FetchOrb_Active" is true, hide it. If "refused_work" is true, hide
  // it. I can duplicate the option with different exclude flags? No, that shows
  // two. I will use "Inn_Work_Topic_Closed" as the unified flag. So StartQuest
  // needs to set "Quest_FetchOrb_Active". And I also need to set
  // "Inn_Work_Topic_Closed". I can't do two actions. I'll make the "I'm looking
  // for work" check exclude on "Quest_FetchOrb_Active". If I refuse, I just
  // won't set "Quest_FetchOrb_Active" and it might show again? That's fine. If
  // I refuse, maybe I can ask again later. That's actually better game design
  // (retry). So I will exclude on "Quest_FetchOrb_Active" AND
  // "Quest_FetchOrb_Done". Wait, I still have the single string limit.
  // "Quest_FetchOrb_Active" implies I have the job.
  // "Quest_FetchOrb_Done" implies I finished.
  // If I finish, "Active" stays true in my logic? Yes.
  // So excluding "Quest_FetchOrb_Active" covers both phases!
  // So "I'm looking for work" is hidden if "Quest_FetchOrb_Active" is set.

  // Corrected Start Node logic:
  // Exclude: "Quest_FetchOrb_Active"

  // Options in workSuccess:
  // "I'll do it." -> Action: StartQuest, Param: "Quest_FetchOrb_Active".
  workSuccess.options.push_back(PixelsEngine::DialogueOption(
      "I'll find it.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::StartQuest, "Quest_FetchOrb_Active"));
  innTree.nodes["work_success"] = workSuccess;

  PixelsEngine::DialogueNode workFail;
  workFail.id = "work_fail";
  workFail.npcText = "You look a bit... slow. I don't have anything for "
                     "someone who can't tell a boar from a bush.";
  workFail.options.push_back(PixelsEngine::DialogueOption(
      "Hmph.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  innTree.nodes["work_fail"] = workFail;

  PixelsEngine::DialogueNode questComplete;
  questComplete.id = "quest_complete";
  questComplete.npcText = "My Orb! You found it! Thank you so much.";
  questComplete.options.push_back(PixelsEngine::DialogueOption(
      "Happy to help.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  innTree.nodes["quest_complete"] = questComplete;

  PixelsEngine::DialogueNode questChat;
  questChat.id = "quest_chat";
  questChat.npcText = "Business is booming thanks to you!";
  questChat.options.push_back(PixelsEngine::DialogueOption(
      "Glad to hear it.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  innTree.nodes["quest_chat"] = questChat;

  PixelsEngine::DialogueNode discountSuccess;
  discountSuccess.id = "discount_success";
  discountSuccess.npcText =
      "You have a silver tongue! Fine, half price for you tonight.";
  discountSuccess.options.push_back(PixelsEngine::DialogueOption(
      "Much appreciated.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation, "discount_active"));
  innTree.nodes["discount_success"] = discountSuccess;

  PixelsEngine::DialogueNode discountFail;
  discountFail.id = "discount_fail";
  discountFail.npcText = "Nice try, but I have a business to run. Full price.";
  discountFail.options.push_back(PixelsEngine::DialogueOption(
      "Worth a shot. [End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  innTree.nodes["discount_fail"] = discountFail;

  PixelsEngine::DialogueNode
      refusedWork; // Not used anymore if we rely on repeatable=false, but good
                   // to keep structure
  refusedWork.id = "refused_work";
  refusedWork.npcText = "Suit yourself.";
  refusedWork.options.push_back(PixelsEngine::DialogueOption(
      "[End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  innTree.nodes["refused_work"] = refusedWork;

  GetRegistry().AddComponent(
      npc1, PixelsEngine::DialogueComponent{
                std::make_shared<PixelsEngine::DialogueTree>(innTree)});

  auto &n1Inv =
      GetRegistry().AddComponent(npc1, PixelsEngine::InventoryComponent{});
  n1Inv.AddItem("Coins", 100);
  n1Inv.AddItem("Potion", 1, PixelsEngine::ItemType::Consumable, 0, "", 50);

  auto npc2 = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(npc2,
                             PixelsEngine::TransformComponent{20.0f, 25.0f});
  GetRegistry().AddComponent(npc2, PixelsEngine::SpriteComponent{
                                       guardianTex, {0, 0, 32, 32}, 16, 30});
  GetRegistry().AddComponent(
      npc2, PixelsEngine::InteractionComponent{"Guardian", "npc_guardian",
                                               false, 0.0f});
  GetRegistry().AddComponent(npc2,
                             PixelsEngine::StatsComponent{50, 50, 5, false});
  GetRegistry().AddComponent(
      npc2, PixelsEngine::QuestComponent{"HuntBoars", 0, "Boar Meat"});
  GetRegistry().AddComponent(
      npc2, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Quest});
  GetRegistry().AddComponent(npc2, PixelsEngine::AIComponent{8.0f, 1.5f, 2.0f,
                                                             0.0f, false, 0.0f,
                                                             270.0f, 90.0f});

  PixelsEngine::DialogueTree guardTree;
  guardTree.currentEntityName = "Guardian";
  guardTree.currentNodeId = "start";
  PixelsEngine::DialogueNode gStart;
  gStart.id = "start";
  gStart.npcText = "The road ahead is dangerous. You'll need more than that "
                   "look on your face to survive.";
  gStart.options.push_back(PixelsEngine::DialogueOption(
      "I can handle myself. [Intimidate DC 10]", "g_check", "Charisma", 10,
      "g_pass", "g_fail", PixelsEngine::DialogueAction::None, "", "", false));
  gStart.options.push_back(PixelsEngine::DialogueOption(
      "What's out there?", "g_info", "None", 0, "", "",
      PixelsEngine::DialogueAction::None, "", "", false)); // Lore

  // Turn In
  gStart.options.push_back(PixelsEngine::DialogueOption(
      "I have the Boar Meat.", "g_done", "None", 0, "", "",
      PixelsEngine::DialogueAction::CompleteQuest, "Quest_HuntBoars_Done",
      "Quest_HuntBoars_Done", true, "Quest_HuntBoars_Active", "Boar Meat"));

  // Post-Quest
  gStart.options.push_back(PixelsEngine::DialogueOption(
      "The boars are thinning out.", "g_chat", "None", 0, "", "",
      PixelsEngine::DialogueAction::None, "", "", true,
      "Quest_HuntBoars_Done"));

  gStart.options.push_back(
      PixelsEngine::DialogueOption("[Attack]", "end", "None", 0, "", "",
                                   PixelsEngine::DialogueAction::StartCombat));
  gStart.options.push_back(PixelsEngine::DialogueOption(
      "Just passing through. [End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  guardTree.nodes["start"] = gStart;

  PixelsEngine::DialogueNode gPass;
  gPass.id = "g_pass";
  gPass.npcText = "Fine, go die then. Don't say I didn't warn you.";
  gPass.options.push_back(PixelsEngine::DialogueOption(
      "[End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  guardTree.nodes["g_pass"] = gPass;

  PixelsEngine::DialogueNode gFail;
  gFail.id = "g_fail";
  gFail.npcText =
      "Ha! You're shaking like a leaf. Get inside before a boar eats you.";
  gFail.options.push_back(PixelsEngine::DialogueOption(
      "[End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  guardTree.nodes["g_fail"] = gFail;

  PixelsEngine::DialogueNode gInfo;
  gInfo.id = "g_info";
  gInfo.npcText =
      "Boars. Big ones. And worse. Stay on the path if you value your hide.";
  gInfo.options.push_back(PixelsEngine::DialogueOption(
      "I can help hunt them.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::StartQuest, "Quest_HuntBoars_Active"));
  gInfo.options.push_back(PixelsEngine::DialogueOption(
      "Thanks for the warning. [End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  guardTree.nodes["g_info"] = gInfo;

  PixelsEngine::DialogueNode gDone;
  gDone.id = "g_done";
  gDone.npcText =
      "You actually killed one? Hmph. Maybe you're not useless after all.";
  gDone.options.push_back(PixelsEngine::DialogueOption(
      "Told you.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  guardTree.nodes["g_done"] = gDone;

  PixelsEngine::DialogueNode gChat;
  gChat.id = "g_chat";
  gChat.npcText = "Keep up the good work.";
  gChat.options.push_back(PixelsEngine::DialogueOption(
      "[End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  guardTree.nodes["g_chat"] = gChat;

  GetRegistry().AddComponent(
      npc2, PixelsEngine::DialogueComponent{
                std::make_shared<PixelsEngine::DialogueTree>(guardTree)});

  auto &n2Inv =
      GetRegistry().AddComponent(npc2, PixelsEngine::InventoryComponent{});
  n2Inv.AddItem("Coins", 50);
  n2Inv.AddItem("Bread", 5, PixelsEngine::ItemType::Consumable, 0, "", 10);

  // Add a Companion
  auto comp = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(
      comp, PixelsEngine::TransformComponent{21.0f, 21.0f}); // In Inn
  GetRegistry().AddComponent(comp, PixelsEngine::SpriteComponent{
                                       companionTex, {0, 0, 32, 32}, 16, 30});
  GetRegistry().AddComponent(
      comp, PixelsEngine::InteractionComponent{"Companion", "npc_companion",
                                               false, 0.0f});
  GetRegistry().AddComponent(
      comp, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Companion});
  GetRegistry().AddComponent(comp,
                             PixelsEngine::StatsComponent{80, 80, 8, false});
  GetRegistry().AddComponent(
      comp, PixelsEngine::AIComponent{10.0f, 1.5f, 2.0f, 0.0f, false});

  PixelsEngine::DialogueTree compTree;
  compTree.currentEntityName = "Traveler";
  compTree.currentNodeId = "start";
  PixelsEngine::DialogueNode cStart;
  cStart.id = "start";
  cStart.npcText = "Quiet night, isn't it? Thinking of heading out?";
  cStart.options.push_back(PixelsEngine::DialogueOption(
      "Who are you?", "c_info", "None", 0, "", "",
      PixelsEngine::DialogueAction::None, "", "", false));
  cStart.options.push_back(PixelsEngine::DialogueOption(
      "[End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  compTree.nodes["start"] = cStart;

  PixelsEngine::DialogueNode cInfo;
  cInfo.id = "c_info";
  cInfo.npcText =
      "Just a wanderer, like you. I've seen things you wouldn't believe.";
  cInfo.options.push_back(PixelsEngine::DialogueOption(
      "Interesting. [End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  compTree.nodes["c_info"] = cInfo;

  GetRegistry().AddComponent(
      comp, PixelsEngine::DialogueComponent{
                std::make_shared<PixelsEngine::DialogueTree>(compTree)});

  auto &cInv =
      GetRegistry().AddComponent(comp, PixelsEngine::InventoryComponent{});
  cInv.AddItem("Coins", 10);

  // Add a Trader
  auto trader = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(
      trader, PixelsEngine::TransformComponent{18.0f, 21.0f}); // In Inn
  GetRegistry().AddComponent(
      trader,
      PixelsEngine::SpriteComponent{traderTex, {0, 0, 32, 32}, 16, 30});
  GetRegistry().AddComponent(trader, PixelsEngine::InteractionComponent{
                                         "Trader", "npc_trader", false, 0.0f});
  GetRegistry().AddComponent(
      trader, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Trader});
  GetRegistry().AddComponent(trader,
                             PixelsEngine::StatsComponent{100, 100, 10, false});
  GetRegistry().AddComponent(
      trader, PixelsEngine::AIComponent{8.0f, 1.5f, 2.0f, 0.0f, false, 0.0f,
                                        0.0f, 120.0f});

  PixelsEngine::DialogueTree tradeTree;
  tradeTree.currentEntityName = "Merchant";
  tradeTree.currentNodeId = "start";
  PixelsEngine::DialogueNode tStart;
  tStart.id = "start";
  tStart.npcText =
      "Looking for supplies? I've got the best prices in the valley.";
  tStart.options.push_back(PixelsEngine::DialogueOption(
      "Show me your wares. [Trade]", "start", "None", 0, "", "",
      PixelsEngine::DialogueAction::GiveItem));
  tStart.options.push_back(PixelsEngine::DialogueOption(
      "Any news? [Persuasion DC 10]", "t_check", "Charisma", 10, "t_pass",
      "t_fail", PixelsEngine::DialogueAction::None, "", "", false));
  tStart.options.push_back(PixelsEngine::DialogueOption(
      "[End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  tradeTree.nodes["start"] = tStart;

  PixelsEngine::DialogueNode tPass;
  tPass.id = "t_pass";
  tPass.npcText = "Rumor has it there's a golden orb hidden in the forest. "
                  "Worth a fortune.";
  tPass.options.push_back(PixelsEngine::DialogueOption(
      "I'll keep an eye out. [End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  tradeTree.nodes["t_pass"] = tPass;

  PixelsEngine::DialogueNode tFail;
  tFail.id = "t_fail";
  tFail.npcText =
      "Nothing for free, friend. Buy something and maybe I'll talk.";
  tFail.options.push_back(PixelsEngine::DialogueOption(
      "[End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  tradeTree.nodes["t_fail"] = tFail;

  GetRegistry().AddComponent(
      trader, PixelsEngine::DialogueComponent{
                  std::make_shared<PixelsEngine::DialogueTree>(tradeTree)});

  auto &tInv =
      GetRegistry().AddComponent(trader, PixelsEngine::InventoryComponent{});

  // 5. Spawn Gold Orb (Across the river)
  auto orb = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(orb, PixelsEngine::TransformComponent{5.0f, 5.0f});
  m_Level->SetTile(5, 5, GRASS);
  auto orbTexture = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/gold_orb.png");
  GetRegistry().AddComponent(
      orb, PixelsEngine::SpriteComponent{orbTexture, {0, 0, 32, 32}, 16, 16});
  GetRegistry().AddComponent(
      orb, PixelsEngine::InteractionComponent{"Gold Orb", "item_gold_orb",
                                              false, 0.0f});

  // 6. Spawn Locked Chest
  auto chest = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(
      chest, PixelsEngine::TransformComponent{22.0f, 22.0f}); // Inside Inn
  auto chestTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(),
                                                            "assets/chest.png");
  GetRegistry().AddComponent(
      chest, PixelsEngine::SpriteComponent{chestTex, {0, 0, 32, 32}, 16, 16});
  GetRegistry().AddComponent(chest, PixelsEngine::InteractionComponent{
                                        "Old Chest", "obj_chest", false, 0.0f});
  GetRegistry().AddComponent(
      chest,
      PixelsEngine::LockComponent{true, "Chest Key", 15}); // Locked, DC 15

  std::vector<PixelsEngine::Item> chestLoot;
  chestLoot.push_back(
      {"Rare Gem", "", 1, PixelsEngine::ItemType::Misc, 0, 500});
  chestLoot.push_back(
      {"Potion", "", 2, PixelsEngine::ItemType::Consumable, 0, 50});
  GetRegistry().AddComponent(chest, PixelsEngine::LootComponent{chestLoot});

  // 7. Spawn Items (Key and Tools)
  auto keyEnt = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(keyEnt,
                             PixelsEngine::TransformComponent{25.0f, 25.0f});
  auto keyTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(),
                                                          "assets/key.png");
  GetRegistry().AddComponent(
      keyEnt, PixelsEngine::SpriteComponent{keyTex, {0, 0, 32, 32}, 16, 16});
  GetRegistry().AddComponent(keyEnt, PixelsEngine::InteractionComponent{
                                         "Chest Key", "item_key", false, 0.0f});
  GetRegistry().AddComponent(
      keyEnt, PixelsEngine::LootComponent{std::vector<PixelsEngine::Item>{
                  {"Chest Key", "assets/key.png", 1,
                   PixelsEngine::ItemType::Misc, 0, 0}}});

  auto toolsEnt = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(toolsEnt,
                             PixelsEngine::TransformComponent{21.0f, 25.0f});
  auto toolsTex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/thieves_tools.png");
  GetRegistry().AddComponent(toolsEnt, PixelsEngine::SpriteComponent{
                                           toolsTex, {0, 0, 32, 32}, 16, 16});
  GetRegistry().AddComponent(
      toolsEnt, PixelsEngine::InteractionComponent{"Thieves' Tools",
                                                   "item_tools", false, 0.0f});
  GetRegistry().AddComponent(
      toolsEnt, PixelsEngine::LootComponent{std::vector<PixelsEngine::Item>{
                    {"Thieves' Tools", "assets/thieves_tools.png", 1,
                     PixelsEngine::ItemType::Tool, 0, 25}}});
}

// Temporary storage for creation menu
static int tempStats[6] = {10, 10, 10, 10, 10, 10};
static const char *statNames[] = {"Strength",     "Dexterity", "Constitution",
                                  "Intelligence", "Wisdom",    "Charisma"};
static std::string tempClasses[] = {"Fighter", "Rogue", "Wizard", "Cleric"};
static int classIndex = 0;
static std::string tempRaces[] = {"Human", "Elf", "Dwarf", "Halfling"};
static int raceIndex = 0;
static int selectionIndex = 0;
static int pointsRemaining = 5;

void PixelsGateGame::RenderCharacterCreation() {
  SDL_Renderer *renderer = GetRenderer();
  m_TextRenderer->RenderTextCentered("CHARACTER CREATION", 400, 50,
                                     {255, 215, 0, 255});
  m_TextRenderer->RenderTextCentered("Points Remaining: " +
                                         std::to_string(pointsRemaining),
                                     400, 90, {200, 255, 200, 255});

  int y = 130;
  SDL_Color selectedColor = {50, 255, 50, 255};
  SDL_Color normalColor = {255, 255, 255, 255};

  for (int i = 0; i < 6; ++i) {
    std::string text =
        std::string(statNames[i]) + ": " + std::to_string(tempStats[i]);
    SDL_Color c = (selectionIndex == i) ? selectedColor : normalColor;
    m_TextRenderer->RenderTextCentered(text, 400, y, c);
    if (selectionIndex == i) {
      m_TextRenderer->RenderTextCentered(">", 400 - 120, y, selectedColor);
      m_TextRenderer->RenderTextCentered("<", 400 + 120, y, selectedColor);
    }
    y += 35;
  }
  y += 20;
  {
    std::string text = "Class: " + tempClasses[classIndex];
    SDL_Color c = (selectionIndex == 6) ? selectedColor : normalColor;
    m_TextRenderer->RenderTextCentered(text, 400, y, c);
    if (selectionIndex == 6) {
      m_TextRenderer->RenderTextCentered(">", 400 - 150, y, selectedColor);
      m_TextRenderer->RenderTextCentered("<", 400 + 150, y, selectedColor);
    }
  }
  y += 35;
  {
    std::string text = "Race: " + tempRaces[raceIndex];
    SDL_Color c = (selectionIndex == 7) ? selectedColor : normalColor;
    m_TextRenderer->RenderTextCentered(text, 400, y, c);
    if (selectionIndex == 7) {
      m_TextRenderer->RenderTextCentered(">", 400 - 150, y, selectedColor);
      m_TextRenderer->RenderTextCentered("<", 400 + 150, y, selectedColor);
    }
  }
  y += 50;
  SDL_Rect btnRect = {300, y, 200, 50};
  if (selectionIndex == 8)
    SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
  else
    SDL_SetRenderDrawColor(renderer, 50, 150, 50, 255);
  SDL_RenderFillRect(renderer, &btnRect);
  m_TextRenderer->RenderTextCentered("START ADVENTURE", 400, y + 25,
                                     {255, 255, 255, 255});
  m_TextRenderer->RenderTextCentered("Controls: W/S to Select, A/D to Change",
                                     400, 550, {150, 150, 150, 255});
}

void PixelsGateGame::HandleCreationInput() {
  static bool wasUp = false, wasDown = false, wasLeft = false, wasRight = false,
              wasEnter = false;
  bool isUp = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_W);
  bool isDown = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_S);
  bool isLeft = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A);
  bool isRight = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D);
  bool isEnter = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RETURN) ||
                 PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_SPACE);

  bool pressUp = isUp && !wasUp;
  bool pressDown = isDown && !wasDown;
  bool pressLeft = isLeft && !wasLeft;
  bool pressRight = isRight && !wasRight;
  bool pressEnter = isEnter && !wasEnter;

  wasUp = isUp;
  wasDown = isDown;
  wasLeft = isLeft;
  wasRight = isRight;
  wasEnter = isEnter;

  if (pressUp) {
    selectionIndex--;
    if (selectionIndex < 0)
      selectionIndex = 8;
  }
  if (pressDown) {
    selectionIndex++;
    if (selectionIndex > 8)
      selectionIndex = 0;
  }

  if (pressLeft || pressRight) {
    int delta = pressRight ? 1 : -1;
    if (selectionIndex >= 0 && selectionIndex <= 5) {
      int currentVal = tempStats[selectionIndex];
      if (delta > 0) {
        if (pointsRemaining > 0 && currentVal < 18) {
          tempStats[selectionIndex]++;
          pointsRemaining--;
        }
      } else {
        if (currentVal > 8) {
          tempStats[selectionIndex]--;
          pointsRemaining++;
        }
      }
    } else if (selectionIndex == 6)
      classIndex = (classIndex + delta + 4) % 4;
    else if (selectionIndex == 7)
      raceIndex = (raceIndex + delta + 4) % 4;
  }

  if (pressEnter || (selectionIndex == 8 &&
                     (pressRight || PixelsEngine::Input::IsMouseButtonDown(
                                        SDL_BUTTON_LEFT)))) {
    auto *stats =
        GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    if (stats) {
      stats->strength = tempStats[0];
      stats->dexterity = tempStats[1];
      stats->constitution = tempStats[2];
      stats->intelligence = tempStats[3];
      stats->wisdom = tempStats[4];
      stats->charisma = tempStats[5];
      stats->characterClass = tempClasses[classIndex];
      stats->race = tempRaces[raceIndex];
      stats->maxHealth = 10 + stats->GetModifier(stats->constitution) * 2;
      stats->currentHealth = stats->maxHealth;
      stats->damage = (stats->characterClass == "Wizard")
                          ? 12
                          : (2 + stats->GetModifier(stats->strength));
    }
    m_State = GameState::Playing;
  }
}

void PixelsGateGame::PerformAttack(PixelsEngine::Entity forcedTarget) {
  auto *playerStats =
      GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
  auto *playerTrans =
      GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
  if (!playerStats || !playerTrans)
    return;

  if (m_State == GameState::Combat) {
    if (!m_Combat.m_TurnOrder[m_Combat.m_CurrentTurnIndex].isPlayer) {
      SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "Not Your Turn!",
                        {200, 0, 0, 255});
      return;
    }
    if (m_Combat.m_ActionsLeft <= 0) {
      SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f,
                        "No Actions Left!", {200, 0, 0, 255});
      return;
    }
  }

  PixelsEngine::Entity target = forcedTarget;

  // 1. Try Context Menu Target
  if (target == PixelsEngine::INVALID_ENTITY && m_ContextMenu.isOpen &&
      m_ContextMenu.targetEntity != PixelsEngine::INVALID_ENTITY) {
    target = m_ContextMenu.targetEntity;
  }

  // 2. If no target, find closest enemy
  if (target == PixelsEngine::INVALID_ENTITY) {
    float minDistance = 3.0f; // Increased from 2.0f for better usability
    auto &view = GetRegistry().View<PixelsEngine::StatsComponent>();
    for (auto &[entity, stats] : view) {
      if (entity == m_Player || stats.isDead)
        continue;
      auto *targetTrans =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
      if (targetTrans) {
        float dist = std::sqrt(std::pow(playerTrans->x - targetTrans->x, 2) +
                               std::pow(playerTrans->y - targetTrans->y, 2));
        if (dist < minDistance) {
          minDistance = dist;
          target = entity;
        }
      }
    }
  }

  if (target != PixelsEngine::INVALID_ENTITY) {
    // If not in combat, start combat!
    if (m_State == GameState::Playing) {
      auto *ai = GetRegistry().GetComponent<PixelsEngine::AIComponent>(target);
      if (ai) {
        StartCombat(target);
      } else {
        StartCombat(PixelsEngine::INVALID_ENTITY);
      }
    }

    auto *targetStats =
        GetRegistry().GetComponent<PixelsEngine::StatsComponent>(target);
    auto *inv =
        GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if (targetStats && inv) {
      if (targetStats->isDead) {
        SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "Already Dead",
                          {200, 200, 200, 255});
        return;
      }
      // Select active weapon
      PixelsEngine::Item &activeWeapon = (m_SelectedWeaponSlot == 0)
                                             ? inv->equippedMelee
                                             : inv->equippedRanged;
      float maxRange = (m_SelectedWeaponSlot == 0)
                           ? 3.0f
                           : 10.0f; // Bow has much higher range

      // Check range for forced target too
      auto *targetTrans =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(target);
      float dist =
          targetTrans ? std::sqrt(std::pow(playerTrans->x - targetTrans->x, 2) +
                                  std::pow(playerTrans->y - targetTrans->y, 2))
                      : 100.0f;

      if (dist > maxRange) {
        SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "Too Far!",
                          {200, 200, 200, 255});
        return;
      }

      // Hit Roll against AC (Assuming NPCs have default 10 AC + Dex mod for
      // now)
      int targetAC = 10 + targetStats->GetModifier(targetStats->dexterity);

      int roll = PixelsEngine::Dice::Roll(20);
      if (playerStats->hasAdvantage) {
        int secondRoll = PixelsEngine::Dice::Roll(20);
        roll = std::max(roll, secondRoll);
        playerStats->hasAdvantage = false; // Consume advantage
        playerStats->isStealthed = false;  // Break stealth
        SpawnFloatingText(playerTrans->x, playerTrans->y, "Advantage!",
                          {0, 255, 0, 255});
      }

      int attackBonus = (m_SelectedWeaponSlot == 0)
                            ? playerStats->GetModifier(playerStats->strength)
                            : playerStats->GetModifier(playerStats->dexterity);
      attackBonus += activeWeapon.statBonus;

      if (roll == 20 || (roll != 1 && roll + attackBonus >= targetAC)) {
        // Hit!
        int dmg = playerStats->damage + activeWeapon.statBonus;
        bool isCrit = (roll == 20);
        if (isCrit)
          dmg *= 2;

        targetStats->currentHealth -= dmg;

        if (targetTrans) {
          std::string dmgText = std::to_string(dmg);
          if (isCrit)
            dmgText += "!";
          SpawnFloatingText(targetTrans->x, targetTrans->y, dmgText,
                            isCrit ? SDL_Color{255, 0, 0, 255}
                                   : SDL_Color{255, 255, 255, 255});
        }

        // --- Witness Logic ---
        auto *targetTag =
            GetRegistry().GetComponent<PixelsEngine::TagComponent>(target);
        auto *targetAI =
            GetRegistry().GetComponent<PixelsEngine::AIComponent>(target);

        // A crime is attacking a non-hostile NPC
        bool isCrime =
            (targetTag && (targetTag->tag == PixelsEngine::EntityTag::NPC ||
                           targetTag->tag == PixelsEngine::EntityTag::Trader ||
                           targetTag->tag == PixelsEngine::EntityTag::Quest));
        if (targetAI && targetAI->isAggressive)
          isCrime = false; // Self-defense is not a crime

        if (isCrime) {
          auto &aiView = GetRegistry().View<PixelsEngine::AIComponent>();
          for (auto &[witness, wai] : aiView) {
            if (witness == m_Player || witness == target)
              continue;
            auto *wTrans =
                GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
                    witness);
            if (!wTrans)
              continue;

            // Check if witness sees the player (criminal)
            float dx = playerTrans->x - wTrans->x;
            float dy = playerTrans->y - wTrans->y;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d <= wai.sightRange) {
              auto *wStats =
                  GetRegistry().GetComponent<PixelsEngine::StatsComponent>(
                      witness);
              if (wStats && wStats->isDead)
                continue; // Dead don't witness crime

              float angle = std::atan2(dy, dx) * (180.0f / M_PI);
              float diff = angle - wai.facingDir;
              while (diff > 180.0f)
                diff -= 360.0f;
              while (diff < -180.0f)
                diff += 360.0f;

              if (std::abs(diff) <= wai.coneAngle / 2.0f) {
                // Witnessed!
                wai.isAggressive = true;
                wai.hostileTimer = 30.0f; // Hostile for 30 seconds
                SpawnFloatingText(wTrans->x, wTrans->y, "Halt criminal!",
                                  {255, 0, 0, 255});

                // Join combat if not already
                if (m_State == GameState::Combat) {
                  // Add to turn order if not there
                  bool inTurnOrder = false;
                  for (auto &t : m_Combat.m_TurnOrder)
                    if (t.entity == witness)
                      inTurnOrder = true;
                  if (!inTurnOrder) {
                    auto *wStats =
                        GetRegistry()
                            .GetComponent<PixelsEngine::StatsComponent>(
                                witness);
                    int init =
                        PixelsEngine::Dice::Roll(20) +
                        (wStats ? wStats->GetModifier(wStats->dexterity) : 0);
                    m_Combat.m_TurnOrder.push_back({witness, init, false});
                    // Sort needs to happen but simplified here
                  }
                }
              }
            }
          }
        }
      } else {
        // Miss
        if (targetTrans)
          SpawnFloatingText(targetTrans->x, targetTrans->y, "Miss",
                            {200, 200, 200, 255});
      }

      if (m_State == GameState::Combat) {
        m_Combat.m_ActionsLeft--;
      }

      if (targetStats->currentHealth <= 0) {
        targetStats->currentHealth = 0;
        targetStats->isDead = true;

        // Consolidate all items (Inventory + Drops) into LootComponent
        auto *inv =
            GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
                target);
        auto *loot =
            GetRegistry().GetComponent<PixelsEngine::LootComponent>(target);

        if (!loot) {
          loot = &GetRegistry().AddComponent(target,
                                             PixelsEngine::LootComponent{});
        }

        if (inv) {
          for (auto &item : inv->items)
            loot->drops.push_back(item);
          inv->items.clear();
        }

        // Add special icon or change sprite to indicate dead?
        // For now, we just keep the entity but stop its AI and logic
        auto *ai =
            GetRegistry().GetComponent<PixelsEngine::AIComponent>(target);
        if (ai)
          ai->isAggressive = false;

        // Immediate Victory Check
        if (m_State == GameState::Combat) {
          bool enemiesAlive = false;
          for (const auto &turn : m_Combat.m_TurnOrder) {
            if (!turn.isPlayer) {
              auto *s =
                  GetRegistry().GetComponent<PixelsEngine::StatsComponent>(
                      turn.entity);
              if (s && !s->isDead)
                enemiesAlive = true;
            }
          }
          if (!enemiesAlive) {
            EndCombat();
          }
        }
      }
    }
  } else {
    // Whiff
    SpawnFloatingText(playerTrans->x, playerTrans->y - 1.0f, "No Target",
                      {200, 200, 200, 255});
  }
}

void PixelsGateGame::CreateBoar(float x, float y) {
  auto boar = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(boar, PixelsEngine::TransformComponent{x, y});
  GetRegistry().AddComponent(
      boar, PixelsEngine::StatsComponent{30, 30, 2,
                                         false}); // Reduced damage from 5 to 2

  static int boarCount = 0;
  GetRegistry().AddComponent(
      boar,
      PixelsEngine::InteractionComponent{
          "Boar", "npc_boar_" + std::to_string(boarCount++), false, 0.0f});

  GetRegistry().AddComponent(
      boar, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Hostile});
  // Add AI
  GetRegistry().AddComponent(
      boar, PixelsEngine::AIComponent{8.0f, 1.2f, 2.0f, 0.0f, true});
  // Add Loot
  std::vector<PixelsEngine::Item> drops;
  drops.push_back(
      {"Boar Meat", "", 1, PixelsEngine::ItemType::Consumable, 0, 25});
  GetRegistry().AddComponent(boar, PixelsEngine::LootComponent{drops});

  auto tex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/critters/boar/boar_SE_run_strip.png");
  GetRegistry().AddComponent(
      boar, PixelsEngine::SpriteComponent{tex, {0, 0, 41, 25}, 20, 20});
  auto &anim =
      GetRegistry().AddComponent(boar, PixelsEngine::AnimationComponent{});
  anim.AddAnimation("Idle", 0, 0, 41, 25, 1);
  anim.AddAnimation("Run", 0, 0, 41, 25, 4);
  anim.Play("Idle");
}

void PixelsGateGame::OnUpdate(float deltaTime) {
  if (m_State == GameState::Creation) {
    HandleCreationInput();
    return;
  }

  // Check for Load Completion
  if (m_State == GameState::Loading) {
    if (m_LoadFuture.valid() && m_LoadFuture.wait_for(std::chrono::seconds(
                                    0)) == std::future_status::ready) {
      m_LoadFuture.get(); // Join thread

      m_FadeState = FadeState::FadingIn;
      m_FadeTimer = m_FadeDuration;

      m_State = m_LoadedIsCamp ? GameState::Camp : GameState::Playing;

      // Ensure player is not marked as dead after loading
      auto *pStats =
          GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
      if (pStats && pStats->currentHealth > 0) {
        pStats->isDead = false;
      }
    }
    return;
  }

  // Update Timers
  if (m_SaveMessageTimer > 0.0f)
    m_SaveMessageTimer -= deltaTime;

  // Update Hazards
  auto &hazards = GetRegistry().View<PixelsEngine::HazardComponent>();
  std::vector<PixelsEngine::Entity> hazardsDestroy;
  for (auto &[hEnt, haz] : hazards) {
    haz.duration -= deltaTime;
    if (haz.duration <= 0.0f) {
      hazardsDestroy.push_back(hEnt);
      continue;
    }

    haz.tickTimer += deltaTime;
    if (haz.tickTimer >= 1.0f) {
      haz.tickTimer = 0.0f;
      auto *hTrans =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(hEnt);
      if (hTrans) {
        // Check for entities in radius
        auto &victims = GetRegistry().View<PixelsEngine::StatsComponent>();
        for (auto &[vEnt, vStats] : victims) {
          if (vStats.isDead)
            continue;
          auto *vTrans =
              GetRegistry().GetComponent<PixelsEngine::TransformComponent>(vEnt);
          if (vTrans) {
            float d = std::sqrt(std::pow(vTrans->x - hTrans->x, 2) +
                                std::pow(vTrans->y - hTrans->y, 2));
            if (d < 1.5f) {
              // Deal Damage
              vStats.currentHealth -= haz.damage;
              SpawnFloatingText(vTrans->x, vTrans->y,
                                "-" + std::to_string(haz.damage) + " (Fire)",
                                {255, 100, 0, 255});

              // Check Item Destruction
              auto *vLoot =
                  GetRegistry().GetComponent<PixelsEngine::LootComponent>(vEnt);
              bool isItem =
                  vLoot &&
                  !GetRegistry().GetComponent<PixelsEngine::PlayerComponent>(vEnt) &&
                  !GetRegistry().GetComponent<PixelsEngine::AIComponent>(vEnt);

              if (isItem && !vLoot->drops.empty()) {
                PixelsEngine::Item &item = vLoot->drops[0];
                if (item.type == PixelsEngine::ItemType::Consumable &&
                    item.name == "Potion") {
                  SpawnFloatingText(vTrans->x, vTrans->y, "Explosion!",
                                    {255, 50, 0, 255});
                  vStats.currentHealth = 0;
                } else if (item.type == PixelsEngine::ItemType::Scroll) {
                  SpawnFloatingText(vTrans->x, vTrans->y, "Burnt!",
                                    {100, 100, 100, 255});
                  vStats.currentHealth = 0;
                }
              }

              if (vStats.currentHealth <= 0) {
                vStats.isDead = true;
                if (isItem) {
                  auto *sprite =
                      GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(vEnt);
                  if (sprite)
                    sprite->texture = nullptr; // Disappear
                }
              }
            }
          }
        }
      }
    }
  }
  for (auto h : hazardsDestroy)
    GetRegistry().DestroyEntity(h);

  // Update Day/Night
  UpdateDayNight(deltaTime);

  // Environmental Damage (Real-time)
  m_EnvironmentDamageTimer += deltaTime;
  if (m_EnvironmentDamageTimer >= 2.0f) {
    m_EnvironmentDamageTimer = 0.0f;
    auto *pTrans =
        GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    auto *pStats =
        GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    auto *currentMap =
        (m_State == GameState::Camp) ? m_CampLevel.get() : m_Level.get();

    if (pTrans && pStats && !pStats->isDead && currentMap) {
      int tile = currentMap->GetTile((int)pTrans->x, (int)pTrans->y);
      if (tile == 28) { // WATER (acting as environmental hazard)
        pStats->currentHealth -= 2;
        SpawnFloatingText(pTrans->x, pTrans->y, "-2 (Environment)",
                          {0, 255, 255, 255});
        if (pStats->currentHealth <= 0)
          pStats->isDead = true;
      }
    }
  }

  // Update Floating Texts
  m_FloatingText.Update(deltaTime);

  if (m_FadeState != FadeState::None) {
    m_FadeTimer -= deltaTime;
    if (m_FadeState == FadeState::FadingOut) {
      if (m_FadeTimer <= 0.0f) {
        // Start Threaded Load
        m_State = GameState::Loading;
        m_LoadFuture = std::async(std::launch::async, [this]() {
          float lx = 0.0f, ly = 0.0f;
          PixelsEngine::SaveSystem::LoadGame(m_PendingLoadFile, GetRegistry(),
                                             m_Player, *m_Level, m_LoadedIsCamp,
                                             lx, ly);
          m_LastWorldPos.x = lx;
          m_LastWorldPos.y = ly;
          PixelsEngine::SaveSystem::LoadWorldFlags(m_PendingLoadFile,
                                                   m_WorldFlags);
        });
        return;
      }
    } else if (m_FadeState == FadeState::FadingIn) {
      if (m_FadeTimer <= 0.0f) {
        m_FadeState = FadeState::None;
      }
    }
    return;
  }

  switch (m_State) {
  case GameState::MainMenu:
    HandleMainMenuInput();
    break;
  case GameState::Creation:
    HandleCreationInput();
    break;
  case GameState::Paused:
    HandlePauseMenuInput();
    break;
  case GameState::Options:
    HandleOptionsInput();
    break;
  case GameState::Credits:
  case GameState::Controls:
    break;
  case GameState::GameOver:
    HandleGameOverInput();
    break;
  case GameState::Map:
    HandleMapInput();
    break;
  case GameState::CharacterMenu:
    HandleCharacterMenuInput();
    break;
  case GameState::RestMenu:
    HandleRestMenuInput();
    break;
  case GameState::Trading:
    HandleTradeInput();
    break;
  case GameState::KeybindSettings:
    HandleKeybindInput();
    break;
  case GameState::Looting:
    HandleLootInput();
    break;
  case GameState::Dialogue:
    HandleDialogueInput();
    break;
  case GameState::Loading:
    break;
  case GameState::Targeting:
    HandleTargetingInput();
    break;
  case GameState::TargetingJump:
    HandleTargetingJumpInput();
    break;
  case GameState::TargetingShove:
    HandleTargetingShoveInput();
    break;
  case GameState::Camp:
  case GameState::Playing:
  case GameState::Combat: {
    auto *pStats =
        GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    if (pStats && pStats->currentHealth <= 0) {
      m_State = GameState::GameOver;
      m_MenuSelection = 0;
      return;
    }

    auto escKey =
        PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Pause);
    auto invKey =
        PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Inventory);
    auto mapKey =
        PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Map);
    auto chrKey =
        PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Character);
    auto magKey =
        PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Magic);
    auto jumpKey =
        PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Jump);
    auto sneakKey =
        PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Sneak);
    auto shoveKey =
        PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Shove);
    auto dashKey =
        PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Dash);
    auto toggleKey = PixelsEngine::Config::GetKeybind(
        PixelsEngine::GameAction::ToggleWeapon);

    // Tooltip Pinning
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_T) &&
        !m_HoveredItemName.empty()) {
      m_TooltipPinned = !m_TooltipPinned;
      if (m_TooltipPinned) {
        PixelsEngine::Input::GetMousePosition(m_PinnedTooltipX,
                                              m_PinnedTooltipY);
      }
    }
    if (m_TooltipPinned &&
        PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
      m_TooltipPinned = false;
    }

    if (PixelsEngine::Input::IsKeyPressed(escKey)) {
      if (m_State == GameState::CharacterMenu || m_State == GameState::Map ||
          m_State == GameState::Targeting ||
          m_State == GameState::TargetingJump ||
          m_State == GameState::TargetingShove) {
        m_State = m_ReturnState;
      } else {
        m_State = GameState::Paused;
        m_MenuSelection = 0;
      }
    }
    if (PixelsEngine::Input::IsKeyPressed(invKey)) {
      if (m_State == GameState::CharacterMenu && m_CharacterTab == 0) {
        m_State = m_ReturnState;
      } else {
        m_ReturnState =
            (m_State == GameState::CharacterMenu) ? m_ReturnState : m_State;
        m_State = GameState::CharacterMenu;
        m_CharacterTab = 0;
      }
    }
    if (PixelsEngine::Input::IsKeyPressed(chrKey)) {
      if (m_State == GameState::CharacterMenu && m_CharacterTab == 1) {
        m_State = m_ReturnState;
      } else {
        m_ReturnState =
            (m_State == GameState::CharacterMenu) ? m_ReturnState : m_State;
        m_State = GameState::CharacterMenu;
        m_CharacterTab = 1;
      }
    }
    if (PixelsEngine::Input::IsKeyPressed(magKey)) {
      if (m_State == GameState::CharacterMenu && m_CharacterTab == 2) {
        m_State = m_ReturnState;
      } else {
        m_ReturnState =
            (m_State == GameState::CharacterMenu) ? m_ReturnState : m_State;
        m_State = GameState::CharacterMenu;
        m_CharacterTab = 2;
      }
    }
    if (PixelsEngine::Input::IsKeyPressed(mapKey)) {
      if (m_State == GameState::Map && m_MapTab == 0) {
        m_State = m_ReturnState;
      } else {
        m_ReturnState = (m_State == GameState::Map) ? m_ReturnState : m_State;
        m_State = GameState::Map;
        m_MapTab = 0;
      }
    }
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_J)) {
      if (m_State == GameState::Map && m_MapTab == 1) {
        m_State = m_ReturnState;
      } else {
        m_ReturnState = (m_State == GameState::Map) ? m_ReturnState : m_State;
        m_State = GameState::Map;
        m_MapTab = 1;
      }
    }

    if (m_State == GameState::Combat) {
      UpdateCombat(deltaTime);
    } else if (m_State == GameState::Playing || m_State == GameState::Camp) {
      HandleInput();
      UpdateAI(deltaTime); // Update AI
    }

    // --- New Actions ---
    if (PixelsEngine::Input::IsKeyPressed(toggleKey)) {
      m_SelectedWeaponSlot = (m_SelectedWeaponSlot == 0) ? 1 : 0;
      SpawnFloatingText(
          0, 0, (m_SelectedWeaponSlot == 0) ? "Melee Mode" : "Ranged Mode",
          {200, 200, 255, 255});
    }
    if (PixelsEngine::Input::IsKeyPressed(jumpKey)) {
      m_ReturnState = m_State;
      m_State = GameState::TargetingJump;
    }
    if (PixelsEngine::Input::IsKeyPressed(shoveKey)) {
      m_ReturnState = m_State;
      m_State = GameState::TargetingShove;
    }
    if (PixelsEngine::Input::IsKeyPressed(sneakKey) && pStats) {
      // Sneak logic (Hide) - Uses Action
      if (m_State == GameState::Combat) {
        if (m_Combat.m_ActionsLeft > 0) {
          pStats->isStealthed = true;
          pStats->hasAdvantage = true;
          m_Combat.m_ActionsLeft--;
          SpawnFloatingText(0, 0, "Hidden!", {150, 150, 150, 255});
        } else {
          SpawnFloatingText(0, 0, "No Actions!", {255, 0, 0, 255});
        }
      } else {
        pStats->isStealthed = !pStats->isStealthed;
        if (pStats->isStealthed)
          SpawnFloatingText(0, 0, "Sneaking...", {150, 150, 150, 255});
      }
    }
    if (PixelsEngine::Input::IsKeyPressed(dashKey)) {
      // Dash - Uses Action
      if (m_State == GameState::Combat) {
        if (m_Combat.m_ActionsLeft > 0) {
          m_Combat.m_MovementLeft += 5.0f; // Approx double
          m_Combat.m_ActionsLeft--;
          SpawnFloatingText(0, 0, "Dashed!", {255, 255, 0, 255});
        } else {
          SpawnFloatingText(0, 0, "No Actions!", {255, 0, 0, 255});
        }
      }
    }

    if (m_State == GameState::Combat) {
      UpdateCombat(deltaTime);
    } else if (m_State == GameState::Playing || m_State == GameState::Camp) {
      HandleInput();
      UpdateAI(deltaTime); // Update AI

      auto *transform =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
              m_Player);
      auto *playerComp =
          GetRegistry().GetComponent<PixelsEngine::PlayerComponent>(m_Player);
      auto *anim = GetRegistry().GetComponent<PixelsEngine::AnimationComponent>(
          m_Player);
      auto *sprite =
          GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(m_Player);
      auto *pathComp =
          GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(
              m_Player);
      auto *currentMap =
          (m_State == GameState::Camp) ? m_CampLevel.get() : m_Level.get();

      if (m_SelectedNPC != PixelsEngine::INVALID_ENTITY) {
        auto *npcTransform =
            GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
                m_SelectedNPC);
        auto *interaction =
            GetRegistry().GetComponent<PixelsEngine::InteractionComponent>(
                m_SelectedNPC);
        if (npcTransform && interaction && currentMap) {
          float dist = std::sqrt(std::pow(transform->x - npcTransform->x, 2) +
                                 std::pow(transform->y - npcTransform->y, 2));
          if (dist < 1.5f) {
            pathComp->isMoving = false;
            anim->currentFrameIndex = 0;
            if (interaction->dialogueText == "Gold Orb") {
              auto *inv =
                  GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
                      m_Player);
              if (inv) {
                inv->AddItem("Gold Orb", 1);
                // Use DestroyEntity to ensure it's removed from the world and
                // save file
                GetRegistry().DestroyEntity(m_SelectedNPC);
                m_SelectedNPC = PixelsEngine::INVALID_ENTITY;
              }
            } else if (interaction->dialogueText == "Loot" ||
                       interaction->dialogueText == "Chest Key" ||
                       interaction->dialogueText == "Thieves' Tools") {
              // Loot Bag or Item Pickup
              auto *inv =
                  GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
                      m_Player);
              auto *loot =
                  GetRegistry().GetComponent<PixelsEngine::LootComponent>(
                      m_SelectedNPC);
              if (inv && loot) {
                for (const auto &item : loot->drops) {
                  inv->AddItemObject(item);
                  SpawnFloatingText(transform->x, transform->y,
                                    "+ " + item.name, {255, 255, 0, 255});
                }
                GetRegistry().DestroyEntity(
                    m_SelectedNPC); // Destroy bag or item
              }
            } else {
              auto *quest =
                  GetRegistry().GetComponent<PixelsEngine::QuestComponent>(
                      m_SelectedNPC);
              if (quest) {
                auto *inv = GetRegistry()
                                .GetComponent<PixelsEngine::InventoryComponent>(
                                    m_Player);
                if (quest->state == 0) {
                  if (quest->questId == "FetchOrb")
                    interaction->dialogueText = "Please find my Gold Orb!";
                  else if (quest->questId == "HuntBoars")
                    interaction->dialogueText =
                        "The boars are aggressive. Hunt them!";
                  quest->state = 1;
                } else if (quest->state == 1) {
                  bool hasItem = false;
                  for (auto &item : inv->items) {
                    if (item.name == quest->targetItem && item.quantity > 0)
                      hasItem = true;
                  }
                  if (hasItem) {
                    interaction->dialogueText = "You found it! Thank you.";
                    quest->state = 2;

                    // 1. Remove Item from Player
                    auto it = inv->items.begin();
                    while (it != inv->items.end()) {
                      if (it->name == quest->targetItem) {
                        PixelsEngine::Item removedItem = *it;
                        removedItem.quantity = 1;
                        it->quantity--;
                        if (it->quantity <= 0)
                          it = inv->items.erase(it);
                        else
                          ++it;

                        // 2. Add Item to NPC
                        auto *nInv =
                            GetRegistry()
                                .GetComponent<PixelsEngine::InventoryComponent>(
                                    m_SelectedNPC);
                        if (nInv)
                          nInv->AddItemObject(removedItem);
                        break;
                      } else
                        ++it;
                    }

                    // 3. Gold Reward from NPC (if they have it)
                    int reward = 50;
                    auto *nInv =
                        GetRegistry()
                            .GetComponent<PixelsEngine::InventoryComponent>(
                                m_SelectedNPC);
                    if (nInv) {
                      auto nIt = nInv->items.begin();
                      while (nIt != nInv->items.end()) {
                        if (nIt->name == "Coins") {
                          int actualReward = std::min(nIt->quantity, reward);
                          nIt->quantity -= actualReward;
                          inv->AddItem("Coins", actualReward);
                          if (nIt->quantity <= 0)
                            nInv->items.erase(nIt);
                          break;
                        } else
                          ++nIt;
                      }
                    } else {
                      inv->AddItem("Coins", reward); // Fallback
                    }

                    // 4. XP Reward
                    auto *pStats =
                        GetRegistry()
                            .GetComponent<PixelsEngine::StatsComponent>(
                                m_Player);
                    if (pStats) {
                      pStats->experience += 100;
                      SpawnFloatingText(transform->x, transform->y, "+100 XP",
                                        {0, 255, 255, 255});
                    }
                  } else {
                    if (quest->questId == "FetchOrb")
                      interaction->dialogueText = "Bring me the Orb...";
                    else if (quest->questId == "HuntBoars")
                      interaction->dialogueText = "Bring me Boar Meat.";
                  }
                } else if (quest->state == 2)
                  interaction->dialogueText = "Blessings upon you.";
              }
              interaction->showDialogue = true;
              interaction->dialogueTimer = 3.0f;
            }
            m_SelectedNPC = PixelsEngine::INVALID_ENTITY;
          }
        }
      }

      auto &interactions =
          GetRegistry().View<PixelsEngine::InteractionComponent>();
      for (auto &[entity, interact] : interactions) {
        if (interact.showDialogue) {
          interact.dialogueTimer -= deltaTime;
          if (interact.dialogueTimer <= 0)
            interact.showDialogue = false;
        }
      }

      if (transform && playerComp && anim && sprite && pathComp && currentMap) {

        // Update Fog of War based on Player Position
        currentMap->UpdateVisibility((int)transform->x, (int)transform->y,
                                     5); // 5 tile radius

        bool wasdInput = false;
        float dx = 0, dy = 0;
        std::string newAnim = "Idle";

        if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_W)) {
          dx -= 1;
          dy -= 1;
          newAnim = "WalkUp";
          wasdInput = true;
        }
        if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_S)) {
          dx += 1;
          dy += 1;
          newAnim = "WalkDown";
          wasdInput = true;
        }
        if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A)) {
          dx -= 1;
          dy += 1;
          newAnim = "WalkLeft";
          wasdInput = true;
        }
        if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D)) {
          dx += 1;
          dy -= 1;
          newAnim = "WalkRight";
          wasdInput = true;
        }

        if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_A))
          sprite->flip = SDL_FLIP_HORIZONTAL;
        else if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_D))
          sprite->flip = SDL_FLIP_NONE;

        if (wasdInput) {
          pathComp->isMoving = false;
          m_SelectedNPC = PixelsEngine::INVALID_ENTITY;
          float newX = transform->x + dx * playerComp->speed * deltaTime;
          float newY = transform->y + dy * playerComp->speed * deltaTime;
          if (currentMap->IsWalkable((int)newX, (int)newY)) {
            transform->x = newX;
            transform->y = newY;
          } else {
            if (currentMap->IsWalkable((int)newX, (int)transform->y))
              transform->x = newX;
            else if (currentMap->IsWalkable((int)transform->x, (int)newY))
              transform->y = newY;
          }
          anim->Play(newAnim);
        } else if (pathComp->isMoving) {
          float targetX = pathComp->targetX, targetY = pathComp->targetY;
          float diffX = targetX - transform->x, diffY = targetY - transform->y;
          float dist = std::sqrt(diffX * diffX + diffY * diffY);
          if (dist < 0.1f) {
            transform->x = targetX;
            transform->y = targetY;
            pathComp->currentPathIndex++;
            if (pathComp->currentPathIndex >= pathComp->path.size()) {
              pathComp->isMoving = false;
              anim->currentFrameIndex = 0;
            } else {
              pathComp->targetX =
                  (float)pathComp->path[pathComp->currentPathIndex].first;
              pathComp->targetY =
                  (float)pathComp->path[pathComp->currentPathIndex].second;
            }
          } else {
            float moveX = (diffX / dist) * playerComp->speed * deltaTime;
            float moveY = (diffY / dist) * playerComp->speed * deltaTime;
            transform->x += moveX;
            transform->y += moveY;
            if (diffX < 0 && diffY < 0)
              anim->Play("WalkUp");
            else if (diffX > 0 && diffY > 0)
              anim->Play("WalkDown");
            else if (diffX < 0 && diffY > 0) {
              anim->Play("WalkLeft");
              sprite->flip = SDL_FLIP_HORIZONTAL;
            } else if (diffX > 0 && diffY < 0) {
              anim->Play("WalkRight");
              sprite->flip = SDL_FLIP_NONE;
            }
          }
        } else
          anim->currentFrameIndex = 0;

        if (wasdInput || pathComp->isMoving) {
          anim->timer += deltaTime;
          auto &currentAnim = anim->animations[anim->currentAnimationIndex];
          if (anim->timer >= currentAnim.frameDuration) {
            anim->timer = 0.0f;
            anim->currentFrameIndex =
                (anim->currentFrameIndex + 1) % currentAnim.frames.size();
          }
          sprite->srcRect = currentAnim.frames[anim->currentFrameIndex];
        } else {
          auto &currentAnim = anim->animations[anim->currentAnimationIndex];
          sprite->srcRect = currentAnim.frames[0];
        }

        int screenX, screenY;
        currentMap->GridToScreen(transform->x, transform->y, screenX, screenY);
        auto &camera = GetCamera();
        camera.x = screenX - camera.width / 2;
        camera.y = screenY - camera.height / 2;
      }
    }
  } break;
  }
}

void PixelsGateGame::OnRender() {
  switch (m_State) {
  case GameState::MainMenu:
    RenderMainMenu();
    break;
  case GameState::Creation:
    RenderCharacterCreation();
    break;
  case GameState::Options:
    RenderOptions();
    break;
  case GameState::Credits:
    RenderCredits();
    break;
  case GameState::Controls:
    RenderControls();
    break;
  case GameState::Loading: {
    SDL_Renderer *renderer = GetRenderer();
    int w, h;
    SDL_GetWindowSize(m_Window, &w, &h);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect screen = {0, 0, w, h};
    SDL_RenderFillRect(renderer, &screen);
    m_TextRenderer->RenderTextCentered("Loading...", w / 2, h / 2,
                                       {255, 255, 255, 255});
  } break;
  case GameState::Paused:
  case GameState::Camp:
  case GameState::Playing:
  case GameState::Combat:
  case GameState::GameOver:
  case GameState::Map:
  case GameState::CharacterMenu:
  case GameState::Targeting:
  case GameState::TargetingJump:
  case GameState::TargetingShove:
  case GameState::Dialogue:
  case GameState::RestMenu:
  case GameState::Trading:
  case GameState::KeybindSettings:
  case GameState::Looting: {
    auto &camera = GetCamera();
    auto *currentMap =
        (m_State == GameState::Camp) ? m_CampLevel.get() : m_Level.get();
    if (currentMap)
      currentMap->Render(camera);

    RenderEnemyCones(camera);

    struct Renderable {
      int y;
      PixelsEngine::Entity entity;
    };
    std::vector<Renderable> renderQueue;
    auto &sprites = GetRegistry().View<PixelsEngine::SpriteComponent>();
    for (auto &[entity, sprite] : sprites) {
      auto *transform =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
      if (transform && currentMap) {
        // Filter entities by map state
        if (m_State == GameState::Camp) {
          if (entity != m_Player) {
            auto *tag =
                GetRegistry().GetComponent<PixelsEngine::TagComponent>(entity);
            if (!tag || tag->tag != PixelsEngine::EntityTag::Companion)
              continue;
          }
        } else {
          // In main world, hide companions if they should be at camp (not
          // implemented yet, so show all)
        }

        // Check Fog of War
        if (entity != m_Player) {
          // Always show entities currently in combat turn order
          bool inCombat = IsInTurnOrder(entity);
          if (!inCombat &&
              !currentMap->IsVisible((int)transform->x, (int)transform->y)) {
            continue; // Skip rendering hidden entity
          }
        }

        int screenX, screenY;
        currentMap->GridToScreen(transform->x, transform->y, screenX, screenY);
        renderQueue.push_back({screenY, entity});
      }
    }

    std::sort(
        renderQueue.begin(), renderQueue.end(),
        [](const Renderable &a, const Renderable &b) { return a.y < b.y; });

    for (const auto &item : renderQueue) {
      auto *transform =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
              item.entity);
      auto *sprite = GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(
          item.entity);
      if (transform && sprite && sprite->texture) {
        int screenX, screenY;
        currentMap->GridToScreen(transform->x, transform->y, screenX, screenY);
        screenX -= (int)camera.x;
        screenY -= (int)camera.y;

        auto *entStats =
            GetRegistry().GetComponent<PixelsEngine::StatsComponent>(
                item.entity);
        if (entStats && entStats->isDead) {
          sprite->texture->SetColorMod(100, 100, 100); // Tint Grey
        }
        if (item.entity == m_Player && entStats && entStats->isStealthed) {
          // Apply alpha for stealth (Note: Texture class needs to handle alpha
          // mod or we can use SetColorMod)
          sprite->texture->SetColorMod(150, 150,
                                       255); // Blueish tint for stealth
        }

        sprite->texture->RenderRect(screenX + 16 - sprite->pivotX,
                                    screenY + 16 - sprite->pivotY,
                                    &sprite->srcRect, -1, -1, sprite->flip);

        if (entStats && (entStats->isDead || entStats->isStealthed)) {
          sprite->texture->SetColorMod(255, 255, 255); // Reset
        }

        // --- Render Health Bar Above Entity (in Combat or if damaged) ---
        if (entStats &&
            (m_State == GameState::Combat ||
             entStats->currentHealth < entStats->maxHealth) &&
            !entStats->isDead) {
          int barW = 32;
          int barH = 4;
          int barX = screenX;
          int barY = screenY - 8;

          SDL_Rect bg = {barX, barY, barW, barH};
          SDL_SetRenderDrawColor(GetRenderer(), 50, 50, 50, 255);
          SDL_RenderFillRect(GetRenderer(), &bg);

          float hpPercent =
              (float)entStats->currentHealth / (float)entStats->maxHealth;
          SDL_Rect fg = {barX, barY, (int)(barW * hpPercent), barH};
          SDL_SetRenderDrawColor(GetRenderer(), 255, 0, 0, 255);
          SDL_RenderFillRect(GetRenderer(), &fg);
        }
      }
    }
    // Render Floating Texts
    for (const auto &ft : m_FloatingText.m_Texts) {
      int screenX, screenY;
      currentMap->GridToScreen(ft.x, ft.y, screenX, screenY);
      screenX -= (int)camera.x;
      screenY -= (int)camera.y;
      m_TextRenderer->RenderTextCentered(ft.text, screenX + 16, screenY - 20,
                                         ft.color);
    }

    RenderHUD();
    if (m_State == GameState::Combat)
      RenderCombatUI();

    // --- Targeting Visuals ---
    if (m_State == GameState::Targeting ||
        m_State == GameState::TargetingJump ||
        m_State == GameState::TargetingShove ||
        PixelsEngine::Input::IsKeyDown(PixelsEngine::Config::GetKeybind(
            PixelsEngine::GameAction::AttackModifier))) {
      SDL_Renderer *renderer = GetRenderer();
      int winW, winH;
      SDL_GetWindowSize(m_Window, &winW, &winH);
      int mx, my;
      PixelsEngine::Input::GetMousePosition(mx, my);

      bool isAttack = (m_State != GameState::Targeting &&
                       m_State != GameState::TargetingJump &&
                       m_State != GameState::TargetingShove);

      if (m_State == GameState::Targeting) {
        m_TextRenderer->RenderTextCentered("TARGETING: " + m_PendingSpellName,
                                           winW / 2, 100, {200, 100, 255, 255});
        m_TextRenderer->RenderTextCentered("Click a target or ESC to cancel",
                                           winW / 2, 130, {150, 150, 150, 255});
      } else if (m_State == GameState::TargetingJump) {
        m_TextRenderer->RenderTextCentered("JUMPING", winW / 2, 100,
                                           {100, 255, 255, 255});
        m_TextRenderer->RenderTextCentered(
            "Click location to jump (Costs 3m + Bonus Action)", winW / 2, 130,
            {150, 150, 150, 255});
      } else if (m_State == GameState::TargetingShove) {
        m_TextRenderer->RenderTextCentered("SHOVING", winW / 2, 100,
                                           {255, 150, 50, 255});
        m_TextRenderer->RenderTextCentered(
            "Click an entity to shove (Costs Bonus Action)", winW / 2, 130,
            {150, 150, 150, 255});
      } else {
        m_TextRenderer->RenderTextCentered("ATTACK MODE", winW / 2, 100,
                                           {255, 100, 100, 255});
      }

      // Helper for world circles
      auto DrawWorldCircle = [&](float worldX, float worldY, float radiusTiles,
                                 SDL_Color color) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        int cx, cy;
        m_Level->GridToScreen(worldX, worldY, cx, cy);
        cx -= (int)camera.x;
        cy -= (int)camera.y;
        cx += 16;
        cy += 16; // Center of tile

        int r = (int)(radiusTiles * 32.0f);
        for (int i = 0; i < 360; i += 5) {
          float rad1 = i * (M_PI / 180.0f);
          float rad2 = (i + 5) * (M_PI / 180.0f);
          SDL_RenderDrawLine(renderer, cx + std::cos(rad1) * r,
                             cy + std::sin(rad1) * r, cx + std::cos(rad2) * r,
                             cy + std::sin(rad2) * r);
        }
      };

      // 2. Circle around caster (Player)
      auto *pTrans =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
              m_Player);
      if (pTrans) {
        float pulse = (std::sin(SDL_GetTicks() * 0.01f) + 1.0f) * 0.5f;
        SDL_Color auraCol = {255, 255, 255, (Uint8)(100 + pulse * 100)};
        if (m_State == GameState::Targeting)
          auraCol = {150, 50, 255, (Uint8)(100 + pulse * 100)};
        else if (m_State == GameState::TargetingJump)
          auraCol = {255, 255, 255,
                     (Uint8)(100 + pulse * 100)}; // White for Jump
        else if (m_State == GameState::TargetingShove ||
                 PixelsEngine::Input::IsKeyDown(
                     PixelsEngine::Config::GetKeybind(
                         PixelsEngine::GameAction::AttackModifier)))
          auraCol = {255, 50, 50, (Uint8)(100 + pulse * 100)};
        DrawWorldCircle(pTrans->x, pTrans->y, 1.5f, auraCol);
      }

      // 3. Jump Path Line
      if (m_State == GameState::TargetingJump && pTrans) {
        int px, py;
        m_Level->GridToScreen(pTrans->x, pTrans->y, px, py);
        px -= (int)camera.x;
        py -= (int)camera.y;
        px += 16;
        py += 16;

        // Cap the line at max jump distance
        float maxJumpDistPixels = 5.0f * 32.0f;
        float dx = (float)mx - px;
        float dy = (float)my - py;
        float currentDist = std::sqrt(dx * dx + dy * dy);

        int endX = mx;
        int endY = my;

        if (currentDist > maxJumpDistPixels) {
          float ratio = maxJumpDistPixels / currentDist;
          endX = px + (int)(dx * ratio);
          endY = py + (int)(dy * ratio);
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        SDL_RenderDrawLine(renderer, px, py, endX, endY);
      }

      // 4. Circle around hovered target
      auto &transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
      for (auto &[entity, transform] : transforms) {
        if (entity == m_Player)
          continue;
        auto *sprite =
            GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
        if (sprite) {
          int screenX, screenY;
          m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
          screenX -= (int)camera.x;
          screenY -= (int)camera.y;
          SDL_Rect drawRect = {screenX + 16 - sprite->pivotX,
                               screenY + 16 - sprite->pivotY, sprite->srcRect.w,
                               sprite->srcRect.h};
          if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w &&
              my >= drawRect.y && my <= drawRect.y + drawRect.h) {
            bool isTargetable =
                (m_State == GameState::Targeting ||
                 m_State == GameState::TargetingShove ||
                 PixelsEngine::Input::IsKeyDown(
                     PixelsEngine::Config::GetKeybind(
                         PixelsEngine::GameAction::AttackModifier)));
            if (isTargetable)
              DrawWorldCircle(transform.x, transform.y, 0.8f, {255, 0, 0, 255});
            break;
          }
        }
      }
    }

    // --- Movement Path Visual (Dash/Normal) ---
    if (m_State == GameState::Combat || m_ReturnState == GameState::Combat) {
      auto *pTrans =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
              m_Player);
      if (pTrans && !PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT)) {
        int mx, my;
        PixelsEngine::Input::GetMousePosition(mx, my);
        auto &camera = GetCamera();
        int worldX = mx + (int)camera.x, worldY = my + (int)camera.y, gridX,
            gridY;
        m_Level->ScreenToGrid(worldX, worldY, gridX, gridY);

        if (m_Level->IsWalkable(gridX, gridY)) {
          float dist = std::sqrt(std::pow(pTrans->x - gridX, 2) +
                                 std::pow(pTrans->y - gridY, 2));
          int px, py;
          m_Level->GridToScreen(pTrans->x, pTrans->y, px, py);
          px -= (int)camera.x;
          py -= (int)camera.y;
          px += 16;
          py += 16;

          SDL_Renderer *renderer = GetRenderer();
          if (dist <= m_Combat.m_MovementLeft) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255,
                                   150); // White: Reachable
            SDL_RenderDrawLine(renderer, px, py, mx, my);
          } else {
            // Draw two-part line
            float dashPotential = m_Combat.m_MovementLeft + 5.0f; // If they dashed
            float t = m_Combat.m_MovementLeft / dist;
            int midX = px + (int)((mx - px) * t);
            int midY = py + (int)((my - py) * t);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255,
                                   150); // Reachable now
            SDL_RenderDrawLine(renderer, px, py, midX, midY);

            SDL_SetRenderDrawColor(renderer, 255, 0, 0,
                                   150); // Need dash or too far
            SDL_RenderDrawLine(renderer, midX, midY, mx, my);
          }
        }
      }
    }

    RenderInventory();
    RenderContextMenu();
    RenderDiceRoll();
    RenderDayNightCycle(); // Render tint on top

    // Render Save Alert
    if (m_SaveMessageTimer > 0.0f) {
      m_TextRenderer->RenderText("Saving...", 20, 80,
                                 {255, 255, 0, 255}); // Below HP bar
    }

    // Render Fade Overlay
    if (m_FadeState != FadeState::None) {
      SDL_Renderer *renderer = GetRenderer();
      int w, h;
      SDL_GetWindowSize(m_Window, &w, &h);

      float alpha = 0.0f;
      if (m_FadeState == FadeState::FadingOut) {
        alpha = 1.0f - (m_FadeTimer / m_FadeDuration);
      } else {
        alpha = (m_FadeTimer / m_FadeDuration);
      }
      if (alpha < 0.0f)
        alpha = 0.0f;
      if (alpha > 1.0f)
        alpha = 1.0f;

      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)(alpha * 255));
      SDL_Rect screenRect = {0, 0, w, h};
      SDL_RenderFillRect(renderer, &screenRect);
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // If Paused, render Pause Menu on top
    if (m_State == GameState::Paused) {
      RenderPauseMenu();
    }
    if (m_State == GameState::RestMenu) {
      RenderRestMenu();
    }
    if (m_State == GameState::GameOver) {
      RenderGameOver();
    }
    if (m_State == GameState::Map) {
      RenderMapScreen();
    }
    if (m_State == GameState::CharacterMenu) {
      RenderCharacterMenu();
    }
    if (m_State == GameState::Trading) {

      RenderTradeScreen();
    }
    if (m_State == GameState::KeybindSettings) {
      RenderKeybindSettings();
    }
    if (m_State == GameState::Looting) {
      RenderLootScreen();
    }
    if (m_State == GameState::Dialogue) {
      RenderDialogueScreen();
    }
  } break;
  }
}

void PixelsGateGame::StartDiceRoll(int modifier, int dc,
                                   const std::string &skill,
                                   PixelsEngine::Entity target,
                                   PixelsEngine::ContextActionType type) {
  m_DiceRoll.active = true;
  m_DiceRoll.timer = 0.0f;
  m_DiceRoll.resultShown = false;
  m_DiceRoll.modifier = modifier;
  m_DiceRoll.dc = dc;
  m_DiceRoll.skillName = skill;
  m_DiceRoll.target = target;
  m_DiceRoll.actionType = type;
  int roll = PixelsEngine::Dice::Roll(20);
  m_DiceRoll.finalValue = roll;
  m_DiceRoll.success = (roll + modifier >= dc) || (roll == 20);
}

void PixelsGateGame::ResolveDiceRoll() {
  if (m_DiceRoll.success) {
    if (m_DiceRoll.actionType == PixelsEngine::ContextActionType::Pickpocket) {
      std::cout << "Pickpocket Success!" << std::endl;
      auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
          m_Player);
      if (inv)
        inv->AddItem("Coins", 10);
    } else if (m_DiceRoll.actionType ==
               PixelsEngine::ContextActionType::Lockpick) {
      auto *lock = GetRegistry().GetComponent<PixelsEngine::LockComponent>(
          m_DiceRoll.target);
      if (lock) {
        lock->isLocked = false;
        auto *tTrans =
            GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
                m_DiceRoll.target);
        SpawnFloatingText(tTrans->x, tTrans->y, "Unlocked!", {0, 255, 0, 255});
      }
    } else if (m_DiceRoll.actionType == PixelsEngine::ContextActionType::Talk) {
      auto *diag = GetRegistry().GetComponent<PixelsEngine::DialogueComponent>(
          m_DialogueWith);
      if (diag && diag->tree) {
        // Find the option that triggered this
        auto &node = diag->tree->nodes[diag->tree->currentNodeId];
        for (auto &opt : node.options) {
          if (opt.requiredStat != "None" &&
              (opt.requiredStat == m_DiceRoll.skillName ||
               m_DiceRoll.skillName.find(opt.requiredStat) !=
                   std::string::npos)) {
            diag->tree->currentNodeId = opt.onSuccessNodeId;
            break;
          }
        }
      }
    }
  } else {
    if (m_DiceRoll.actionType == PixelsEngine::ContextActionType::Pickpocket) {
      auto *interact =
          GetRegistry().GetComponent<PixelsEngine::InteractionComponent>(
              m_DiceRoll.target);
      if (interact) {
        interact->dialogueText = "Hey! Hands off!";
        interact->showDialogue = true;
        interact->dialogueTimer = 2.0f;
      }
    } else if (m_DiceRoll.actionType ==
               PixelsEngine::ContextActionType::Lockpick) {
      auto *inv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
          m_Player);
      if (inv) {
        for (auto it = inv->items.begin(); it != inv->items.end(); ++it) {
          if (it->name == "Thieves' Tools") {
            it->quantity--;
            if (it->quantity <= 0)
              inv->items.erase(it);
            break;
          }
        }
      }
      auto *tTrans =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
              m_DiceRoll.target);
      SpawnFloatingText(tTrans->x, tTrans->y, "Failed! Tool Broke.",
                        {255, 0, 0, 255});
    } else if (m_DiceRoll.actionType == PixelsEngine::ContextActionType::Talk) {
      auto *diag = GetRegistry().GetComponent<PixelsEngine::DialogueComponent>(
          m_DialogueWith);
      if (diag && diag->tree) {
        auto &node = diag->tree->nodes[diag->tree->currentNodeId];
        for (auto &opt : node.options) {
          if (opt.requiredStat != "None" &&
              (opt.requiredStat == m_DiceRoll.skillName ||
               m_DiceRoll.skillName.find(opt.requiredStat) !=
                   std::string::npos)) {
            diag->tree->currentNodeId = opt.onFailureNodeId;
            break;
          }
        }
      }
    }
  }
}

void PixelsGateGame::RenderDiceRoll() {
  if (!m_DiceRoll.active)
    return;
  SDL_Renderer *renderer = GetRenderer();
  int winW, winH;
  SDL_GetWindowSize(m_Window, &winW, &winH);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
  SDL_Rect overlay = {0, 0, winW, winH};
  SDL_RenderFillRect(renderer, &overlay);
  int boxW = 400, boxH = 300;
  SDL_Rect box = {(winW - boxW) / 2, (winH - boxH) / 2, boxW, boxH};
  SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
  SDL_RenderFillRect(renderer, &box);
  SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
  SDL_RenderDrawRect(renderer, &box);
  m_TextRenderer->RenderTextCentered("Skill Check: " + m_DiceRoll.skillName,
                                     box.x + boxW / 2, box.y + 30,
                                     {255, 255, 255, 255});
  m_TextRenderer->RenderTextCentered(
      "Difficulty Class: " + std::to_string(m_DiceRoll.dc), box.x + boxW / 2,
      box.y + 60, {200, 200, 200, 255});
  int displayVal = m_DiceRoll.resultShown ? m_DiceRoll.finalValue
                                          : PixelsEngine::Dice::Roll(20);
  m_TextRenderer->RenderTextCentered(std::to_string(displayVal),
                                     box.x + boxW / 2, box.y + 120,
                                     {255, 255, 0, 255});
  m_TextRenderer->RenderTextCentered(
      "Bonus: " + std::string((m_DiceRoll.modifier >= 0) ? "+" : "") +
          std::to_string(m_DiceRoll.modifier),
      box.x + boxW / 2, box.y + 170, {100, 255, 100, 255});
  if (m_DiceRoll.resultShown) {
    std::string resultText = m_DiceRoll.success ? "SUCCESS" : "FAILURE";
    SDL_Color resColor = m_DiceRoll.success ? SDL_Color{50, 255, 50, 255}
                                            : SDL_Color{255, 50, 50, 255};
    m_TextRenderer->RenderTextCentered(resultText, box.x + boxW / 2,
                                       box.y + 220, resColor);
    m_TextRenderer->RenderTextCentered("Click to Continue", box.x + boxW / 2,
                                       box.y + 260, {255, 255, 255, 255});
  } else {
    auto *stats =
        GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    if (stats && stats->inspiration > 0)
      m_TextRenderer->RenderTextCentered(
          "Press SPACE to use Inspiration (Reroll)", box.x + boxW / 2,
          box.y + 260, {100, 200, 255, 255});
  }
}

void PixelsGateGame::RenderContextMenu() {
  if (!m_ContextMenu.isOpen)
    return;
  SDL_Renderer *renderer = GetRenderer();
  int w = 150, h = m_ContextMenu.actions.size() * 30 + 10;
  SDL_Rect menuRect = {m_ContextMenu.x, m_ContextMenu.y, w, h};
  SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
  SDL_RenderFillRect(renderer, &menuRect);
  SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
  SDL_RenderDrawRect(renderer, &menuRect);
  int y = m_ContextMenu.y + 5;
  for (const auto &action : m_ContextMenu.actions) {
    m_TextRenderer->RenderText(action.label, m_ContextMenu.x + 10, y,
                               {255, 255, 255, 255});
    y += 30;
  }
}

void PixelsGateGame::RenderInventory() {
  auto *inv =
      GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
  if (!inv || !inv->isOpen)
    return;

  SDL_Renderer *renderer = GetRenderer();
  int winW, winH;
  SDL_GetWindowSize(m_Window, &winW, &winH);
  int w = 400, h = 500;
  SDL_Rect panel = {(winW - w) / 2, (winH - h) / 2, w, h};

  SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
  SDL_RenderFillRect(renderer, &panel);
  SDL_SetRenderDrawColor(renderer, 218, 165, 32, 255);
  SDL_RenderDrawRect(renderer, &panel);

  m_TextRenderer->RenderTextCentered("INVENTORY", panel.x + w / 2, panel.y + 20,
                                     {255, 255, 255, 255});

  // Close Button (X in top right)
  SDL_Rect closeBtn = {panel.x + w - 30, panel.y + 10, 20, 20};
  SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
  SDL_RenderFillRect(renderer, &closeBtn);
  m_TextRenderer->RenderTextCentered("X", closeBtn.x + 10, closeBtn.y + 10,
                                     {255, 255, 255, 255});

  // --- Equipment Slots (Top) ---
  int slotSize = 40;
  int startX = panel.x + 50;
  int startY = panel.y + 60;

  auto DrawSlot = [&](const std::string &label, PixelsEngine::Item &item, int x,
                      int y) {
    SDL_Rect slotRect = {x, y, slotSize, slotSize};
    SDL_SetRenderDrawColor(renderer, 50, 30, 10, 255);
    SDL_RenderFillRect(renderer, &slotRect);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &slotRect);

    m_TextRenderer->RenderTextCentered(label, x + slotSize / 2, y - 15,
                                       {200, 200, 200, 255});
    if (!item.IsEmpty()) {
      std::string iconPath = item.iconPath;
      if (iconPath.empty()) {
        if (item.name == "Potion")
          iconPath = "assets/ui/item_potion.png";
        else if (item.name == "Bread")
          iconPath = "assets/ui/item_bread.png";
        else if (item.name == "Coins")
          iconPath = "assets/ui/item_coins.png";
        else if (item.name == "Rare Gem")
          iconPath = "assets/ui/item_raregem.png";
        else if (item.name == "Boar Meat")
          iconPath = "assets/ui/item_boarmeat.png";
      }

      if (!iconPath.empty()) {
        auto tex =
            PixelsEngine::TextureManager::LoadTexture(GetRenderer(), iconPath);
        if (tex)
          tex->Render(x + 4, y + 4, 32, 32);
      } else {
        m_TextRenderer->RenderTextCentered(item.name.substr(0, 3),
                                           x + slotSize / 2, y + slotSize / 2,
                                           {255, 255, 0, 255});
      }
    }
  };

  DrawSlot("Melee", inv->equippedMelee, startX, startY);
  DrawSlot("Range", inv->equippedRanged, startX + 100, startY);
  DrawSlot("Armor", inv->equippedArmor, startX + 200, startY);

  // --- Stats Summary ---
  auto *stats =
      GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
  if (stats) {
    int armorBonus = inv->equippedArmor.statBonus;
    int damageBonus =
        inv->equippedMelee.statBonus + inv->equippedRanged.statBonus;
    std::string statStr =
        "AC: " +
        std::to_string(10 + stats->GetModifier(stats->dexterity) + armorBonus) +
        "  Dmg Bonus: " + std::to_string(damageBonus);
    m_TextRenderer->RenderTextCentered(statStr, panel.x + w / 2, startY + 60,
                                       {200, 255, 200, 255});
  }

  // --- Item List ---
  int listY = startY + 90;
  SDL_Rect listArea = {panel.x + 20, listY, w - 40, h - (listY - panel.y) - 20};
  SDL_SetRenderDrawColor(renderer, 80, 40, 10, 255);
  SDL_RenderFillRect(renderer, &listArea);

  int itemY = listArea.y + 10;
  for (int i = 0; i < inv->items.size(); ++i) {
    auto &item = inv->items[i];

    // Simple Hover logic
    int mx, my;
    PixelsEngine::Input::GetMousePosition(mx, my);
    SDL_Rect rowRect = {listArea.x, itemY, listArea.w,
                        40}; // Increased height for icons
    bool hover = (mx >= rowRect.x && mx <= rowRect.x + rowRect.w &&
                  my >= rowRect.y && my <= rowRect.y + rowRect.h);

    if (hover)
      SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
    else
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    if (hover)
      SDL_RenderFillRect(renderer, &rowRect);

    RenderInventoryItem(item, listArea.x + 10, itemY + 4);
    itemY += 45;
  }

  // --- Drag and Drop Visual ---
  if (m_DraggingItemIndex != -1 && m_DraggingItemIndex < inv->items.size()) {
    int mx, my;
    PixelsEngine::Input::GetMousePosition(mx, my);
    RenderInventoryItem(inv->items[m_DraggingItemIndex], mx - 16, my - 16);
  }
}

void PixelsGateGame::RenderInventoryItem(const PixelsEngine::Item &item, int x,
                                         int y) {
  std::string iconPath = item.iconPath;
  if (iconPath.empty()) {
    if (item.name == "Potion")
      iconPath = "assets/ui/item_potion.png";
    else if (item.name == "Bread")
      iconPath = "assets/ui/item_bread.png";
    else if (item.name == "Coins")
      iconPath = "assets/ui/item_coins.png";
    else if (item.name == "Rare Gem")
      iconPath = "assets/ui/item_raregem.png";
    else if (item.name == "Boar Meat")
      iconPath = "assets/ui/item_boarmeat.png";
  }

  if (!iconPath.empty()) {
    auto tex =
        PixelsEngine::TextureManager::LoadTexture(GetRenderer(), iconPath);
    if (tex)
      tex->Render(x, y, 32, 32);
  }

  std::string typeStr = "";
  if (item.type == PixelsEngine::ItemType::WeaponMelee)
    typeStr = "[M]";
  else if (item.type == PixelsEngine::ItemType::WeaponRanged)
    typeStr = "[R]";
  else if (item.type == PixelsEngine::ItemType::Armor)
    typeStr = "[A]";

  std::string display =
      item.name + " x" + std::to_string(item.quantity) + " " + typeStr;
  m_TextRenderer->RenderText(display, x + 40, y + 8, {255, 255, 255, 255});
}

void PixelsGateGame::HandleInput() {
  bool isAttackModifierDown =
      PixelsEngine::Input::IsKeyDown(PixelsEngine::Config::GetKeybind(
          PixelsEngine::GameAction::AttackModifier));
  bool isCtrlDown = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_LCTRL) ||
                    PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RCTRL);

  bool isDownLeftRaw = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT);
  bool isPressedLeftRaw =
      PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT);
  bool isPressedRightRaw =
      PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_RIGHT);

  // CTRL + CLICK = Right Click (equivalent to isPressedRight)
  bool isPressedLeft = isPressedLeftRaw && !isCtrlDown;
  bool isPressedRight = isPressedRightRaw || (isPressedLeftRaw && isCtrlDown);

  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  if (m_DiceRoll.active) {
    if (!m_DiceRoll.resultShown) {
      m_DiceRoll.timer += 0.016f; // approx delta
      if (m_DiceRoll.timer > m_DiceRoll.duration) {
        m_DiceRoll.resultShown = true;
        ResolveDiceRoll();
      }

      // Handle Inspiration
      if (PixelsEngine::Input::IsKeyDown(PixelsEngine::Config::GetKeybind(
              PixelsEngine::GameAction::EndTurn))) { // Default Space
        auto *stats =
            GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
        if (stats && stats->inspiration > 0) {
          stats->inspiration--;
          // Reroll
          m_DiceRoll.finalValue = PixelsEngine::Dice::Roll(20);
          m_DiceRoll.success =
              (m_DiceRoll.finalValue + m_DiceRoll.modifier >= m_DiceRoll.dc) ||
              (m_DiceRoll.finalValue == 20);
          m_DiceRoll.timer = 0.0f; // Restart anim
          std::cout << "Used Inspiration! Rerolling..." << std::endl;
        }
      }
    } else {
      // Click to close
      if (isPressedLeft) {
        m_DiceRoll.active = false;
        // If it was a dialogue roll, return to dialogue to show consequence
        if (m_DiceRoll.actionType == PixelsEngine::ContextActionType::Talk) {
          m_State = GameState::Dialogue;
        }
      }
    }
    return; // Block other input
  }

  // Quick Save/Load
  if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_F5)) {
    PixelsEngine::SaveSystem::SaveGame(
        "quicksave.dat", GetRegistry(), m_Player, *m_Level,
        m_State == GameState::Camp, m_LastWorldPos.x, m_LastWorldPos.y);
    PixelsEngine::SaveSystem::SaveWorldFlags("quicksave.dat", m_WorldFlags);
    ShowSaveMessage();
  }
  if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_F8)) {
    PixelsEngine::SaveSystem::LoadWorldFlags("quicksave.dat", m_WorldFlags);
    TriggerLoadTransition("quicksave.dat");
  }

  if (m_ContextMenu.isOpen) {
    if (isPressedLeft) {
      int w = 150, h = m_ContextMenu.actions.size() * 30 + 10;
      SDL_Rect menuRect = {m_ContextMenu.x, m_ContextMenu.y, w, h};
      if (mx >= menuRect.x && mx <= menuRect.x + w && my >= menuRect.y &&
          my <= menuRect.y + h) {
        int index = (my - m_ContextMenu.y - 5) / 30;
        if (index >= 0 && index < m_ContextMenu.actions.size()) {
          auto &action = m_ContextMenu.actions[index];
          if (action.type == PixelsEngine::ContextActionType::Talk) {
            auto *diag =
                GetRegistry().GetComponent<PixelsEngine::DialogueComponent>(
                    m_ContextMenu.targetEntity);
            if (diag && diag->tree) {
              m_DialogueWith = m_ContextMenu.targetEntity;
              m_ReturnState = m_State;
              m_State = GameState::Dialogue;
              m_DialogueSelection = 0;
              diag->tree->currentNodeId = "start"; // Reset tree
            } else {
              m_SelectedNPC =
                  m_ContextMenu.targetEntity; // Fallback to floating bubble
            }
          } else if (action.type == PixelsEngine::ContextActionType::Attack)
            PerformAttack(m_ContextMenu.targetEntity);
          else if (action.type == PixelsEngine::ContextActionType::Trade) {
            m_TradingWith = m_ContextMenu.targetEntity;
            m_ReturnState = m_State;
            m_State = GameState::Trading;
          } else if (action.type ==
                     PixelsEngine::ContextActionType::Pickpocket) {
            auto *stats =
                GetRegistry().GetComponent<PixelsEngine::StatsComponent>(
                    m_Player);
            StartDiceRoll(stats->GetModifier(stats->dexterity), 15,
                          "Dexterity (Sleight of Hand)",
                          m_ContextMenu.targetEntity,
                          PixelsEngine::ContextActionType::Pickpocket);
          } else if (action.type == PixelsEngine::ContextActionType::Lockpick) {
            auto *inv =
                GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
                    m_Player);
            bool hasTools = false;

            if (inv) {
              std::cout << "Checking Inventory for Tools:" << std::endl;
              for (auto &item : inv->items) {
                std::cout << " - Item: '" << item.name << "'" << std::endl;
                if (item.name == "Thieves' Tools")
                  hasTools = true;
              }
            }

            if (hasTools) {
              auto *stats =
                  GetRegistry().GetComponent<PixelsEngine::StatsComponent>(
                      m_Player);
              auto *lock =
                  GetRegistry().GetComponent<PixelsEngine::LockComponent>(
                      m_ContextMenu.targetEntity);

              int dc = lock ? lock->dc : 15;
              int bonus =
                  stats->GetModifier(stats->dexterity) + stats->sleightOfHand;
              StartDiceRoll(bonus, dc, "Dexterity (Sleight of Hand)",
                            m_ContextMenu.targetEntity,
                            PixelsEngine::ContextActionType::Lockpick);
            } else {
              auto *pTrans =
                  GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
                      m_Player);
              SpawnFloatingText(pTrans->x, pTrans->y, "Missing: Thieves' Tools",
                                {255, 0, 0, 255});
            }
          }
        }
      }
      m_ContextMenu.isOpen = false;
    } else if (isPressedRight)
      m_ContextMenu.isOpen = false;
    return;
  }

  if (isPressedLeft) {
    int winW, winH;
    SDL_GetWindowSize(m_Window, &winW, &winH);

    auto *inv =
        GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if (inv && inv->isOpen) {
      int iw = 400, ih = 500;
      SDL_Rect panel = {(winW - iw) / 2, (winH - ih) / 2, iw, ih};
      if (mx >= panel.x && mx <= panel.x + iw && my >= panel.y &&
          my <= panel.y + ih)
        return;
    }

    if (my > winH - 90)
      CheckUIInteraction(mx, my);
    else {
      // Check for Shift/AttackModifier + Click Attack
      if (isAttackModifierDown) {
        auto &camera = GetCamera();
        auto &transforms =
            GetRegistry().View<PixelsEngine::TransformComponent>();
        bool found = false;
        for (auto &[entity, transform] : transforms) {
          if (entity == m_Player)
            continue;
          auto *sprite =
              GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
          if (sprite) {
            int screenX, screenY;
            m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
            screenX -= (int)camera.x;
            screenY -= (int)camera.y;
            SDL_Rect drawRect = {screenX + 16 - sprite->pivotX,
                                 screenY + 16 - sprite->pivotY,
                                 sprite->srcRect.w, sprite->srcRect.h};
            if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w &&
                my >= drawRect.y && my <= drawRect.y + drawRect.h) {
              PerformAttack(entity);
              found = true;
              break;
            }
          }
        }
        if (!found)
          CheckWorldInteraction(mx, my); // Fallback to move
      } else if (isCtrlDown) {
        // Ctrl + Click handles context menu logic elsewhere or fallback move
        // here
        CheckWorldInteraction(mx, my);
      } else {
        CheckWorldInteraction(mx, my);
      }
    }
  }

  if (isPressedRight) {
    auto &camera = GetCamera();
    auto &transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
    bool found = false;
    for (auto &[entity, transform] : transforms) {
      if (entity == m_Player)
        continue;
      auto *sprite =
          GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
      if (sprite) {
        int screenX, screenY;
        m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
        screenX -= (int)camera.x;
        screenY -= (int)camera.y;
        SDL_Rect drawRect = {screenX + 16 - sprite->pivotX,
                             screenY + 16 - sprite->pivotY, sprite->srcRect.w,
                             sprite->srcRect.h};
        if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w &&
            my >= drawRect.y && my <= drawRect.y + drawRect.h) {
          m_ContextMenu.isOpen = true;
          m_ContextMenu.x = mx;
          m_ContextMenu.y = my;
          m_ContextMenu.targetEntity = entity;
          m_ContextMenu.actions.clear();
          m_ContextMenu.actions.push_back(
              {"Attack", PixelsEngine::ContextActionType::Attack});

          auto *tag =
              GetRegistry().GetComponent<PixelsEngine::TagComponent>(entity);
          bool isLiving =
              (tag && (tag->tag == PixelsEngine::EntityTag::NPC ||
                       tag->tag == PixelsEngine::EntityTag::Trader ||
                       tag->tag == PixelsEngine::EntityTag::Companion ||
                       tag->tag == PixelsEngine::EntityTag::Hostile));

          if (isLiving &&
              GetRegistry().HasComponent<PixelsEngine::InteractionComponent>(
                  entity)) {
            m_ContextMenu.actions.push_back(
                {"Talk", PixelsEngine::ContextActionType::Talk});
            m_ContextMenu.actions.push_back(
                {"Pickpocket", PixelsEngine::ContextActionType::Pickpocket});
            m_ContextMenu.actions.push_back(
                {"Trade", PixelsEngine::ContextActionType::Trade});
          }

          auto *lock =
              GetRegistry().GetComponent<PixelsEngine::LockComponent>(entity);
          if (lock && lock->isLocked) {
            m_ContextMenu.actions.push_back(
                {"Lockpick", PixelsEngine::ContextActionType::Lockpick});
          }
          found = true;
          break;
        }
      }
    }
    if (!found)
      m_ContextMenu.isOpen = false;
  }
}

void PixelsGateGame::CheckUIInteraction(int mx, int my) {
  int winW, winH;
  SDL_GetWindowSize(m_Window, &winW, &winH);
  int barH = 100;

  auto CheckGridClick = [&](int startX, int count,
                            std::function<void(int)> onClick) {
    for (int i = 0; i < count; ++i) {
      int row = i / 3;
      int col = i % 3;
      SDL_Rect btn = {startX + col * 45, winH - barH + 25 + row * 35, 40, 30};
      if (mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y &&
          my <= btn.y + btn.h) {
        onClick(i);
        return true;
      }
    }
    return false;
  };

  // Actions Grid (20)
  if (CheckGridClick(20, 6, [&](int i) {
        if (i == 0)
          PerformAttack();
        else if (i == 1) {
          m_ReturnState = m_State;
          m_State = GameState::TargetingJump;
        } else if (i == 2) {
          auto *pStats =
              GetRegistry().GetComponent<PixelsEngine::StatsComponent>(
                  m_Player);
          if (pStats) {
            if (m_State == GameState::Combat) {
              if (m_Combat.m_ActionsLeft > 0) {
                pStats->isStealthed = true;
                pStats->hasAdvantage = true;
                m_Combat.m_ActionsLeft--;
              }
            } else {
              pStats->isStealthed = !pStats->isStealthed;
            }
          }
        } else if (i == 3) {
          m_ReturnState = m_State;
          m_State = GameState::TargetingShove;
        } else if (i == 4) {
          if (m_State == GameState::Combat && m_Combat.m_ActionsLeft > 0) {
            m_Combat.m_MovementLeft += 5.0f;
            m_Combat.m_ActionsLeft--;
            SpawnFloatingText(0, 0, "Dashed!", {255, 255, 0, 255});
          }
        } else if (i == 5) {
          if (m_State == GameState::Combat)
            NextTurn();
        }
      }))
    return;

  // Spells Grid (170)
  if (CheckGridClick(170, 6, [&](int i) {
        std::string spellNames[] = {"Fireball", "Heal", "Magic Missile",
                                    "Shield",   "",     ""};
        if (!spellNames[i].empty()) {
          m_PendingSpellName = spellNames[i];
          m_ReturnState = m_State;
          m_State = GameState::Targeting;
        }
      }))
    return;

  // Items Grid (320)
  if (CheckGridClick(320, 6, [&](int i) {
        auto items = GetHotbarItems();
        if (!items[i].empty()) {
          UseItem(items[i]);
        }
      }))
    return;

  // System Buttons
  for (int i = 0; i < 4; ++i) {
    SDL_Rect btn = {winW - 180 + (i % 2) * 85, winH - barH + 15 + (i / 2) * 40,
                    75, 35};
    if (mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y &&
        my <= btn.y + btn.h) {
      if (i == 0) {
        if (m_State != GameState::Map) {
          m_ReturnState = m_State;
          m_State = GameState::Map;
        } else
          m_State = m_ReturnState;
      } else if (i == 1) {
        if (m_State == GameState::CharacterMenu && m_CharacterTab == 1) {
          m_State = m_ReturnState;
        } else {
          m_ReturnState =
              (m_State == GameState::CharacterMenu) ? m_ReturnState : m_State;
          m_State = GameState::CharacterMenu;
          m_CharacterTab = 1;
        }
      } else if (i == 2) {
        if (m_State != GameState::RestMenu) {
          m_ReturnState = m_State;
          m_State = GameState::RestMenu;
          m_MenuSelection = 0;
        } else
          m_State = m_ReturnState;
      } else if (i == 3) {
        if (m_State == GameState::CharacterMenu && m_CharacterTab == 0) {
          m_State = m_ReturnState;
        } else {
          m_ReturnState =
              (m_State == GameState::CharacterMenu) ? m_ReturnState : m_State;
          m_State = GameState::CharacterMenu;
          m_CharacterTab = 0;
        }
      }
      return;
    }
  }

  // Weapon Selection (next to Atk)
  SDL_Rect atkBtn = {20, winH - barH + 25, 40, 30};
  SDL_Rect mRect = {atkBtn.x - 15, atkBtn.y, 12, 14};
  SDL_Rect rRect = {atkBtn.x - 15, atkBtn.y + 16, 12, 14};
  if (mx >= mRect.x && mx <= mRect.x + mRect.w && my >= mRect.y &&
      my <= mRect.y + mRect.h)
    m_SelectedWeaponSlot = 0;
  if (mx >= rRect.x && mx <= rRect.x + rRect.w && my >= rRect.y &&
      my <= rRect.y + rRect.h)
    m_SelectedWeaponSlot = 1;
}

void PixelsGateGame::CheckWorldInteraction(int mx, int my) {
  auto &camera = GetCamera();
  auto &transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
  bool clickedEntity = false;
  for (auto &[entity, transform] : transforms) {
    if (entity == m_Player)
      continue;

    // Filter entities by map state
    if (m_State == GameState::Camp) {
      auto *tag =
          GetRegistry().GetComponent<PixelsEngine::TagComponent>(entity);
      if (!tag || tag->tag != PixelsEngine::EntityTag::Companion)
        continue;
    }

    auto *sprite =
        GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);

    if (sprite) {
      int screenX, screenY;
      m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
      screenX -= (int)camera.x;
      screenY -= (int)camera.y;
      SDL_Rect drawRect = {screenX + 16 - sprite->pivotX,
                           screenY + 16 - sprite->pivotY, sprite->srcRect.w,
                           sprite->srcRect.h};
      if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w &&
          my >= drawRect.y && my <= drawRect.y + drawRect.h) {
        // If dead, open loot menu
        auto *stats =
            GetRegistry().GetComponent<PixelsEngine::StatsComponent>(entity);
        if (stats && stats->isDead) {
          auto *loot =
              GetRegistry().GetComponent<PixelsEngine::LootComponent>(entity);
          if (loot) {
            m_LootingEntity = entity;
            m_ReturnState = m_State;
            m_State = GameState::Looting;
            return;
          }
        }

        auto *lock =
            GetRegistry().GetComponent<PixelsEngine::LockComponent>(entity);
        if (lock) {
          if (lock->isLocked) {
            auto *inv =
                GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
                    m_Player);
            bool hasKey = false;
            if (inv) {
              for (auto &item : inv->items) {
                if (item.name == lock->keyName)
                  hasKey = true;
              }
            }
            if (hasKey) {
              lock->isLocked = false;
              SpawnFloatingText(transform.x, transform.y, "Unlocked with Key",
                                {0, 255, 0, 255});
              m_LootingEntity = entity;
              m_ReturnState = m_State;
              m_State = GameState::Looting;
              return;
            } else {
              SpawnFloatingText(transform.x, transform.y, "Locked",
                                {255, 0, 0, 255});
              return;
            }
          } else {
            m_LootingEntity = entity;
            m_ReturnState = m_State;
            m_State = GameState::Looting;
            return;
          }
        }

        // Default: Try to open Dialogue
        auto *diag =
            GetRegistry().GetComponent<PixelsEngine::DialogueComponent>(entity);
        if (diag && diag->tree) {
          m_DialogueWith = entity;
          m_ReturnState = m_State;
          m_State = GameState::Dialogue;
          m_DialogueSelection = 0;
          diag->tree->currentNodeId = "start";
          return;
        }

        m_SelectedNPC = entity;
        clickedEntity = true;
        auto *pathComp =
            GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(
                m_Player);
        auto *pTrans =
            GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
                m_Player);
        auto path = PixelsEngine::Pathfinding::FindPath(
            *m_Level, (int)pTrans->x, (int)pTrans->y, (int)transform.x,
            (int)transform.y);
        if (!path.empty()) {
          pathComp->path = path;
          pathComp->currentPathIndex = 0;
          pathComp->isMoving = true;
          pathComp->targetX = (float)path[0].first;
          pathComp->targetY = (float)path[0].second;
        }
        break;
      }
    }
  }
  if (!clickedEntity) {
    int worldX = mx + camera.x, worldY = my + camera.y, gridX, gridY;
    m_Level->ScreenToGrid(worldX, worldY, gridX, gridY);
    m_SelectedNPC = PixelsEngine::INVALID_ENTITY;
    auto *pathComp =
        GetRegistry().GetComponent<PixelsEngine::PathMovementComponent>(
            m_Player);
    auto *pTrans =
        GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    auto path = PixelsEngine::Pathfinding::FindPath(
        *m_Level, (int)pTrans->x, (int)pTrans->y, gridX, gridY);
    if (!path.empty()) {
      pathComp->path = path;
      pathComp->currentPathIndex = 0;
      pathComp->isMoving = true;
      pathComp->targetX = (float)path[0].first;
      pathComp->targetY = (float)path[0].second;
    }
  }
}

void PixelsGateGame::RenderHUD() {
  SDL_Renderer *renderer = GetRenderer();
  int winW, winH;
  SDL_GetWindowSize(m_Window, &winW, &winH);

  if (!m_TooltipPinned)
    m_HoveredItemName = "";

  // --- 1. Health Bar (Top Left) ---
  auto *stats =
      GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
  if (stats) {
    int barW = 200;
    int barH = 20;
    int x = 20;
    int y = 20;

    // Background (Grey)
    SDL_Rect bgRect = {x, y, barW, barH};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &bgRect);

    // Foreground (Red)
    float hpPercent = (float)stats->currentHealth / (float)stats->maxHealth;
    if (hpPercent < 0)
      hpPercent = 0;
    SDL_Rect fgRect = {x, y, (int)(barW * hpPercent), barH};
    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    SDL_RenderFillRect(renderer, &fgRect);

    // Border (White)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &bgRect);

    // Text
    std::string hpText = "HP: " + std::to_string(stats->currentHealth) + " / " +
                         std::to_string(stats->maxHealth);
    m_TextRenderer->RenderText(hpText, x + 10, y + 25, {255, 255, 255, 255});
  }

  // --- 2. Minimap (Top Right) ---
  auto *currentMap =
      (m_State == GameState::Camp) ? m_CampLevel.get() : m_Level.get();
  if (currentMap) {
    int mapW = currentMap->GetWidth();
    int mapH = currentMap->GetHeight();
    int tileSize =
        (m_State == GameState::Camp) ? 8 : 4; // Larger tiles for small camp map
    int miniW = mapW * tileSize;
    int miniH = mapH * tileSize;
    int mx = winW - miniW - 20;
    int my = 20;

    // Background / Border
    SDL_Rect miniRect = {mx - 2, my - 2, miniW + 4, miniH + 4};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &miniRect);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &miniRect);

    // Draw Tiles
    for (int ty = 0; ty < mapH; ++ty) {
      for (int tx = 0; tx < mapW; ++tx) {
        // Check Exploration
        if (!currentMap->IsExplored(tx, ty))
          continue;

        using namespace PixelsEngine::Tiles;
        int tileIdx = currentMap->GetTile(tx, ty);
        SDL_Color c = {0, 0, 0, 255};

        // Detailed Minimap Colors
        if (tileIdx >= DIRT && tileIdx <= DIRT_VARIANT_18)
          c = {101, 67, 33, 255}; // Brown
        else if (tileIdx >= DIRT_WITH_LEAVES_01 &&
                 tileIdx <= DIRT_WITH_LEAVES_02)
          c = {85, 107, 47, 255}; // Olive
        else if (tileIdx >= GRASS && tileIdx <= GRASS_VARIANT_02)
          c = {34, 139, 34, 255}; // Forest Green
        else if (tileIdx == DIRT_VARIANT_19 ||
                 tileIdx == DIRT_WITH_PARTIAL_GRASS)
          c = {139, 69, 19, 255}; // Saddle Brown
        else if (tileIdx >= GRASS_BLOCK_FULL &&
                 tileIdx <= GRASS_BLOCK_FULL_VARIANT_01)
          c = {0, 128, 0, 255}; // Green
        else if (tileIdx >= GRASS_WITH_BUSH &&
                 tileIdx <= GRASS_WITH_BUSH_VARIANT_07)
          c = {0, 100, 0, 255}; // Dark Green
        else if (tileIdx >= GRASS_VARIANT_03 && tileIdx <= GRASS_VARIANT_06)
          c = {50, 205, 50, 255}; // Lime Green
        else if (tileIdx >= FLOWER && tileIdx <= FLOWERS_WITHOUT_LEAVES)
          c = {255, 105, 180, 255}; // Pink/Flower
        else if (tileIdx >= LOG && tileIdx <= LOG_WITH_LEAVES_VARIANT_02)
          c = {139, 69, 19, 255}; // Log Brown
        else if (tileIdx >= DIRT_PILE && tileIdx <= DIRT_PILE_VARIANT_07)
          c = {160, 82, 45, 255}; // Sienna
        else if (tileIdx >= COBBLESTONE && tileIdx <= SMOOTH_STONE)
          c = {128, 128, 128, 255}; // Grey
        else if (tileIdx >= ROCK && tileIdx <= ROCK_VARIANT_03)
          c = {105, 105, 105, 255}; // Dim Grey
        else if (tileIdx >= ROCK_ON_WATER &&
                 tileIdx <= STONES_ON_WATER_VARIANT_11)
          c = {70, 130, 180, 255}; // Steel Blue
        else if (tileIdx >= WATER_DROPLETS && tileIdx <= OCEAN_ROUGH)
          c = {0, 0, 255, 255}; // Blue
        else
          c = {100, 100, 100, 255}; // Unknown: Dark Grey

        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect tileRect = {mx + tx * tileSize, my + ty * tileSize, tileSize,
                             tileSize};
        SDL_RenderFillRect(renderer, &tileRect);
      }
    }

    // Helper for circles on minimap
    auto FillCircle = [&](int cx, int cy, int r, SDL_Color color) {
      SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
      for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
          if (dx * dx + dy * dy <= r * r)
            SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
        }
      }
    };

    // Render all entities on Minimap
    auto &transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
    for (auto &[entity, trans] : transforms) {
      // Filter entities by map
      if (m_State == GameState::Camp) {
        if (entity != m_Player) {
          auto *tag =
              GetRegistry().GetComponent<PixelsEngine::TagComponent>(entity);
          if (!tag || tag->tag != PixelsEngine::EntityTag::Companion)
            continue;
        }
      } else {
        // Don't show companions if they were left at camp? (simplified: show
        // all)
      }

      if (!currentMap->IsVisible((int)trans.x, (int)trans.y))
        continue;

      int px = mx + (int)trans.x * tileSize + tileSize / 2;
      int py = my + (int)trans.y * tileSize + tileSize / 2;

      if (entity == m_Player) {
        FillCircle(px, py, 3, {255, 255, 255, 255}); // Player: White Circle
        continue;
      }

      auto *tagComp =
          GetRegistry().GetComponent<PixelsEngine::TagComponent>(entity);
      if (tagComp) {
        switch (tagComp->tag) {
        case PixelsEngine::EntityTag::Hostile:
          FillCircle(px, py, 2, {255, 0, 0, 255}); // Hostile: Red Circle
          break;
        case PixelsEngine::EntityTag::NPC:
          FillCircle(px, py, 2, {255, 215, 0, 255}); // NPC: Gold Circle
          break;
        case PixelsEngine::EntityTag::Companion:
          FillCircle(px, py, 2, {0, 191, 255, 255}); // Companion: Blue Circle
          break;
        case PixelsEngine::EntityTag::Trader: {
          // Trader: Loot Bag Icon (Small box)
          SDL_Rect box = {px - 2, py - 2, 4, 4};
          SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
          SDL_RenderFillRect(renderer, &box);
          break;
        }
        case PixelsEngine::EntityTag::Quest: {
          // Quest: Circle with Golden Square
          FillCircle(px, py, 3, {255, 255, 255, 255});
          SDL_Rect sq = {px - 1, py - 1, 2, 2};
          SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
          SDL_RenderFillRect(renderer, &sq);
          break;
        }
        default:
          break;
        }
      }
    }
  }

  // --- 3. Bottom Action Bar (Redesigned) ---
  int barH = 100;
  SDL_Rect hudRect = {0, winH - barH, winW, barH};
  SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
  SDL_RenderFillRect(renderer, &hudRect);
  SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
  SDL_RenderDrawRect(renderer, &hudRect);

  auto DrawGrid = [&](const std::string &title, int startX,
                      const std::vector<std::string> &labels,
                      const std::vector<std::string> &keys,
                      const std::vector<std::string> &icons,
                      const std::vector<std::string> &counts, int count) {
    m_TextRenderer->RenderText(title, startX, winH - barH + 5,
                               {200, 200, 200, 255});
    int mx, my;
    PixelsEngine::Input::GetMousePosition(mx, my);

    for (int i = 0; i < count; ++i) {
      int row = i / 3;
      int col = i % 3;
      SDL_Rect btn = {startX + col * 45, winH - barH + 25 + row * 35, 40, 30};
      SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
      SDL_RenderFillRect(renderer, &btn);
      SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
      SDL_RenderDrawRect(renderer, &btn);

      // Hover Detection for Tooltips
      if (mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y &&
          my <= btn.y + btn.h) {
        if (title == "ACTIONS" || title == "SPELLS") {
          if (i < (int)labels.size())
            m_HoveredItemName = labels[i];
        } else if (title == "ITEMS") {
          auto hotbarItems = GetHotbarItems();
          if (i < (int)hotbarItems.size())
            m_HoveredItemName = hotbarItems[i];
        }
      }

      bool iconDrawn = false;
      if (i < (int)icons.size() && !icons[i].empty()) {
        auto tex =
            PixelsEngine::TextureManager::LoadTexture(renderer, icons[i]);
        if (tex) {
          tex->Render(btn.x + 8, btn.y + 3, 24, 24);
          iconDrawn = true;
        }
      }

      // Fallback Label (only if icon missing)
      if (!iconDrawn && i < (int)labels.size() && !labels[i].empty()) {
        m_TextRenderer->RenderTextCentered(labels[i], btn.x + 20, btn.y + 15,
                                           {255, 255, 255, 255});
      }

      // Item Count (Bottom Right, White, only if > 1)
      if (iconDrawn && i < (int)counts.size() && !counts[i].empty() &&
          counts[i] != "1" && counts[i] != "0") {
        m_TextRenderer->RenderTextRightAlignedSmall(
            counts[i], btn.x + btn.w - 2, btn.y + 18, {255, 255, 255, 255});
      }

      // Render Keybind hint in bottom left
      if (i < (int)keys.size() && !keys[i].empty()) {
        m_TextRenderer->RenderTextSmall(
            keys[i], btn.x + 2, btn.y + 18,
            {255, 255, 0, 200}); // Small yellow hint
      }

      // Restore weapon toggles next to Atk (i=0)
      if (title == "ACTIONS" && i == 0) {
        SDL_Rect mRect = {btn.x - 15, btn.y, 12, 14};
        SDL_SetRenderDrawColor(renderer, (m_SelectedWeaponSlot == 0) ? 200 : 50,
                               50, 50, 255);
        SDL_RenderFillRect(renderer, &mRect);
        auto sTex = PixelsEngine::TextureManager::LoadTexture(
            renderer, "assets/sword.png");
        if (sTex)
          sTex->Render(mRect.x + 1, mRect.y + 1, 10, 12);

        SDL_Rect rRect = {btn.x - 15, btn.y + 16, 12, 14};
        SDL_SetRenderDrawColor(renderer, (m_SelectedWeaponSlot == 1) ? 200 : 50,
                               50, 50, 255);
        SDL_RenderFillRect(renderer, &rRect);
        auto bTex = PixelsEngine::TextureManager::LoadTexture(renderer,
                                                              "assets/bow.png");
        if (bTex)
          bTex->Render(rRect.x + 1, rRect.y + 1, 10, 12);
      }
    }
  };

  // Actions Grid
  std::vector<std::string> actions = {"Atk", "Jmp", "Snk", "Shv", "Dsh", "End"};
  std::vector<std::string> actionKeys = {"SHT/F", "Z", "C", "V", "B", "SPC"};
  std::vector<std::string> actionIcons = {
      "assets/ui/action_attack.png", "assets/ui/action_jump.png",
      "assets/ui/action_sneak.png",  "assets/ui/action_shove.png",
      "assets/ui/action_dash.png",   "assets/ui/action_endturn.png"};
  DrawGrid("ACTIONS", 20, actions, actionKeys, actionIcons, {}, 6);

  // Spells Grid (K key opens menu, spells cast via click for now)
  std::vector<std::string> spells = {"Fir", "Hel", "Mis", "Shd", "", ""};
  std::vector<std::string> spellIcons = {"assets/ui/spell_fireball.png",
                                         "assets/ui/spell_heal.png",
                                         "assets/ui/spell_magicmissile.png",
                                         "assets/ui/spell_shield.png",
                                         "",
                                         ""};
  DrawGrid("SPELLS", 170, spells, {}, spellIcons, {}, 6);

  // Items Grid (Hotbar - Auto-populated with consumables)
  auto *playerInv =
      GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
  std::vector<std::string> hotbarItems = GetHotbarItems();
  std::vector<std::string> hotbarLabels; // Names for fallback
  std::vector<std::string> hotbarCounts; // Quantities for display
  std::vector<std::string> hotbarIcons;
  for (int i = 0; i < 6; ++i) {
    std::string name = hotbarItems[i];
    std::string label = "";
    std::string countStr = "";
    std::string icon = "";
    if (!name.empty()) {
      label = name.substr(0, 3); // Fallback short name
      // Find item in inventory to get icon and count
      PixelsEngine::Item *itemObj = nullptr;
      if (playerInv) {
        for (auto &it : playerInv->items) {
          if (it.name == name) {
            itemObj = &it;
            break;
          }
        }
      }

      if (itemObj) {
        countStr = std::to_string(itemObj->quantity);
        icon = itemObj->iconPath;
        if (icon.empty()) {
          if (name == "Potion")
            icon = "assets/ui/item_potion.png";
          else if (name == "Bread")
            icon = "assets/ui/item_bread.png";
        }
      }
    }
    hotbarLabels.push_back(label);
    hotbarCounts.push_back(countStr);
    hotbarIcons.push_back(icon);
  }
  DrawGrid("ITEMS", 320, hotbarLabels, {}, hotbarIcons, hotbarCounts, 6);

  // System Buttons (Map, Chr, Opt -> Rest, Inv)
  const char *sysLabels[] = {"Map", "Chr", "", "Inv"};
  const char *sysKeys[] = {"M", "O", "ESC", "I"};
  const char *sysIcons[] = {"", "", "assets/ui/action_rest.png", ""};
  for (int i = 0; i < 4; ++i) {
    SDL_Rect btn = {winW - 180 + (i % 2) * 85, winH - barH + 15 + (i / 2) * 40,
                    75, 35};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &btn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &btn);

    bool iconDrawn = false;
    if (sysIcons[i][0] != '\0') {
      auto tex =
          PixelsEngine::TextureManager::LoadTexture(renderer, sysIcons[i]);
      if (tex) {
        // If label is empty, center icon. Else shift.
        if (sysLabels[i][0] == '\0') {
          tex->Render(btn.x + (btn.w - 24) / 2, btn.y + (btn.h - 24) / 2, 24,
                      24);
        } else {
          tex->Render(btn.x + 45, btn.y + 5, 24, 24);
          m_TextRenderer->RenderText(sysLabels[i], btn.x + 5, btn.y + 10,
                                     {255, 255, 255, 255});
        }
        iconDrawn = true;
      }
    }

    if (!iconDrawn) {
      m_TextRenderer->RenderTextCentered(sysLabels[i], btn.x + 37, btn.y + 17,
                                         {255, 255, 255, 255});
    }

    m_TextRenderer->RenderTextSmall(sysKeys[i], btn.x + 5, btn.y + 22,
                                    {255, 255, 0, 200});
  }

  auto &camera = GetCamera();
  auto &interactions = GetRegistry().View<PixelsEngine::InteractionComponent>();
  for (auto &[entity, interact] : interactions) {
    if (interact.showDialogue) {
      auto *transform =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
      if (transform) {
        int screenX, screenY;
        m_Level->GridToScreen(transform->x, transform->y, screenX, screenY);
        screenX -= (int)camera.x;
        screenY -= (int)camera.y;

        int maxBubbleW = 200;
        // We don't have a way to measure text before rendering easily without
        // adding more helpers. Let's assume a safe height or just use
        // RenderTextWrapped and let it overflow if really long. But the prompt
        // specifically asked for the conversation text (UI) to fit.

        int w = std::min((int)interact.dialogueText.length() * 10, maxBubbleW);
        int h = 30; // Will grow if we had better measuring
        SDL_Rect bubble = {screenX + 16 - w / 2, screenY - 40, w, h};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        SDL_RenderFillRect(renderer, &bubble);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &bubble);
        m_TextRenderer->RenderTextWrapped(interact.dialogueText, bubble.x + 5,
                                          bubble.y + 5, w - 10, {0, 0, 0, 255});
      }
    }
  }

  // Render Tooltip on top of everything

  if (!m_HoveredItemName.empty()) {

    auto it = m_Tooltips.find(m_HoveredItemName);

    if (it != m_Tooltips.end()) {

      int tx, ty;

      if (m_TooltipPinned) {

        tx = m_PinnedTooltipX;

        ty = m_PinnedTooltipY;

      } else {

        int mx, my;
        PixelsEngine::Input::GetMousePosition(mx, my);

        tx = mx + 15;

        ty = my - 150;
      }

      RenderTooltip(it->second, tx, ty);
    }
  }
}

// --- Menu Implementations ---

void PixelsGateGame::HandleMenuNavigation(int numOptions,
                                          std::function<void(int)> onSelect,
                                          std::function<void()> onCancel,
                                          int forceSelection) {
  bool pressUp = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_W) ||
                 PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_UP);
  bool pressDown = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_S) ||
                   PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_DOWN);
  bool pressEnter = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_RETURN) ||
                    PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_SPACE);
  bool pressEsc = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE);

  // Mouse click handling (latch logic simplified since Input handles raw state)
  static bool wasMouse = false;
  bool isMouse = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT);
  bool pressMouse = isMouse && !wasMouse;
  wasMouse = isMouse;

  if (forceSelection != -1) {
    m_MenuSelection = forceSelection;
  }

  if (pressUp) {
    m_MenuSelection--;
    if (m_MenuSelection < 0)
      m_MenuSelection = numOptions - 1;
  }
  if (pressDown) {
    m_MenuSelection++;
    if (m_MenuSelection >= numOptions)
      m_MenuSelection = 0;
  }
  if ((pressEnter || (pressMouse && forceSelection != -1)) && onSelect) {
    onSelect(m_MenuSelection);
  }
  if (pressEsc && onCancel) {
    onCancel();
  }
}

void PixelsGateGame::HandleMainMenuInput() {
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  int hovered = -1;
  // Layout MUST match RenderMainMenu
  int y = 250;
  int numOptions = 6;
  for (int i = 0; i < numOptions; ++i) {
    // Assume text height ~30px, allow some padding.
    // Text is centered. Width is variable but let's assume a safe click area of
    // 300px width.
    SDL_Rect itemRect = {(w / 2) - 150, y - 5, 300, 30};
    if (mx >= itemRect.x && mx <= itemRect.x + itemRect.w && my >= itemRect.y &&
        my <= itemRect.y + itemRect.h) {
      hovered = i;
    }
    y += 40;
  }

  HandleMenuNavigation(
      6,
      [&](int selection) {
        switch (selection) {
        case 0: // Continue
        {
          std::string recentSave = "";
          namespace fs = std::filesystem;

          auto checkFile = [&](const std::string& name) -> std::string {
              if (fs::exists(name)) return name;
              if (fs::exists("../" + name)) return "../" + name;
              return "";
          };

          std::string savePath = checkFile("savegame.dat");
          std::string quickPath = checkFile("quicksave.dat");

          bool hasSave = !savePath.empty();
          bool hasQuick = !quickPath.empty();

          if (hasSave && hasQuick) {
            if (fs::last_write_time(quickPath) >
                fs::last_write_time(savePath)) {
              recentSave = quickPath;
            } else {
              recentSave = savePath;
            }
          } else if (hasSave) {
            recentSave = savePath;
          } else if (hasQuick) {
            recentSave = quickPath;
          }

          if (!recentSave.empty()) {
            PixelsEngine::SaveSystem::LoadWorldFlags(recentSave, m_WorldFlags);
            TriggerLoadTransition(recentSave);
          }
          break;
        }
        case 1: // New Game
        {
          // Reset player to default starting state
          m_WorldFlags.clear();

          // Clear all entities except player to avoid duplicates/ghosts
          auto &entities =
              GetRegistry().View<PixelsEngine::TransformComponent>();
          std::vector<PixelsEngine::Entity> toDestroy;
          for (auto &[entity, trans] : entities) {
            if (entity != m_Player)
              toDestroy.push_back(entity);
          }
          for (auto ent : toDestroy)
            GetRegistry().DestroyEntity(ent);

          auto *stats =
              GetRegistry().GetComponent<PixelsEngine::StatsComponent>(
                  m_Player);
          auto *inv =
              GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
                  m_Player);
          auto *trans =
              GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
                  m_Player);

          if (trans) {
            trans->x = 20.0f;
            trans->y = 20.0f;
          }
          if (stats) {
            *stats = PixelsEngine::StatsComponent{
                100, 100, 15, false, 0, 1, 1, false, false, 2, 2};
          }
          if (inv) {
            inv->items.clear();
            inv->equippedMelee = {"", "", 0,
                                  PixelsEngine::ItemType::WeaponMelee, 0};
            inv->equippedRanged = {"", "", 0,
                                   PixelsEngine::ItemType::WeaponRanged, 0};
            inv->equippedArmor = {"", "", 0, PixelsEngine::ItemType::Armor, 0};
            inv->AddItem("Potion", 3, PixelsEngine::ItemType::Consumable, 0,
                         "assets/ui/item_potion.png", 50);
            inv->AddItem("Thieves' Tools", 1, PixelsEngine::ItemType::Tool, 0,
                         "assets/thieves_tools.png", 25);
          }

          SpawnWorldEntities();

          m_State = GameState::Creation;
          selectionIndex = 0;
          pointsRemaining = 5;
          for (int i = 0; i < 6; ++i)
            tempStats[i] = 10;
          classIndex = 0;
          raceIndex = 0;
          break;
        }
        case 2: // Load Game
          if (std::filesystem::exists("savegame.dat"))
            TriggerLoadTransition("savegame.dat");
          else if (std::filesystem::exists("../savegame.dat"))
            TriggerLoadTransition("../savegame.dat");
          break;
        case 3: // Options
          m_ReturnState = m_State;
          m_State = GameState::Options;
          m_MenuSelection = 0;
          break;
        case 4: // Credits
          m_State = GameState::Credits;
          break;
        case 5: // Quit
          m_IsRunning = false;
          break;
        }
      },
      nullptr, hovered);
}

void PixelsGateGame::RenderMainMenu() {
  // 1. Render Background (Level)
  if (m_Level) {
    // Auto-scroll camera for effect
    static float scrollX = 0;
    scrollX += 0.5f;
    auto &camera = GetCamera();
    camera.x = (int)scrollX % (m_Level->GetWidth() * 32);
    camera.y = 200; // Fixed Y

    m_Level->Render(camera);
  }

  // 2. Dark Overlay
  SDL_Renderer *renderer = GetRenderer();
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
  SDL_Rect overlay = {0, 0, w, h};
  SDL_RenderFillRect(renderer, &overlay);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  // 3. Title
  m_TextRenderer->RenderTextCentered("PIXELS GATE", w / 2, 100,
                                     {255, 215, 0, 255});

  // 4. Menu Options
  std::vector<std::string> options = {"Continue", "New Game", "Load Game",
                                      "Options",  "Credits",  "Quit"};
  int y = 250;
  for (int i = 0; i < options.size(); ++i) {
    SDL_Color color = (i == m_MenuSelection) ? SDL_Color{50, 255, 50, 255}
                                             : SDL_Color{200, 200, 200, 255};
    m_TextRenderer->RenderTextCentered(options[i], w / 2, y, color);
    if (i == m_MenuSelection) {
      m_TextRenderer->RenderTextCentered(">", w / 2 - 100, y, color);
      m_TextRenderer->RenderTextCentered("<", w / 2 + 100, y, color);
    }
    y += 40;
  }

  m_TextRenderer->RenderTextCentered("v1.0.0", w - 50, h - 30,
                                     {100, 100, 100, 255});
}

void PixelsGateGame::RenderPauseMenu() {
  // 1. Dark Overlay (on top of frozen game)
  SDL_Renderer *renderer = GetRenderer();
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
  SDL_Rect overlay = {0, 0, w, h};
  SDL_RenderFillRect(renderer, &overlay);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  // 2. Menu Box
  int boxW = 300, boxH = 400;
  SDL_Rect box = {(w - boxW) / 2, (h - boxH) / 2, boxW, boxH};
  SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
  SDL_RenderFillRect(renderer, &box);
  SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
  SDL_RenderDrawRect(renderer, &box);

  m_TextRenderer->RenderTextCentered("PAUSED", w / 2, box.y + 30,
                                     {255, 255, 255, 255});

  std::vector<std::string> options = {"Resume",   "Save Game", "Load Game",
                                      "Controls", "Options",   "Main Menu"};
  int y = box.y + 80;
  for (int i = 0; i < options.size(); ++i) {
    SDL_Color color = (i == m_MenuSelection) ? SDL_Color{50, 255, 50, 255}
                                             : SDL_Color{200, 200, 200, 255};
    m_TextRenderer->RenderTextCentered(options[i], w / 2, y, color);
    if (i == m_MenuSelection) {
      m_TextRenderer->RenderTextCentered(">", w / 2 - 100, y, color);
      m_TextRenderer->RenderTextCentered("<", w / 2 + 100, y, color);
    }
    y += 40;
  }
}

void PixelsGateGame::HandlePauseMenuInput() {
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  int boxW = 300, boxH = 400;
  SDL_Rect box = {(w - boxW) / 2, (h - boxH) / 2, boxW, boxH};

  int hovered = -1;
  // Layout MUST match RenderPauseMenu
  int y = box.y + 80;
  int numOptions = 6;
  for (int i = 0; i < numOptions; ++i) {
    SDL_Rect itemRect = {(w / 2) - 100, y - 5, 200,
                         30}; // Smaller click area for pause menu items?
    if (mx >= itemRect.x && mx <= itemRect.x + itemRect.w && my >= itemRect.y &&
        my <= itemRect.y + itemRect.h) {
      hovered = i;
    }
    y += 40;
  }

  HandleMenuNavigation(
      6,
      [&](int selection) {
        switch (selection) {
        case 0: // Resume
          m_State = GameState::Playing;
          break;
        case 1: // Save
          PixelsEngine::SaveSystem::SaveGame(
              "savegame.dat", GetRegistry(), m_Player, *m_Level,
              m_State == GameState::Camp, m_LastWorldPos.x, m_LastWorldPos.y);
          ShowSaveMessage();
          break;
        case 2: // Load
          TriggerLoadTransition("savegame.dat");
          m_State = GameState::Playing; // Will happen after fade
          break;
        case 3: // Controls
          m_ReturnState = m_State;
          m_State = GameState::KeybindSettings;
          m_MenuSelection = 0;
          break;
        case 4: // Options
          m_ReturnState = m_State;
          m_State = GameState::Options;
          m_MenuSelection = 0;
          break;
        case 5: // Main Menu
          m_State = GameState::MainMenu;
          m_MenuSelection = 0;
          break;
        }
      },
      [&]() {
        // On Cancel (ESC)
        m_State = GameState::Playing;
      },
      hovered);
}

void PixelsGateGame::HandleOptionsInput() {
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  int hovered = -1;
  int y = h / 2 - 40;
  for (int i = 0; i < 2; ++i) {
    SDL_Rect itemRect = {(w / 2) - 150, y - 5, 300, 30};
    if (mx >= itemRect.x && mx <= itemRect.x + itemRect.w && my >= itemRect.y &&
        my <= itemRect.y + itemRect.h) {
      hovered = i;
    }
    y += 40;
  }

  HandleMenuNavigation(
      2,
      [&](int selection) {
        if (selection == 0) {
          ToggleFullScreen();
        } else if (selection == 1) {
          if (m_ReturnState != GameState::Options)
            m_State = m_ReturnState;
          else
            m_State = GameState::MainMenu;
        }
      },
      [&]() {
        if (m_ReturnState != GameState::Options)
          m_State = m_ReturnState;
        else
          m_State = GameState::MainMenu;
      },
      hovered);
}

void PixelsGateGame::RenderOptions() {
  SDL_Renderer *renderer = GetRenderer();
  SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
  SDL_RenderClear(renderer);

  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  m_TextRenderer->RenderTextCentered("OPTIONS", w / 2, 50,
                                     {255, 255, 255, 255});

  std::string options[] = {"Toggle Fullscreen", "Back"};
  int y = h / 2 - 40;
  for (int i = 0; i < 2; ++i) {
    SDL_Color color = (m_MenuSelection == i) ? SDL_Color{50, 255, 50, 255}
                                             : SDL_Color{200, 200, 200, 255};
    m_TextRenderer->RenderTextCentered(options[i], w / 2, y, color);
    if (i == m_MenuSelection) {
      m_TextRenderer->RenderTextCentered(">", w / 2 - 150, y, color);
      m_TextRenderer->RenderTextCentered("<", w / 2 + 150, y, color);
    }
    y += 40;
  }
}

void PixelsGateGame::RenderCredits() {
  SDL_Renderer *renderer = GetRenderer();
  SDL_SetRenderDrawColor(renderer, 10, 10, 30, 255);
  SDL_RenderClear(renderer);

  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  m_TextRenderer->RenderTextCentered("CREDITS", w / 2, 50,
                                     {255, 255, 255, 255});

  m_TextRenderer->RenderTextCentered("Created by Jesse Wood", w / 2, 200,
                                     {200, 200, 255, 255});
  m_TextRenderer->RenderTextCentered("Engine: PixelsEngine (Custom)", w / 2,
                                     240, {200, 200, 255, 255});
  m_TextRenderer->RenderTextCentered(
      "Assets: scrabling.itch.io/pixel-isometric-tiles", w / 2, 280,
      {200, 200, 255, 255});

  m_TextRenderer->RenderTextCentered("Press ESC to Back", w / 2, h - 50,
                                     {100, 100, 100, 255});

  if (PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_ESCAPE))
    m_State = GameState::MainMenu;
}

void PixelsGateGame::RenderControls() {
  SDL_Renderer *renderer = GetRenderer();
  SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
  SDL_RenderClear(renderer);

  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  m_TextRenderer->RenderTextCentered("CONTROLS", w / 2, 50,
                                     {255, 255, 255, 255});

  int y = 150;
  m_TextRenderer->RenderTextCentered("W/A/S/D - Move", w / 2, y,
                                     {200, 200, 200, 255});
  y += 40;
  m_TextRenderer->RenderTextCentered("Left Click - Move / Interact", w / 2, y,
                                     {200, 200, 200, 255});
  y += 40;
  m_TextRenderer->RenderTextCentered("Right Click - Context Menu", w / 2, y,
                                     {200, 200, 200, 255});
  y += 40;
  m_TextRenderer->RenderTextCentered("F5 - Quick Save", w / 2, y,
                                     {200, 200, 200, 255});
  y += 40;
  m_TextRenderer->RenderTextCentered("F8 - Quick Load", w / 2, y,
                                     {200, 200, 200, 255});
  y += 40;
  m_TextRenderer->RenderTextCentered("ESC - Pause Menu", w / 2, y,
                                     {200, 200, 200, 255});
  y += 40;

  m_TextRenderer->RenderTextCentered("Press ESC to Back", w / 2, h - 50,
                                     {100, 100, 100, 255});

  static bool wasEsc = false;
  bool isEsc = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_ESCAPE);
  if (isEsc && !wasEsc)
    m_State = GameState::Paused;
  wasEsc = isEsc;
}

// --- New Features Implementations ---

void PixelsGateGame::UpdateAI(float deltaTime) {
  auto *pTrans =
      GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
  auto *pStats =
      GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
  if (!pTrans || !pStats || pStats->isDead)
    return;

  auto &view = GetRegistry().View<PixelsEngine::AIComponent>();
  for (auto &[entity, ai] : view) {
    // If we are in combat, and this entity IS in the turn order, skip real-time
    // logic
    if (m_State == GameState::Combat && IsInTurnOrder(entity))
      continue;

    auto *transform =
        GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
    auto *anim =
        GetRegistry().GetComponent<PixelsEngine::AnimationComponent>(entity);

    if (!transform)
      continue;

    // Update Hostile Timer
    if (ai.hostileTimer > 0.0f) {
      ai.hostileTimer -= deltaTime;
      if (ai.hostileTimer <= 0.0f) {
        // Calm down if not naturally aggressive
        auto *tag =
            GetRegistry().GetComponent<PixelsEngine::TagComponent>(entity);
        if (tag && tag->tag != PixelsEngine::EntityTag::Hostile) {
          ai.isAggressive = false;
          SpawnFloatingText(transform->x, transform->y,
                            "Must have been the wind...", {200, 200, 200, 255});
        }
      }
    }

    float dist = std::sqrt(std::pow(pTrans->x - transform->x, 2) +
                           std::pow(pTrans->y - transform->y, 2));

    bool canSee = false;
    if (ai.isAggressive && dist <= ai.sightRange && !pStats->isStealthed) {
      float dx = pTrans->x - transform->x;
      float dy = pTrans->y - transform->y;
      float angleToPlayer = std::atan2(dy, dx) * (180.0f / M_PI); // Degrees

      // Normalize angles to -180 to 180
      float angleDiff = angleToPlayer - ai.facingDir;
      while (angleDiff > 180.0f)
        angleDiff -= 360.0f;
      while (angleDiff < -180.0f)
        angleDiff += 360.0f;

      if (std::abs(angleDiff) <= ai.coneAngle / 2.0f) {
        canSee = true;
      }
    }

    if (canSee) {
      // Chase
      if (dist > ai.attackRange) {
        float dx = pTrans->x - transform->x;
        float dy = pTrans->y - transform->y;
        // Normalize
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0) {
          dx /= len;
          dy /= len;
        }

        float speed = 2.0f; // Slower than player
        transform->x += dx * speed * deltaTime;
        transform->y += dy * speed * deltaTime;

        // Update facing direction when moving
        ai.facingDir = std::atan2(dy, dx) * (180.0f / M_PI);

        if (anim)
          anim->Play("Run");
      } else {
        // Attack
        if (anim)
          anim->Play("Idle");
        ai.attackTimer -= deltaTime;
        if (ai.attackTimer <= 0.0f) {
          // Deal Damage (Ambush Attack)
          int dmg = 2; // Default
          auto *eStats =
              GetRegistry().GetComponent<PixelsEngine::StatsComponent>(entity);
          if (eStats)
            dmg = eStats->damage;

          pStats->currentHealth -= dmg;
          if (pStats->currentHealth < 0)
            pStats->currentHealth = 0;
          SpawnFloatingText(pTrans->x, pTrans->y, "-" + std::to_string(dmg),
                            {255, 0, 0, 255});

          ai.attackTimer = ai.attackCooldown;

          // Trigger Combat or Join Reinforcements
          StartCombat(entity);

          // If surprise was rolled (only if starting new combat)
          if (m_State == GameState::Combat && IsInTurnOrder(entity)) {
            // Perception Check (Player Wisdom vs Enemy Stealth/DC)
            int perception = PixelsEngine::Dice::Roll(20) +
                             pStats->GetModifier(pStats->wisdom);
            int enemyDC = 12;

            if (perception < enemyDC) {
              // Only apply surprise if this just STARTED combat (simplified
              // check) For now, let's just show text
              SpawnFloatingText(pTrans->x, pTrans->y - 1.0f, "SURPRISED!",
                                {255, 100, 0, 255});
              for (auto &turn : m_Combat.m_TurnOrder) {
                if (turn.isPlayer)
                  turn.isSurprised = true;
              }
            }
          }
        }
      }
    } else {
      if (anim)
        anim->Play("Idle");
    }
  }
}

void PixelsGateGame::UpdateDayNight(float deltaTime) {
  m_Time.Update(deltaTime);
}

void PixelsGateGame::RenderDayNightCycle() {
  SDL_Renderer *renderer = GetRenderer();
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);

  SDL_Color tint = {0, 0, 0, 0};

  // Interpolation helpers
  auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
  auto lerpColor = [&](SDL_Color c1, SDL_Color c2, float t) {
    return SDL_Color{(Uint8)lerp(c1.r, c2.r, t), (Uint8)lerp(c1.g, c2.g, t),
                     (Uint8)lerp(c1.b, c2.b, t), (Uint8)lerp(c1.a, c2.a, t)};
  };

  // Define cycle stages
  if (m_Time.m_TimeOfDay >= 5.0f && m_Time.m_TimeOfDay < 8.0f) { // Sunrise to Early Day
    float t = (m_Time.m_TimeOfDay - 5.0f) / 3.0f;
    tint = lerpColor({255, 200, 100, 40}, {20, 20, 40, 20}, t);
  } else if (m_Time.m_TimeOfDay >= 8.0f && m_Time.m_TimeOfDay < 17.0f) { // Day
    tint = {20, 20, 40, 20}; // Subtle ambient darkening during day
  } else if (m_Time.m_TimeOfDay >= 17.0f &&
             m_Time.m_TimeOfDay < 20.0f) { // Late Day to Sunset
    float t = (m_Time.m_TimeOfDay - 17.0f) / 3.0f;
    tint = lerpColor({20, 20, 40, 20}, {255, 100, 50, 60}, t);
  } else if (m_Time.m_TimeOfDay >= 20.0f && m_Time.m_TimeOfDay < 23.0f) { // Sunset to Night
    float t = (m_Time.m_TimeOfDay - 20.0f) / 3.0f;
    tint = lerpColor({255, 100, 50, 60}, {10, 10, 50, 110}, t);
  } else if (m_Time.m_TimeOfDay >= 23.0f || m_Time.m_TimeOfDay < 4.0f) { // Deep Night
    tint = {10, 10, 50, 110}; // Reduced from 180 for better visibility
  } else {                    // Night to Sunrise (4.0 to 5.0)
    float t = (m_Time.m_TimeOfDay - 4.0f) / 1.0f;
    tint = lerpColor({10, 10, 50, 110}, {255, 200, 100, 40}, t);
  }

  if (tint.a > 0) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, tint.r, tint.g, tint.b, tint.a);
    SDL_Rect screen = {0, 0, w, h};
    SDL_RenderFillRect(renderer, &screen);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  }

  // Render Clock UI
  m_TextRenderer->RenderText("Time: " + std::to_string((int)m_Time.m_TimeOfDay) +
                                 ":00",
                             w - 120, 10, {255, 255, 255, 255});
}

void PixelsGateGame::SpawnFloatingText(float x, float y,
                                       const std::string &text,
                                       SDL_Color color) {
  m_FloatingText.m_Texts.push_back({x, y, text, 1.5f, color});
}

void PixelsGateGame::SpawnLootBag(
    float x, float y, const std::vector<PixelsEngine::Item> &items) {
  auto bag = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(bag, PixelsEngine::TransformComponent{x, y});
  auto tex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/gold_orb.png"); // Reuse orb sprite for now
  GetRegistry().AddComponent(
      bag, PixelsEngine::SpriteComponent{tex, {0, 0, 32, 32}, 16, 16});
  GetRegistry().AddComponent(
      bag, PixelsEngine::InteractionComponent{"Loot", "loot_bag", false, 0.0f});

  PixelsEngine::LootComponent loot;
  loot.drops = items;
  GetRegistry().AddComponent(bag, loot);
}

std::vector<std::string> PixelsGateGame::GetHotbarItems() {
  std::vector<std::string> result;
  auto *inv =
      GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
  if (inv) {
    for (const auto &item : inv->items) {
      if (item.type == PixelsEngine::ItemType::Consumable ||
          item.type == PixelsEngine::ItemType::Tool ||
          item.type == PixelsEngine::ItemType::Scroll ||
          item.type == PixelsEngine::ItemType::Ammo) {
        if (result.size() < 6)
          result.push_back(item.name);
      }
    }
  }
  while (result.size() < 6)
    result.push_back("");
  return result;
}

void PixelsGateGame::UseItem(const std::string &itemName) {
  auto *inv =
      GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
  auto *stats =
      GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
  auto *trans =
      GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
  if (!inv || !stats || !trans)
    return;

  // Find the item
  PixelsEngine::Item *item = nullptr;
  size_t itemIndex = 0;
  for (size_t i = 0; i < inv->items.size(); ++i) {
    if (inv->items[i].name == itemName) {
      item = &inv->items[i];
      itemIndex = i;
      break;
    }
  }

  if (!item) {
    SpawnFloatingText(trans->x, trans->y - 1.0f, "Out of " + itemName,
                      {200, 200, 200, 255});
    return;
  }

  if (m_State == GameState::Combat && m_Combat.m_BonusActionsLeft <= 0) {
    SpawnFloatingText(trans->x, trans->y - 1.0f, "No Bonus Action!",
                      {255, 0, 0, 255});
    return;
  }

  bool used = false;
  if (item->type == PixelsEngine::ItemType::Consumable ||
      item->type == PixelsEngine::ItemType::Scroll ||
      item->type == PixelsEngine::ItemType::Ammo) {

    if (itemName == "Potion") {
      stats->currentHealth =
          std::min(stats->maxHealth, stats->currentHealth + 20);
      SpawnFloatingText(trans->x, trans->y, "+20 HP", {0, 255, 0, 255});
      used = true;
    } else if (itemName == "Bread") {
      stats->currentHealth =
          std::min(stats->maxHealth, stats->currentHealth + 5);
      SpawnFloatingText(trans->x, trans->y, "+5 HP", {0, 255, 0, 255});
      used = true;
    } else if (item->type == PixelsEngine::ItemType::Scroll) {
      SpawnFloatingText(trans->x, trans->y, "Used Scroll: " + itemName,
                        {200, 100, 255, 255});
      used = true;
    } else if (item->type == PixelsEngine::ItemType::Ammo) {
      SpawnFloatingText(trans->x, trans->y, "Ammo selected",
                        {200, 200, 200, 255});
      // No direct effect, but allows showing on bar
    }

    if (used) {
      item->quantity--;
      if (item->quantity <= 0) {
        inv->items.erase(inv->items.begin() + itemIndex);
      }
    }
  } else if (item->type == PixelsEngine::ItemType::Tool) {
    SpawnFloatingText(trans->x, trans->y, "Use this on a locked object",
                      {200, 200, 200, 255});
  }

  if (used && m_State == GameState::Combat) {
    m_Combat.m_BonusActionsLeft--;
  }
}

void PixelsGateGame::HandleInventoryInput() {
  int winW, winH;
  SDL_GetWindowSize(m_Window, &winW, &winH);
  int panelW = 800, panelH = 500;
  SDL_Rect panel = {(winW - panelW) / 2, (winH - panelH) / 2, panelW, panelH};

  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);
  auto *inv =
      GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
  if (!inv)
    return;

  bool pressed = PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT);
  bool isDown = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT);

  static bool wasDownForReleased = false;
  bool released = !isDown && wasDownForReleased;
  wasDownForReleased = isDown;

  float currentTime = SDL_GetTicks() / 1000.0f;

  if (pressed) {
    // Check for double click or start drag
    int listX = panel.x + 200;
    int itemY = panel.y + 50;
    for (int i = 0; i < (int)inv->items.size(); ++i) {
      SDL_Rect rowRect = {listX, itemY, panelW - 250, 40};
      if (mx >= rowRect.x && mx <= rowRect.x + rowRect.w && my >= rowRect.y &&
          my <= rowRect.y + rowRect.h) {
        // Potential Double Click
        if (i == m_LastClickedItemIndex &&
            (currentTime - m_LastClickTime) < 0.5f) {
          PixelsEngine::Item &item = inv->items[i];
          bool equipped = false;
          if (item.type == PixelsEngine::ItemType::WeaponMelee) {
            if (!inv->equippedMelee.IsEmpty())
              inv->AddItemObject(inv->equippedMelee);
            inv->equippedMelee = item;
            inv->equippedMelee.quantity = 1;
            equipped = true;
          } else if (item.type == PixelsEngine::ItemType::WeaponRanged) {
            if (!inv->equippedRanged.IsEmpty())
              inv->AddItemObject(inv->equippedRanged);
            inv->equippedRanged = item;
            inv->equippedRanged.quantity = 1;
            equipped = true;
          } else if (item.type == PixelsEngine::ItemType::Armor) {
            if (!inv->equippedArmor.IsEmpty())
              inv->AddItemObject(inv->equippedArmor);
            inv->equippedArmor = item;
            inv->equippedArmor.quantity = 1;
            equipped = true;
          }
          if (equipped) {
            item.quantity--;
            if (item.quantity <= 0)
              inv->items.erase(inv->items.begin() + i);
            m_LastClickedItemIndex = -1;
            m_DraggingItemIndex = -1;
            return;
          }
        }
        m_LastClickTime = currentTime;
        m_LastClickedItemIndex = i;
        m_DraggingItemIndex = i;
      }
      itemY += 45;
    }

    // Check Unequip
    int startX = panel.x + 50;
    int startY = panel.y + 60;
    int slotSize = 64;
    auto CheckUnequip = [&](PixelsEngine::Item &slotItem, int x, int y) {
      SDL_Rect r = {x, y, slotSize, slotSize};
      if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) {
        if (!slotItem.IsEmpty()) {
          inv->AddItemObject(slotItem);
          slotItem = {"", "", 0, PixelsEngine::ItemType::Misc, 0};
        }
      }
    };
    CheckUnequip(inv->equippedMelee, startX, startY);
    CheckUnequip(inv->equippedRanged, startX, startY + 100);
    CheckUnequip(inv->equippedArmor, startX, startY + 200);
  }

  if (released && m_DraggingItemIndex != -1) {
    if (m_DraggingItemIndex < (int)inv->items.size()) {
      int startX = panel.x + 50;
      int startY = panel.y + 60;
      int slotSize = 64;

      PixelsEngine::Item &dragItem = inv->items[m_DraggingItemIndex];
      bool dropped = false;

      auto CheckDrop = [&](PixelsEngine::Item &slotItem, int x, int y,
                           PixelsEngine::ItemType allowed) {
        SDL_Rect r = {x, y, slotSize, slotSize};
        if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h &&
            dragItem.type == allowed) {
          if (!slotItem.IsEmpty())
            inv->AddItemObject(slotItem);
          slotItem = dragItem;
          slotItem.quantity = 1;
          dropped = true;
        }
      };

      CheckDrop(inv->equippedMelee, startX, startY,
                PixelsEngine::ItemType::WeaponMelee);
      CheckDrop(inv->equippedRanged, startX, startY + 100,
                PixelsEngine::ItemType::WeaponRanged);
      CheckDrop(inv->equippedArmor, startX, startY + 200,
                PixelsEngine::ItemType::Armor);

      if (dropped) {
        dragItem.quantity--;
        if (dragItem.quantity <= 0)
          inv->items.erase(inv->items.begin() + m_DraggingItemIndex);
      }
    }
    m_DraggingItemIndex = -1;
  }
}

void PixelsGateGame::StartCombat(PixelsEngine::Entity enemy) {
  if (m_State == GameState::Combat) {
    // Combat already active, try to add this enemy if not already in turn order
    if (!IsInTurnOrder(enemy)) {
      auto *eStats =
          GetRegistry().GetComponent<PixelsEngine::StatsComponent>(enemy);
      if (eStats && !eStats->isDead) {
        int eInit = PixelsEngine::Dice::Roll(20) +
                    eStats->GetModifier(eStats->dexterity);
        m_Combat.m_TurnOrder.push_back({enemy, eInit, false});
        // Sort to keep order consistent
        std::sort(m_Combat.m_TurnOrder.begin(), m_Combat.m_TurnOrder.end(),
                  [](const CombatManager::CombatTurn &a, const CombatManager::CombatTurn &b) {
                    return a.initiative > b.initiative;
                  });
        SpawnFloatingText(0, 0, "Reinforcements!", {255, 100, 0, 255});
      }
    }
    return;
  }

  auto *eStartStats =
      GetRegistry().GetComponent<PixelsEngine::StatsComponent>(enemy);
  if (eStartStats && eStartStats->isDead)
    return;

  m_Combat.m_TurnOrder.clear();
  bool anyEnemyAdded = false;

  // Add Player
  auto *pStats =
      GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
  int pInit = PixelsEngine::Dice::Roll(20) +
              (pStats ? pStats->GetModifier(pStats->dexterity) : 0);
  m_Combat.m_TurnOrder.push_back({m_Player, pInit, true});
  SpawnFloatingText(20, 20, "Initiative!", {0, 255, 255, 255});

  // Add Target Enemy
  if (GetRegistry().Valid(enemy)) {
    auto *eStats =
        GetRegistry().GetComponent<PixelsEngine::StatsComponent>(enemy);
    if (eStats && !eStats->isDead) {
      int eInit = PixelsEngine::Dice::Roll(20) +
                  (eStats ? eStats->GetModifier(eStats->dexterity) : 0);
      m_Combat.m_TurnOrder.push_back({enemy, eInit, false});
      anyEnemyAdded = true;
    }
  }

  // Find nearby enemies
  auto *pTrans =
      GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
  auto &view = GetRegistry().View<PixelsEngine::AIComponent>();
  for (auto &[ent, ai] : view) {
    if (ent == enemy)
      continue; // Already added
    auto *t = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(ent);
    if (t && pTrans) {
      float dist = std::sqrt(std::pow(t->x - pTrans->x, 2) +
                             std::pow(t->y - pTrans->y, 2));
      if (dist < 15.0f) { // Combat radius
        auto *eStats =
            GetRegistry().GetComponent<PixelsEngine::StatsComponent>(ent);
        if (eStats && !eStats->isDead &&
            ai.isAggressive) { // Only add aggressive nearby NPCs
          int eInit = PixelsEngine::Dice::Roll(20) +
                      eStats->GetModifier(eStats->dexterity);
          m_Combat.m_TurnOrder.push_back({ent, eInit, false});
          anyEnemyAdded = true;
        }
      }
    }
  }

  if (!anyEnemyAdded) {
    m_Combat.m_TurnOrder.clear();
    return; // Don't start combat if no enemies
  }

  // Sort
  std::sort(m_Combat.m_TurnOrder.begin(), m_Combat.m_TurnOrder.end(),
            [](const CombatManager::CombatTurn &a, const CombatManager::CombatTurn &b) {
              return a.initiative > b.initiative; // Descending
            });

  m_State = GameState::Combat;
  m_Combat.m_CurrentTurnIndex = -1;
  NextTurn();
}

void PixelsGateGame::EndCombat() {
  m_State = GameState::Playing;
  m_Combat.m_TurnOrder.clear();
  SpawnFloatingText(0, 0, "Combat Ended", {0, 255, 0, 255});
}

void PixelsGateGame::NextTurn() {
  // Check if combat is over
  bool enemiesAlive = false;
  for (const auto &turn : m_Combat.m_TurnOrder) {
    if (!turn.isPlayer) {
      auto *stats =
          GetRegistry().GetComponent<PixelsEngine::StatsComponent>(turn.entity);
      if (stats && !stats->isDead)
        enemiesAlive = true;
    }
  }

  auto *pStats =
      GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
  if (!pStats || pStats->isDead) {
    EndCombat();
    return;
  }

  if (!enemiesAlive) {
    EndCombat();
    return;
  }

  m_Combat.m_CurrentTurnIndex++;
  if (m_Combat.m_CurrentTurnIndex >= m_Combat.m_TurnOrder.size())
    m_Combat.m_CurrentTurnIndex = 0;

  auto &current = m_Combat.m_TurnOrder[m_Combat.m_CurrentTurnIndex];
  auto *cStats =
      GetRegistry().GetComponent<PixelsEngine::StatsComponent>(current.entity);

  // Check Surprised
  if (current.isSurprised) {
    current.isSurprised = false; // Clear for next round
    SpawnFloatingText(0, 0, "Surprised! Skipping Turn", {255, 100, 0, 255});
    NextTurn();
    return;
  }

  // Skip dead
  if (!GetRegistry().HasComponent<PixelsEngine::StatsComponent>(
          current.entity) ||
      (cStats && cStats->isDead)) {
    // Infinite recursion protection needed ideally, but assumes at least player
    // alive
    NextTurn();
    return;
  }

  // Restore Resources
  m_Combat.m_ActionsLeft = 1;
  m_Combat.m_BonusActionsLeft = 1;
  m_Combat.m_MovementLeft = 5.0f; // Default
  if (current.isPlayer) {
    auto *pc =
        GetRegistry().GetComponent<PixelsEngine::PlayerComponent>(m_Player);
    if (pc)
      m_Combat.m_MovementLeft = pc->speed;
    SpawnFloatingText(0, 0, "YOUR TURN", {0, 255, 0, 255});
  } else {
    m_Combat.m_CombatTurnTimer = 1.0f; // AI thinking time
    auto *ai =
        GetRegistry().GetComponent<PixelsEngine::AIComponent>(current.entity);
    if (ai)
      m_Combat.m_MovementLeft = ai->sightRange;
  }
}

void PixelsGateGame::UpdateCombat(float deltaTime) {
  if (m_Combat.m_CurrentTurnIndex < 0 || m_Combat.m_CurrentTurnIndex >= m_Combat.m_TurnOrder.size())
    return;

  auto &turn = m_Combat.m_TurnOrder[m_Combat.m_CurrentTurnIndex];

  if (turn.isPlayer) {
    HandleCombatInput();
  } else {
    // AI Logic
    auto *aiTrans =
        GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
            turn.entity);
    auto *pTrans =
        GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);
    auto *aiStats =
        GetRegistry().GetComponent<PixelsEngine::StatsComponent>(turn.entity);
    auto *aiComp =
        GetRegistry().GetComponent<PixelsEngine::AIComponent>(turn.entity);
    auto *anim = GetRegistry().GetComponent<PixelsEngine::AnimationComponent>(
        turn.entity);

    if (!aiTrans || !pTrans || !aiStats || aiStats->isDead || !aiComp) {
      NextTurn();
      return;
    }

    if (m_Combat.m_CombatTurnTimer > 0.0f) {
      m_Combat.m_CombatTurnTimer -= deltaTime;
      if (m_Combat.m_CombatTurnTimer <= 0.0f) {
        // Thinking done, calculate path
        m_CurrentAIPath = PixelsEngine::Pathfinding::FindPath(
            *m_Level, (int)aiTrans->x, (int)aiTrans->y, (int)pTrans->x,
            (int)pTrans->y);
        m_CurrentAIPathIndex = 1; // Skip start node
      }
      return;
    }

    // --- Movement Phase ---
    bool moving = false;
    if (m_CurrentAIPathIndex != -1 &&
        m_CurrentAIPathIndex < m_CurrentAIPath.size() &&
        m_Combat.m_MovementLeft > 0.1f) {
      float distToPlayer = std::sqrt(std::pow(aiTrans->x - pTrans->x, 2) +
                                     std::pow(aiTrans->y - pTrans->y, 2));

      if (distToPlayer > aiComp->attackRange) {
        float targetX = (float)m_CurrentAIPath[m_CurrentAIPathIndex].first;
        float targetY = (float)m_CurrentAIPath[m_CurrentAIPathIndex].second;

        float dx = targetX - aiTrans->x;
        float dy = targetY - aiTrans->y;
        float distStep = std::sqrt(dx * dx + dy * dy);

        if (distStep < 0.05f) {
          m_CurrentAIPathIndex++;
        } else {
          float speed = 4.0f; // Movement speed
          float moveThisFrame = speed * deltaTime;

          if (moveThisFrame > distStep)
            moveThisFrame = distStep;
          if (moveThisFrame > m_Combat.m_MovementLeft)
            moveThisFrame = m_Combat.m_MovementLeft;

          aiTrans->x += (dx / distStep) * moveThisFrame;
          aiTrans->y += (dy / distStep) * moveThisFrame;
          m_Combat.m_MovementLeft -= moveThisFrame;
          moving = true;

          if (anim)
            anim->Play("Run");
        }
      }
    }

    if (moving)
      return; // Keep moving next frame

    // --- Attack Phase (After movement finished) ---
    if (anim)
      anim->Play("Idle");

    float finalDist = std::sqrt(std::pow(aiTrans->x - pTrans->x, 2) +
                                std::pow(aiTrans->y - pTrans->y, 2));
    if (finalDist <= aiComp->attackRange && m_Combat.m_ActionsLeft > 0) {
      auto *pTargetStats =
          GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
      auto *pInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
          m_Player);
      if (pTargetStats && pInv) {
        int ac = 10 + pTargetStats->GetModifier(pTargetStats->dexterity) +
                 pInv->equippedArmor.statBonus;
        int roll = PixelsEngine::Dice::Roll(20);

        if (roll == 20 ||
            (roll != 1 &&
             roll + aiStats->GetModifier(aiStats->strength) >= ac)) {
          int dmg = aiStats->damage;
          if (roll == 20)
            dmg *= 2;
          pTargetStats->currentHealth -= dmg;
          if (pTargetStats->currentHealth < 0)
            pTargetStats->currentHealth = 0;
          SpawnFloatingText(pTrans->x, pTrans->y,
                            std::to_string(dmg) + (roll == 20 ? "!" : ""),
                            {255, 0, 0, 255});
        } else {
          SpawnFloatingText(pTrans->x, pTrans->y, "Miss", {200, 200, 200, 255});
        }
        m_Combat.m_ActionsLeft--;
      }
    }

    m_CurrentAIPathIndex = -1; // Reset for next turn
    NextTurn();
  }
}

void PixelsGateGame::HandleCombatInput() {
  if (PixelsEngine::Input::IsKeyPressed(PixelsEngine::Config::GetKeybind(
          PixelsEngine::GameAction::EndTurn))) {
    NextTurn();
    return;
  }

  float dx = 0, dy = 0;
  if (PixelsEngine::Input::IsKeyDown(
          PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::MoveUp)))
    dy -= 1;
  if (PixelsEngine::Input::IsKeyDown(
          PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::MoveDown)))
    dy += 1;
  if (PixelsEngine::Input::IsKeyDown(
          PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::MoveLeft)))
    dx -= 1;
  if (PixelsEngine::Input::IsKeyDown(PixelsEngine::Config::GetKeybind(
          PixelsEngine::GameAction::MoveRight)))
    dx += 1;

  if ((dx != 0 || dy != 0) && m_Combat.m_MovementLeft > 0) {
    float speed = 5.0f;
    float moveCost = speed * 0.016f;
    if (m_Combat.m_MovementLeft >= moveCost) {
      auto *transform =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
              m_Player);
      auto *anim = GetRegistry().GetComponent<PixelsEngine::AnimationComponent>(
          m_Player);
      if (transform) {
        float newX = transform->x + dx * moveCost;
        float newY = transform->y + dy * moveCost;

        bool moved = false;
        if (m_Level->IsWalkable((int)newX, (int)newY)) {
          transform->x = newX;
          transform->y = newY;
          moved = true;
        } else {
          // Sliding
          if (m_Level->IsWalkable((int)newX, (int)transform->y)) {
            transform->x = newX;
            moved = true;
          } else if (m_Level->IsWalkable((int)transform->x, (int)newY)) {
            transform->y = newY;
            moved = true;
          }
        }

        if (moved)
          m_Combat.m_MovementLeft -= moveCost;

        if (anim) {
          if (dy < 0)
            anim->Play("WalkUp");
          else if (dy > 0)
            anim->Play("WalkDown");
          else if (dx < 0)
            anim->Play("WalkLeft");
          else
            anim->Play("WalkRight");
        }
      }
    }
  }

  bool isAttackModifierDown =
      PixelsEngine::Input::IsKeyDown(PixelsEngine::Config::GetKeybind(
          PixelsEngine::GameAction::AttackModifier));
  bool isCtrlDown = PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_LCTRL) ||
                    PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_RCTRL);

  bool isDownLeftRaw = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT);
  bool isPressedLeftRaw =
      PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT);

  // CTRL + CLICK = Right Click (Context Menu)
  if (isPressedLeftRaw && isCtrlDown) {
    int mx, my;
    PixelsEngine::Input::GetMousePosition(mx, my);
    auto &camera = GetCamera();
    auto &transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
    bool found = false;
    for (auto &[entity, transform] : transforms) {
      if (entity == m_Player)
        continue;
      auto *sprite =
          GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
      if (sprite) {
        int screenX, screenY;
        m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
        screenX -= (int)camera.x;
        screenY -= (int)camera.y;
        SDL_Rect drawRect = {screenX + 16 - sprite->pivotX,
                             screenY + 16 - sprite->pivotY, sprite->srcRect.w,
                             sprite->srcRect.h};
        if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w &&
            my >= drawRect.y && my <= drawRect.y + drawRect.h) {
          m_ContextMenu.isOpen = true;
          m_ContextMenu.x = mx;
          m_ContextMenu.y = my;
          m_ContextMenu.targetEntity = entity;
          m_ContextMenu.actions.clear();
          m_ContextMenu.actions.push_back(
              {"Attack", PixelsEngine::ContextActionType::Attack});

          auto *tag =
              GetRegistry().GetComponent<PixelsEngine::TagComponent>(entity);
          bool isLiving =
              (tag && (tag->tag == PixelsEngine::EntityTag::NPC ||
                       tag->tag == PixelsEngine::EntityTag::Trader ||
                       tag->tag == PixelsEngine::EntityTag::Companion ||
                       tag->tag == PixelsEngine::EntityTag::Hostile));

          if (isLiving &&
              GetRegistry().HasComponent<PixelsEngine::InteractionComponent>(
                  entity)) {
            m_ContextMenu.actions.push_back(
                {"Talk", PixelsEngine::ContextActionType::Talk});
            m_ContextMenu.actions.push_back(
                {"Pickpocket", PixelsEngine::ContextActionType::Pickpocket});
            m_ContextMenu.actions.push_back(
                {"Trade", PixelsEngine::ContextActionType::Trade});
          }

          auto *lock =
              GetRegistry().GetComponent<PixelsEngine::LockComponent>(entity);
          if (lock && lock->isLocked) {
            m_ContextMenu.actions.push_back(
                {"Lockpick", PixelsEngine::ContextActionType::Lockpick});
          }
          found = true;
          break;
        }
      }
    }
    if (!found)
      m_ContextMenu.isOpen = false;
  }

  if (isPressedLeftRaw && !isCtrlDown) {
    int mx, my;
    PixelsEngine::Input::GetMousePosition(mx, my);
    int winW, winH;
    SDL_GetWindowSize(m_Window, &winW, &winH);

    auto *inv =
        GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if (inv && inv->isOpen) {
      int iw = 400, ih = 500;
      SDL_Rect panel = {(winW - iw) / 2, (winH - ih) / 2, iw, ih};
      if (mx >= panel.x && mx <= panel.x + iw && my >= panel.y &&
          my <= panel.y + ih)
        return;
    }

    if (my > winH - 90) {
      CheckUIInteraction(mx, my);
    } else {
      if (isAttackModifierDown) {
        auto &camera = GetCamera();
        auto &transforms =
            GetRegistry().View<PixelsEngine::TransformComponent>();
        bool found = false;
        for (auto &[entity, transform] : transforms) {
          if (entity == m_Player)
            continue;
          auto *sprite =
              GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
          if (sprite) {
            int screenX, screenY;
            m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
            screenX -= (int)camera.x;
            screenY -= (int)camera.y;
            SDL_Rect drawRect = {screenX + 16 - sprite->pivotX,
                                 screenY + 16 - sprite->pivotY,
                                 sprite->srcRect.w, sprite->srcRect.h};
            if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w &&
                my >= drawRect.y && my <= drawRect.y + drawRect.h) {
              PerformAttack(entity);
              found = true;
              break;
            }
          }
        }
        if (!found)
          CheckWorldInteraction(mx, my);
      } else {
        CheckWorldInteraction(mx, my);
      }
    }
  }
}

void PixelsGateGame::RenderCombatUI() {
  SDL_Renderer *renderer = GetRenderer();
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);

  // Turn Order Bar
  std::vector<CombatManager::CombatTurn> aliveParticipants;
  int currentActiveAliveIndex = -1;
  for (int i = 0; i < m_Combat.m_TurnOrder.size(); ++i) {
    auto *entStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(
        m_Combat.m_TurnOrder[i].entity);
    if (m_Combat.m_TurnOrder[i].isPlayer || (entStats && !entStats->isDead)) {
      if (i == m_Combat.m_CurrentTurnIndex)
        currentActiveAliveIndex = aliveParticipants.size();
      aliveParticipants.push_back(m_Combat.m_TurnOrder[i]);
    }
  }

  int count = aliveParticipants.size();
  int slotW = 50;
  int startX = (w - (count * slotW)) / 2;
  int startY = 10;

  for (int i = 0; i < count; ++i) {
    SDL_Rect slot = {startX + i * slotW, startY, slotW - 5, 30};

    SDL_Color bg = {100, 100, 100, 255};
    if (i == currentActiveAliveIndex)
      bg = {0, 200, 0, 255}; // Highlight current
    else if (aliveParticipants[i].isPlayer)
      bg = {50, 50, 200, 255}; // Player blue

    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(renderer, &slot);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &slot);

    auto *entStats = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(
        aliveParticipants[i].entity);
    std::string label = "HP: ??%";
    if (entStats) {
      int hpPercent = (int)((float)entStats->currentHealth /
                            (float)entStats->maxHealth * 100.0f);
      label = std::to_string(hpPercent) + "%";
    }
    m_TextRenderer->RenderTextCentered(label, slot.x + slotW / 2, slot.y + 15,
                                       {255, 255, 255, 255});
  }

  // Resources UI
  std::string resStr = "Actions: " + std::to_string(m_Combat.m_ActionsLeft) +
                       " | Bonus: " + std::to_string(m_Combat.m_BonusActionsLeft) +
                       " | Move: " + std::to_string((int)m_Combat.m_MovementLeft);

  m_TextRenderer->RenderTextCentered(resStr, w / 2, h - 150,
                                     {255, 255, 0, 255});

  if (m_Combat.m_TurnOrder[m_Combat.m_CurrentTurnIndex].isPlayer) {
    m_TextRenderer->RenderTextCentered("Press SPACE to End Turn", w / 2,
                                       h - 125, {200, 200, 200, 255});
  } else {
    m_TextRenderer->RenderTextCentered("Enemy Turn...", w / 2, h - 125,
                                       {255, 100, 100, 255});
  }
}
void PixelsGateGame::RenderEnemyCones(const PixelsEngine::Camera &camera) {
  if (!PixelsEngine::Input::IsKeyDown(SDL_SCANCODE_LSHIFT))
    return;

  SDL_Renderer *renderer = GetRenderer();
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  int winW, winH;
  SDL_GetWindowSize(m_Window, &winW, &winH);

  auto &view = GetRegistry().View<PixelsEngine::AIComponent>();
  for (auto &[entity, ai] : view) {
    auto *transform =
        GetRegistry().GetComponent<PixelsEngine::TransformComponent>(entity);
    if (!transform)
      continue;

    // Center of the entity (assuming 32x32 tiles and entity center is roughly
    // center of tile) Transform->x/y are in tile coordinates
    float cx = transform->x + 0.5f;
    float cy = transform->y + 0.5f;

    // Calculate screen coordinates matching the sprite rendering logic
    // screenX = (transform->x - camera.x) * 32 + winW/2 - 16;
    // The -16 centers the 32x32 sprite.
    // For the cone origin, we want the center of the sprite.
    // centerScreenX = ((transform->x - camera.x) * 32) + winW/2 - 16 + 16 =
    // ((transform->x - camera.x) * 32) + winW/2 Wait, transform->x is float
    // tile coords. Let's use the exact center calculation.

    float screenX = (transform->x - camera.x) * 32.0f + winW / 2.0f;
    float screenY = (transform->y - camera.y) * 32.0f + winH / 2.0f;

    float radDir = ai.facingDir * (M_PI / 180.0f);
    float radHalfCone = (ai.coneAngle / 2.0f) * (M_PI / 180.0f);
    float rangePixels = ai.sightRange * 32.0f;

    // We can approximate the cone with a triangle fan for better look, but a
    // simple triangle is a start. Let's do a fan of 3 triangles for a smoother
    // arc.

    int segments = 5;
    float angleStep = (ai.coneAngle * (M_PI / 180.0f)) / segments;
    float currentAngle = radDir - radHalfCone;

    std::vector<SDL_Vertex> vertices;
    // Center vertex
    SDL_Vertex center = {{screenX, screenY}, {255, 0, 0, 100}, {0, 0}};

    for (int i = 0; i <= segments; ++i) {
      float x = screenX + std::cos(currentAngle) * rangePixels;
      float y = screenY + std::sin(currentAngle) * rangePixels;

      vertices.push_back({{x, y}, {255, 0, 0, 100}, {0, 0}});
      currentAngle += angleStep;
    }

    // Draw triangles
    for (size_t i = 0; i < vertices.size() - 1; ++i) {
      SDL_Vertex tri[3];
      tri[0] = center;
      tri[1] = vertices[i];
      tri[2] = vertices[i + 1];
      SDL_RenderGeometry(renderer, NULL, tri, 3, NULL, 0);
    }
  }
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void PixelsGateGame::RenderGameOver() {
  SDL_Renderer *renderer = GetRenderer();
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);

  // Dark Red Overlay
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 50, 0, 0, 200);
  SDL_Rect overlay = {0, 0, w, h};
  SDL_RenderFillRect(renderer, &overlay);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  m_TextRenderer->RenderTextCentered("YOU DIED", w / 2, h / 3,
                                     {255, 0, 0, 255});

  std::string options[] = {"Load Last Save", "Quit to Main Menu"};
  int y = h / 2;
  for (int i = 0; i < 2; ++i) {
    SDL_Color color = (m_MenuSelection == i) ? SDL_Color{255, 255, 0, 255}
                                             : SDL_Color{255, 255, 255, 255};
    m_TextRenderer->RenderTextCentered(options[i], w / 2, y, color);
    if (i == m_MenuSelection) {
      m_TextRenderer->RenderTextCentered(">", w / 2 - 120, y, color);
      m_TextRenderer->RenderTextCentered("<", w / 2 + 120, y, color);
    }
    y += 50;
  }
}

void PixelsGateGame::HandleGameOverInput() {
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  int hovered = -1;
  int y = h / 2;
  for (int i = 0; i < 2; ++i) {
    SDL_Rect itemRect = {(w / 2) - 100, y - 15, 200, 30};
    if (mx >= itemRect.x && mx <= itemRect.x + itemRect.w && my >= itemRect.y &&
        my <= itemRect.y + itemRect.h) {
      hovered = i;
    }
    y += 50;
  }

  HandleMenuNavigation(
      2,
      [&](int selection) {
        if (selection == 0) { // Load Last Save
          if (std::filesystem::exists("quicksave.dat")) {
            PixelsEngine::SaveSystem::LoadWorldFlags("quicksave.dat",
                                                     m_WorldFlags);
            TriggerLoadTransition("quicksave.dat");
          } else if (std::filesystem::exists("savegame.dat")) {
            PixelsEngine::SaveSystem::LoadWorldFlags("savegame.dat",
                                                     m_WorldFlags);
            TriggerLoadTransition("savegame.dat");
          } else {
            // No save found, just restart to main menu
            m_State = GameState::MainMenu;
          }
        } else { // Quit to Main Menu
          m_State = GameState::MainMenu;
        }
      },
      nullptr, hovered);
}

void PixelsGateGame::RenderCharacterMenu() {
  SDL_Renderer *renderer = GetRenderer();
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  // Overlay
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 20, 20, 30, 230);
  SDL_Rect overlay = {0, 0, w, h};
  SDL_RenderFillRect(renderer, &overlay);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  // Main Panel
  int panelW = 800, panelH = 500;
  SDL_Rect panel = {(w - panelW) / 2, (h - panelH) / 2, panelW, panelH};
  SDL_SetRenderDrawColor(renderer, 45, 45, 55, 255);
  SDL_RenderFillRect(renderer, &panel);
  SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
  SDL_RenderDrawRect(renderer, &panel);

  // --- Tab Headers ---
  const char *tabNames[] = {"INVENTORY", "CHARACTER", "SPELLBOOK"};
  int tabW = panelW / 3;
  for (int i = 0; i < 3; ++i) {
    SDL_Rect tabRect = {panel.x + i * tabW, panel.y - 40, tabW, 40};
    bool hover = (mx >= tabRect.x && mx <= tabRect.x + tabRect.w &&
                  my >= tabRect.y && my <= tabRect.y + tabRect.h);

    if (m_CharacterTab == i)
      SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
    else if (hover)
      SDL_SetRenderDrawColor(renderer, 50, 50, 70, 255);
    else
      SDL_SetRenderDrawColor(renderer, 35, 35, 45, 255);

    SDL_RenderFillRect(renderer, &tabRect);
    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
    SDL_RenderDrawRect(renderer, &tabRect);

    m_TextRenderer->RenderTextCentered(tabNames[i], tabRect.x + tabW / 2,
                                       tabRect.y + 20, {255, 255, 255, 255});
  }

  // --- Content ---
  if (m_CharacterTab == 0) { // Inventory
    // ... (existing inventory render)
    auto *inv =
        GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
    if (inv) {
      // Equipment Slots (Left)
      int slotSize = 64;
      int startX = panel.x + 50;
      int startY = panel.y + 50;

      auto DrawBigSlot = [&](const std::string &label, PixelsEngine::Item &item,
                             int x, int y) {
        SDL_Rect r = {x, y, slotSize, slotSize};
        SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, 150, 150, 180, 255);
        SDL_RenderDrawRect(renderer, &r);
        m_TextRenderer->RenderTextCentered(label, x + slotSize / 2, y - 15,
                                           {200, 200, 200, 255});
        if (!item.IsEmpty()) {
          auto tex = PixelsEngine::TextureManager::LoadTexture(renderer,
                                                               item.iconPath);
          if (tex)
            tex->Render(x + 8, y + 8, 48, 48);
        }
      };

      DrawBigSlot("Melee", inv->equippedMelee, startX, startY);
      DrawBigSlot("Ranged", inv->equippedRanged, startX, startY + 100);
      DrawBigSlot("Armor", inv->equippedArmor, startX, startY + 200);

      // Item List (Right)
      int listX = panel.x + 200;
      int itemY = panel.y + 50;
      for (int i = 0; i < (int)inv->items.size(); ++i) {
        auto &item = inv->items[i];
        SDL_Rect row = {listX, itemY, panelW - 250, 40};
        bool hov = (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
                    my <= row.y + row.h);
        if (hov) {
          SDL_SetRenderDrawColor(renderer, 60, 60, 90, 255);
          SDL_RenderFillRect(renderer, &row);
        }
        RenderInventoryItem(item, listX + 5, itemY + 4);
        itemY += 45;
      }
    }
  } else if (m_CharacterTab == 1) { // Character Sheet
    auto *stats =
        GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
    if (stats) {
      int lx = panel.x + 50, rx = panel.x + 400, ty = panel.y + 50;
      m_TextRenderer->RenderText("Level " + std::to_string(stats->level) + " " +
                                     stats->race + " " + stats->characterClass,
                                 lx, ty, {255, 255, 0, 255});
      ty += 60;

      auto DrawStat = [&](const std::string &name, int val, int x, int y,
                          SDL_Color col) {
        m_TextRenderer->RenderText(name + ": " + std::to_string(val), x, y,
                                   col);
        int mod = (val - 10) / 2;
        std::string modStr = (mod >= 0 ? "+" : "") + std::to_string(mod);
        m_TextRenderer->RenderText("(" + modStr + ")", x + 150, y,
                                   {150, 150, 150, 255});
      };

      DrawStat("STR", stats->strength, lx, ty, {255, 150, 150, 255});
      ty += 40;
      DrawStat("DEX", stats->dexterity, lx, ty, {150, 255, 150, 255});
      ty += 40;
      DrawStat("CON", stats->constitution, lx, ty, {150, 150, 255, 255});
      ty += 40;
      DrawStat("INT", stats->intelligence, lx, ty, {255, 150, 255, 255});
      ty += 40;
      DrawStat("WIS", stats->wisdom, lx, ty, {150, 255, 255, 255});
      ty += 40;
      DrawStat("CHA", stats->charisma, lx, ty, {255, 255, 150, 255});

      ty = panel.y + 110;
      m_TextRenderer->RenderText(
          "Health: " + std::to_string(stats->currentHealth) + " / " +
              std::to_string(stats->maxHealth),
          rx, ty, {255, 255, 255, 255});
      ty += 40;
      m_TextRenderer->RenderText("Experience: " +
                                     std::to_string(stats->experience),
                                 rx, ty, {200, 200, 200, 255});
      ty += 40;
            m_TextRenderer->RenderText("Short Rests: " + std::to_string(stats->shortRestsAvailable), rx, ty, {200, 200, 200, 255});
        }
    }
    else if (m_CharacterTab == 2) { // Spellbook
        m_TextRenderer->RenderTextCentered("Known Spells", panel.x + panelW / 2,
                                           panel.y + 50, {200, 100, 255, 255});
        std::string spellNames[] = {"Fireball", "Heal", "Magic Missile", "Shield"};
        std::string spellDescs[] = {"Deals fire damage in a 6m radius.",
                                    "Restores HP to a creature.",
                                    "Three darts of force that never miss.",
                                    "Reaction to gain +5 AC until next turn."};
        std::string spellTooltips[] = {"Fir", "Hel", "Mis", "Shd"};
        int ty = panel.y + 100;
        for (int i = 0; i < 4; ++i) {
            SDL_Rect row = {panel.x + 50, ty, panelW - 100, 40};
            bool hov = (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
                        my <= row.y + row.h);
            if (hov) {
                SDL_SetRenderDrawColor(renderer, 80, 40, 110, 255); // Prominent purple highlight
                SDL_RenderFillRect(renderer, &row);
                SDL_SetRenderDrawColor(renderer, 200, 100, 255, 255);
                SDL_RenderDrawRect(renderer, &row);
                m_HoveredItemName = spellTooltips[i];
            }

            m_TextRenderer->RenderText(spellNames[i], panel.x + 60, ty + 10,
                                       {255, 255, 255, 255});
            m_TextRenderer->RenderText(spellDescs[i], panel.x + 250, ty + 10,
                                       {180, 180, 180, 255});
            ty += 50;
        }
    }

  m_TextRenderer->RenderTextCentered(
      "Press ESC or Tab Key to Close", panel.x + panelW / 2,
      panel.y + panelH - 30, {150, 150, 150, 255});
}

void PixelsGateGame::HandleCharacterMenuInput() {
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);
  int panelW = 800, panelH = 500;
  SDL_Rect panel = {(w - panelW) / 2, (h - panelH) / 2, panelW, panelH};

  if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
    int tabW = panelW / 3;
    for (int i = 0; i < 3; ++i) {
      SDL_Rect tabRect = {panel.x + i * tabW, panel.y - 40, tabW, 40};
      if (mx >= tabRect.x && mx <= tabRect.x + tabRect.w && my >= tabRect.y &&
          my <= tabRect.y + tabRect.h) {
        m_CharacterTab = i;
        m_DraggingItemIndex = -1;
        return;
      }
    }

    // Spell casting logic for Spellbook tab
    if (m_CharacterTab == 2) {
      int ty = panel.y + 100;
      std::string spellNames[] = {"Fireball", "Heal", "Magic Missile",
                                  "Shield"};
      for (int i = 0; i < 4; ++i) {
        SDL_Rect row = {panel.x + 50, ty, panelW - 100, 40};
        if (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
            my <= row.y + row.h) {
          m_PendingSpellName = spellNames[i];
          // m_ReturnState = GameState::Playing; // Removed to preserve context (Combat/Playing)
          m_State = GameState::Targeting;
          return;
        }
        ty += 50;
      }
    }
  }

  if (m_CharacterTab == 0) {
    HandleInventoryInput();
  }

  auto escKey =
      PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Pause);
  auto invKey =
      PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Inventory);
  auto chrKey =
      PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Character);
  auto magKey =
      PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Magic);

  if (PixelsEngine::Input::IsKeyPressed(escKey))
    m_State = m_ReturnState;
  if (PixelsEngine::Input::IsKeyPressed(invKey)) {
    if (m_CharacterTab == 0)
      m_State = m_ReturnState;
    else
      m_CharacterTab = 0;
  }
  if (PixelsEngine::Input::IsKeyPressed(chrKey)) {
    if (m_CharacterTab == 1)
      m_State = m_ReturnState;
    else
      m_CharacterTab = 1;
  }
  if (PixelsEngine::Input::IsKeyPressed(magKey)) {
    if (m_CharacterTab == 2)
      m_State = m_ReturnState;
    else
      m_CharacterTab = 2;
  }
}

void PixelsGateGame::RenderMapScreen() {
  SDL_Renderer *renderer = GetRenderer();
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);

  // Semi-transparent Overlay
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 20, 20, 20, 230);
  SDL_Rect screenRect = {0, 0, w, h};
  SDL_RenderFillRect(renderer, &screenRect);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  // --- Tabs ---
  int tabW = 150;
  int tabH = 40;
  int startX = w / 2 - tabW;
  int startY = 20;

  SDL_Rect mapTabRect = {startX, startY, tabW, tabH};
  SDL_Rect journalTabRect = {startX + tabW, startY, tabW, tabH};

  // Draw Map Tab
  SDL_SetRenderDrawColor(renderer, (m_MapTab == 0) ? 100 : 50,
                         (m_MapTab == 0) ? 100 : 50, (m_MapTab == 0) ? 150 : 50,
                         255);
  SDL_RenderFillRect(renderer, &mapTabRect);
  SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
  SDL_RenderDrawRect(renderer, &mapTabRect);
  m_TextRenderer->RenderTextCentered("MAP (M)", mapTabRect.x + tabW / 2,
                                     mapTabRect.y + 10, {255, 255, 255, 255});

  // Draw Journal Tab
  SDL_SetRenderDrawColor(renderer, (m_MapTab == 1) ? 100 : 50,
                         (m_MapTab == 1) ? 100 : 50, (m_MapTab == 1) ? 150 : 50,
                         255);
  SDL_RenderFillRect(renderer, &journalTabRect);
  SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
  SDL_RenderDrawRect(renderer, &journalTabRect);
  m_TextRenderer->RenderTextCentered("JOURNAL (J)", journalTabRect.x + tabW / 2,
                                     journalTabRect.y + 10,
                                     {255, 255, 255, 255});

  if (m_MapTab == 0) {
    // --- Render Map ---
    if (m_Level) {
      int mapW = m_Level->GetWidth();
      int mapH = m_Level->GetHeight();
      int tileSize = 10;
      int miniW = mapW * tileSize;
      int miniH = mapH * tileSize;
      int mx = (w - miniW) / 2;
      int my = (h - miniH) / 2 + 40; // Offset for tabs

      SDL_Rect miniRect = {mx - 5, my - 5, miniW + 10, miniH + 10};
      SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
      SDL_RenderFillRect(renderer, &miniRect);

      for (int ty = 0; ty < mapH; ++ty) {
        for (int tx = 0; tx < mapW; ++tx) {
          if (!m_Level->IsExplored(tx, ty))
            continue;
          using namespace PixelsEngine::Tiles;
          int tileIdx = m_Level->GetTile(tx, ty);
          SDL_Color c = {0, 0, 0, 255};

          if (tileIdx >= DIRT && tileIdx <= DIRT_VARIANT_18)
            c = {101, 67, 33, 255};
          else if (tileIdx >= DIRT_WITH_LEAVES_01 &&
                   tileIdx <= DIRT_WITH_LEAVES_02)
            c = {85, 107, 47, 255};
          else if (tileIdx >= GRASS && tileIdx <= GRASS_VARIANT_02)
            c = {34, 139, 34, 255};
          else if (tileIdx == DIRT_VARIANT_19 ||
                   tileIdx == DIRT_WITH_PARTIAL_GRASS)
            c = {139, 69, 19, 255};
          else if (tileIdx >= GRASS_BLOCK_FULL &&
                   tileIdx <= GRASS_BLOCK_FULL_VARIANT_01)
            c = {0, 128, 0, 255};
          else if (tileIdx >= GRASS_WITH_BUSH &&
                   tileIdx <= GRASS_WITH_BUSH_VARIANT_07)
            c = {0, 100, 0, 255};
          else if (tileIdx >= GRASS_VARIANT_03 && tileIdx <= GRASS_VARIANT_06)
            c = {50, 205, 50, 255};
          else if (tileIdx >= FLOWER && tileIdx <= FLOWERS_WITHOUT_LEAVES)
            c = {255, 105, 180, 255};
          else if (tileIdx >= LOG && tileIdx <= LOG_WITH_LEAVES_VARIANT_02)
            c = {139, 69, 19, 255};
          else if (tileIdx >= DIRT_PILE && tileIdx <= DIRT_PILE_VARIANT_07)
            c = {160, 82, 45, 255};
          else if (tileIdx >= COBBLESTONE && tileIdx <= SMOOTH_STONE)
            c = {128, 128, 128, 255};
          else if (tileIdx >= ROCK && tileIdx <= ROCK_VARIANT_03)
            c = {105, 105, 105, 255};
          else if (tileIdx >= ROCK_ON_WATER &&
                   tileIdx <= STONES_ON_WATER_VARIANT_11)
            c = {70, 130, 180, 255};
          else if (tileIdx >= WATER_DROPLETS && tileIdx <= OCEAN_ROUGH)
            c = {0, 0, 255, 255};
          else
            c = {100, 100, 100, 255};

          SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
          SDL_Rect tileRect = {mx + tx * tileSize, my + ty * tileSize, tileSize,
                               tileSize};
          SDL_RenderFillRect(renderer, &tileRect);
        }
      }

      auto FillCircleLarge = [&](int cx, int cy, int r, SDL_Color color) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        for (int dy = -r; dy <= r; dy++) {
          for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r)
              SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
          }
        }
      };

      auto &transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
      for (auto &[entity, trans] : transforms) {
        if (!m_Level->IsExplored((int)trans.x, (int)trans.y))
          continue;

        int px = mx + (int)trans.x * tileSize + tileSize / 2;
        int py = my + (int)trans.y * tileSize + tileSize / 2;

        if (entity == m_Player) {
          FillCircleLarge(px, py, 6, {255, 255, 255, 255});
          continue;
        }

        auto *tagComp =
            GetRegistry().GetComponent<PixelsEngine::TagComponent>(entity);
        if (tagComp) {
          switch (tagComp->tag) {
          case PixelsEngine::EntityTag::Hostile:
            FillCircleLarge(px, py, 4, {255, 0, 0, 255});
            break;
          case PixelsEngine::EntityTag::NPC:
            FillCircleLarge(px, py, 4, {255, 215, 0, 255});
            break;
          case PixelsEngine::EntityTag::Companion:
            FillCircleLarge(px, py, 4, {0, 191, 255, 255});
            break;
          case PixelsEngine::EntityTag::Trader: {
            SDL_Rect box = {px - 4, py - 4, 8, 8};
            SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
            SDL_RenderFillRect(renderer, &box);
            break;
          }
          case PixelsEngine::EntityTag::Quest: {
            FillCircleLarge(px, py, 6, {255, 255, 255, 255});
            SDL_Rect sq = {px - 2, py - 2, 4, 4};
            SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
            SDL_RenderFillRect(renderer, &sq);
            break;
          }
          default:
            break;
          }
        }
      }
    }
  } else {
    // --- Render Journal ---
    m_TextRenderer->RenderTextCentered("QUEST JOURNAL", w / 2, 100,
                                       {255, 215, 0, 255});

    auto &quests = GetRegistry().View<PixelsEngine::QuestComponent>();
    int qY = 160;
    bool anyQuests = false;

    for (auto &[entity, quest] : quests) {
      if (quest.state == 0)
        continue; // Not started
      anyQuests = true;

      std::string title = quest.questId;
      if (title == "FetchOrb")
        title = "The Lost Orb";
      else if (title == "HuntBoars")
        title = "Boar Hunt";

      std::string status = (quest.state == 2) ? " (Complete)" : "";
      SDL_Color color = (quest.state == 2) ? SDL_Color{150, 150, 150, 255}
                                           : SDL_Color{255, 255, 255, 255};

      // Draw bullet point
      m_TextRenderer->RenderText("- " + title + status, w / 2 - 150, qY, color);

      // Draw simple description based on ID/State
      std::string desc = "";
      if (quest.state == 1) {
        if (quest.questId == "FetchOrb")
          desc = "Find the Gold Orb for the Innkeeper.";
        else if (quest.questId == "HuntBoars")
          desc = "Hunt Boars and bring Boar Meat to the Guardian.";
      } else {
        if (quest.questId == "FetchOrb")
          desc = "You returned the Gold Orb.";
        else if (quest.questId == "HuntBoars")
          desc = "You helped reduce the boar population.";
      }

      if (!desc.empty()) {
        m_TextRenderer->RenderTextWrapped(desc, w / 2 - 130, qY + 25, 300,
                                          {180, 180, 180, 255});
        qY += 60; // Extra spacing for description
      } else {
        qY += 40;
      }
    }

    if (!anyQuests) {
      m_TextRenderer->RenderTextCentered("No active quests.", w / 2, h / 2,
                                         {150, 150, 150, 255});
    }
  }

  m_TextRenderer->RenderTextCentered("Press ESC or MAP/J to Close", w / 2,
                                     h - 50, {150, 150, 150, 255});
}

void PixelsGateGame::HandleMapInput() {
  auto escKey =
      PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Pause);
  auto mapKey = PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Map);

  if (PixelsEngine::Input::IsKeyPressed(escKey)) {
    m_State = m_ReturnState;
    return;
  }

  // M and J keys handled in OnUpdate mostly, but good to have toggle here for
  // M/J within menu
  if (PixelsEngine::Input::IsKeyPressed(mapKey)) {
    if (m_MapTab == 0)
      m_State = m_ReturnState;
    else
      m_MapTab = 0;
  }
  if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_J)) {
    if (m_MapTab == 1)
      m_State = m_ReturnState;
    else
      m_MapTab = 1;
  }

  // Mouse Interaction for Tabs
  if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
    int w, h;
    SDL_GetWindowSize(m_Window, &w, &h);
    int mx, my;
    PixelsEngine::Input::GetMousePosition(mx, my);

    int tabW = 150;
    int tabH = 40;
    int startX = w / 2 - tabW;
    int startY = 20;

    SDL_Rect mapTabRect = {startX, startY, tabW, tabH};
    SDL_Rect journalTabRect = {startX + tabW, startY, tabW, tabH};

    if (mx >= mapTabRect.x && mx <= mapTabRect.x + mapTabRect.w &&
        my >= mapTabRect.y && my <= mapTabRect.y + mapTabRect.h) {
      m_MapTab = 0;
    } else if (mx >= journalTabRect.x &&
               mx <= journalTabRect.x + journalTabRect.w &&
               my >= journalTabRect.y &&
               my <= journalTabRect.y + journalTabRect.h) {
      m_MapTab = 1;
    }
  }
}

void PixelsGateGame::RenderRestMenu() {
  SDL_Renderer *renderer = GetRenderer();
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);

  // Overlay
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
  SDL_Rect overlay = {0, 0, w, h};
  SDL_RenderFillRect(renderer, &overlay);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  int boxW = 400, boxH = 250;
  SDL_Rect box = {(w - boxW) / 2, (h - boxH) / 2, boxW, boxH};
  SDL_SetRenderDrawColor(renderer, 40, 30, 20, 255);
  SDL_RenderFillRect(renderer, &box);
  SDL_SetRenderDrawColor(renderer, 200, 150, 100, 255);
  SDL_RenderDrawRect(renderer, &box);

  m_TextRenderer->RenderTextCentered("REST MENU", box.x + boxW / 2, box.y + 30,
                                     {255, 215, 0, 255});

  auto *stats =
      GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
  std::string shortRestStr =
      "Short Rest (" + std::to_string(stats ? stats->shortRestsAvailable : 0) +
      " left)";

  // Check Supplies for Long Rest
  int supplies = 0;
  auto *inv =
      GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
  if (inv) {
    for (const auto &item : inv->items) {
      if (item.name == "Bread")
        supplies += item.quantity * 10;
      else if (item.name == "Boar Meat")
        supplies += item.quantity * 20;
    }
  }
  std::string longRestStr =
      "Long Rest (Supplies: " + std::to_string(supplies) + "/40)";
  std::string campStr =
      (m_ReturnState == GameState::Camp) ? "Leave Camp" : "Go To Camp";

  std::string options[] = {shortRestStr, longRestStr, campStr, "Back"};
  int y = box.y + 80;
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  for (int i = 0; i < 4; ++i) {
    SDL_Rect row = {box.x + 50, y - 15, boxW - 100, 30};
    bool hover = (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
                  my <= row.y + row.h);
    if (hover)
      m_MenuSelection = i;

    SDL_Color color = (m_MenuSelection == i) ? SDL_Color{255, 255, 0, 255}
                                             : SDL_Color{255, 255, 255, 255};
    m_TextRenderer->RenderTextCentered(options[i], box.x + boxW / 2, y, color);
    if (i == m_MenuSelection) {
      m_TextRenderer->RenderTextCentered(">", box.x + boxW / 2 - 150, y, color);
      m_TextRenderer->RenderTextCentered("<", box.x + boxW / 2 + 150, y, color);
    }
    y += 40;
  }
}

void PixelsGateGame::HandleRestMenuInput() {
  int winW, winH;
  SDL_GetWindowSize(m_Window, &winW, &winH);
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);
  int boxW = 400, boxH = 250;
  SDL_Rect box = {(winW - boxW) / 2, (winH - boxH) / 2, boxW, boxH};

  int hovered = -1;
  int optY = box.y + 80;
  for (int i = 0; i < 4; ++i) {
    SDL_Rect row = {box.x + 50, optY - 15, boxW - 100, 30};
    if (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
        my <= row.y + row.h) {
      hovered = i;
    }
    optY += 40;
  }

  HandleMenuNavigation(
      4,
      [&](int selection) {
        auto *stats =
            GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
        auto *trans =
            GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
                m_Player);
        auto *inv =
            GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
                m_Player);

        if (selection == 0) { // Short Rest
          if (stats && stats->shortRestsAvailable > 0) {
            stats->shortRestsAvailable--;
            int heal = stats->maxHealth / 2;
            stats->currentHealth =
                std::min(stats->maxHealth, stats->currentHealth + heal);
            SpawnFloatingText(trans->x, trans->y,
                              "+" + std::to_string(heal) + " (Short Rest)",
                              {0, 255, 0, 255});
            m_State = m_ReturnState;
          } else {
            SpawnFloatingText(trans->x, trans->y, "No Short Rests Left!",
                              {255, 0, 0, 255});
          }
        } else if (selection == 1) { // Long Rest
          int supplies = 0;
          if (inv) {
            for (auto &item : inv->items) {
              if (item.name == "Bread")
                supplies += item.quantity * 10;
              else if (item.name == "Boar Meat")
                supplies += item.quantity * 20;
            }
          }

          if (supplies >= 40) {
            // Consume supplies
            int needed = 40;
            if (inv) {
              auto it = inv->items.begin();
              while (it != inv->items.end() && needed > 0) {
                int val = (it->name == "Bread")
                              ? 10
                              : ((it->name == "Boar Meat") ? 20 : 0);
                if (val > 0) {
                  int countToTake =
                      std::min(it->quantity, (needed + val - 1) / val);
                  it->quantity -= countToTake;
                  needed -= countToTake * val;
                  if (it->quantity <= 0)
                    it = inv->items.erase(it);
                  else
                    ++it;
                } else
                  ++it;
              }
            }
            // Full Rest Benefits
            if (stats) {
              stats->currentHealth = stats->maxHealth;
              stats->shortRestsAvailable = stats->maxShortRests;
              stats->inspiration = 1;
            }
            m_Time.m_TimeOfDay = 8.0f; // New Day
            SpawnFloatingText(trans->x, trans->y, "Long Rest Complete!",
                              {255, 255, 0, 255});
            m_State =
                GameState::Playing; // Always return to playing after long rest
          } else {
            SpawnFloatingText(trans->x, trans->y, "Need 40 Supplies!",
                              {255, 0, 0, 255});
          }
        } else if (selection == 2) { // Go To Camp / Leave Camp
          if (m_ReturnState == GameState::Camp) {
            // Leave Camp
            if (trans) {
              trans->x = m_LastWorldPos.x;
              trans->y = m_LastWorldPos.y;
            }
            m_State = GameState::Playing;
          } else {
            // Go To Camp
            if (trans) {
              m_LastWorldPos.x = trans->x;
              m_LastWorldPos.y = trans->y;
              // Teleport to center of separate camp map
              trans->x = 7.0f;
              trans->y = 7.0f;
            }
            m_State = GameState::Camp;
          }
        } else {
          m_State = m_ReturnState;
        }
      },
      nullptr, hovered);
}

void PixelsGateGame::RenderDialogueScreen() {
  SDL_Renderer *renderer = GetRenderer();
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  auto *diag = GetRegistry().GetComponent<PixelsEngine::DialogueComponent>(
      m_DialogueWith);
  if (!diag)
    return;

  // Overlay
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
  SDL_Rect overlay = {0, 0, w, h};
  SDL_RenderFillRect(renderer, &overlay);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  // Dialogue Panel
  int panelW = 600, panelH = 250;
  SDL_Rect panel = {(w - panelW) / 2, h - panelH - 100, panelW, panelH};
  SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
  SDL_RenderFillRect(renderer, &panel);
  SDL_SetRenderDrawColor(renderer, 150, 150, 160, 255);
  SDL_RenderDrawRect(renderer, &panel);

  // NPC Name & Text
  m_TextRenderer->RenderText(diag->tree->currentEntityName, panel.x + 20,
                             panel.y + 20, {255, 215, 0, 255});
  int npcTextHeight = m_TextRenderer->RenderTextWrapped(
      diag->tree->nodes[diag->tree->currentNodeId].npcText, panel.x + 20,
      panel.y + 50, panelW - 40, {255, 255, 255, 255});

  // Options
  auto &allOptions = diag->tree->nodes[diag->tree->currentNodeId].options;
  auto *inv =
      GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);

  std::vector<int> visibleOptionIndices;
  for (int i = 0; i < (int)allOptions.size(); ++i) {
    auto &opt = allOptions[i];
    // Check Exclude Flag (Hide if true)
    if (!opt.excludeFlag.empty() && m_WorldFlags[opt.excludeFlag])
      continue;

    // Check Required Flag (Hide if false/missing)
    if (!opt.requiredFlag.empty() && !m_WorldFlags[opt.requiredFlag])
      continue;

    // Check Required Item (Hide if missing)
    if (!opt.requiredItem.empty()) {
      bool hasItem = false;
      if (inv) {
        for (auto &it : inv->items) {
          if (it.name == opt.requiredItem && it.quantity > 0)
            hasItem = true;
        }
      }
      if (!hasItem)
        continue;
    }

    if (!opt.repeatable && opt.hasBeenChosen)
      continue;
    visibleOptionIndices.push_back(i);
  }

  int optY = panel.y + 50 + npcTextHeight + 20;
  for (int i = 0; i < (int)visibleOptionIndices.size(); ++i) {
    int originalIndex = visibleOptionIndices[i];

    std::string prefix = std::to_string(i + 1) + ". ";
    std::string fullText = prefix + allOptions[originalIndex].text;

    // We need the height for the hover box.
    // A simple way is to use a fixed max width and guess or pre-calculate.
    // For now, let's use a standard height but wrap the text.
    // Most options are short.

    SDL_Rect row = {panel.x + 20, optY - 5, panelW - 40,
                    30}; // Default height 30

    // If we want truly dynamic hover boxes, we'd need a way to query text
    // height without rendering. SDL_ttf has TTF_SizeText for this.

    bool hover = (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
                  my <= row.y + row.h);
    SDL_Color color = (m_DialogueSelection == i)
                          ? SDL_Color{50, 255, 50, 255}
                          : SDL_Color{200, 200, 200, 255};

    if (hover) {
      color = {255, 255, 0, 255};
      m_DialogueSelection = i;
    }

    int optHeight = m_TextRenderer->RenderTextWrapped(fullText, panel.x + 30,
                                                      optY, panelW - 60, color);
    optY += std::max(30, optHeight) + 10;
  }
}

void PixelsGateGame::HandleDialogueInput() {
  auto *diag = GetRegistry().GetComponent<PixelsEngine::DialogueComponent>(
      m_DialogueWith);
  if (!diag || !diag->tree) {
    m_State = m_ReturnState;
    return;
  }

  auto &node = diag->tree->nodes[diag->tree->currentNodeId];
  auto *inv =
      GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);

  std::vector<int> visibleOptionIndices;
  for (int i = 0; i < (int)node.options.size(); ++i) {
    auto &opt = node.options[i];
    if (!opt.excludeFlag.empty() && m_WorldFlags[opt.excludeFlag])
      continue;
    if (!opt.requiredFlag.empty() && !m_WorldFlags[opt.requiredFlag])
      continue;
    if (!opt.requiredItem.empty()) {
      bool hasItem = false;
      if (inv) {
        for (auto &it : inv->items) {
          if (it.name == opt.requiredItem && it.quantity > 0)
            hasItem = true;
        }
      }
      if (!hasItem)
        continue;
    }
    if (!opt.repeatable && opt.hasBeenChosen)
      continue;
    visibleOptionIndices.push_back(i);
  }
  int numOptions = visibleOptionIndices.size();

  bool isClick = PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT);
  bool isEnter = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_RETURN) ||
                 PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_SPACE);

  if (isClick || isEnter) {
    if (m_DialogueSelection >= 0 && m_DialogueSelection < numOptions) {
      int originalIndex = visibleOptionIndices[m_DialogueSelection];
      auto &opt = node.options[originalIndex];

      // Mark as chosen
      opt.hasBeenChosen = true;

      // 1. Handle Dice Roll if needed
      if (opt.requiredStat != "None") {
        auto *pStats =
            GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);
        int mod = 0;
        if (pStats) {
          if (opt.requiredStat == "Intelligence")
            mod = pStats->GetModifier(pStats->intelligence);
          else if (opt.requiredStat == "Charisma")
            mod = pStats->GetModifier(pStats->charisma);
        }
        StartDiceRoll(mod, opt.dc, opt.requiredStat, m_DialogueWith,
                      PixelsEngine::ContextActionType::Talk);
        m_State = GameState::Playing;
        return;
      }

      // 2. Handle Actions
      if (opt.action == PixelsEngine::DialogueAction::StartCombat) {
        m_State = m_ReturnState;
        StartCombat(m_DialogueWith);
        auto *ai = GetRegistry().GetComponent<PixelsEngine::AIComponent>(
            m_DialogueWith);
        if (ai)
          ai->isAggressive = true;
        return;
      } else if (opt.action == PixelsEngine::DialogueAction::SetFlag) {
        m_WorldFlags[opt.actionParam] = true;
      } else if (opt.action == PixelsEngine::DialogueAction::StartQuest) {
        auto *q = GetRegistry().GetComponent<PixelsEngine::QuestComponent>(
            m_DialogueWith);
        if (q)
          q->state = 1;
        m_WorldFlags[opt.actionParam] =
            true; // Use param as flag name for "Active"
      } else if (opt.action == PixelsEngine::DialogueAction::CompleteQuest) {
        auto *q = GetRegistry().GetComponent<PixelsEngine::QuestComponent>(
            m_DialogueWith);
        if (q && q->state == 1) {
          q->state = 2;
          m_WorldFlags[opt.actionParam] = true; // Flag "Completed"
          // Remove Item
          if (inv) {
            auto it = inv->items.begin();
            while (it != inv->items.end()) {
              if (it->name == q->targetItem) {
                it->quantity--;
                if (it->quantity <= 0)
                  it = inv->items.erase(it);
                break;
              } else
                ++it;
            }
            // Rewards
            int rewardGold = 50;
            if (q->questId == "FetchOrb")
              rewardGold = 500;
            if (q->questId == "HuntBoars")
              rewardGold = 100;
            inv->AddItem("Coins", rewardGold);

            auto *pTrans =
                GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
                    m_Player);
            float px = pTrans ? pTrans->x : 0.0f;
            float py = pTrans ? pTrans->y : 0.0f;

            SpawnFloatingText(px, py - 0.5f,
                              "+" + std::to_string(rewardGold) + " Gold",
                              {255, 215, 0, 255});

            auto *pStats =
                GetRegistry().GetComponent<PixelsEngine::StatsComponent>(
                    m_Player);
            if (pStats) {
              pStats->experience += 100;
              SpawnFloatingText(px, py - 1.5f, "+100 XP", {0, 255, 255, 255});
            }
          }
        }
      } else if (opt.action == PixelsEngine::DialogueAction::EndConversation) {
        m_State = m_ReturnState;
        return;
      } else if (opt.action ==
                 PixelsEngine::DialogueAction::GiveItem) { // Special logic for
                                                           // trade trigger
        m_TradingWith = m_DialogueWith;
        m_State = GameState::Trading;
        return;
      }

      // 3. Next Node
      if (opt.nextNodeId == "end") {
        m_State = m_ReturnState;
      } else {
        diag->tree->currentNodeId = opt.nextNodeId;
        m_DialogueSelection = 0;
      }
    }
  }

  if (numOptions > 0) {
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_W)) {
      m_DialogueSelection = (m_DialogueSelection - 1 + numOptions) % numOptions;
    }
    if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_S)) {
      m_DialogueSelection = (m_DialogueSelection + 1) % numOptions;
    }
  }
}

void PixelsGateGame::HandleTargetingInput() {

  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  bool isClick = PixelsEngine::Input::IsMouseButtonDown(SDL_BUTTON_LEFT);

  static bool wasClick = false;

  bool clicked = isClick && !wasClick;

  wasClick = isClick;

  if (clicked) {

    // Find clicked entity

    auto &camera = GetCamera();

    auto &transforms = GetRegistry().View<PixelsEngine::TransformComponent>();

    PixelsEngine::Entity target = PixelsEngine::INVALID_ENTITY;

    for (auto &[entity, transform] : transforms) {

      auto *sprite =
          GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);

      if (sprite) {

        int screenX, screenY;
        m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);

        screenX -= (int)camera.x;
        screenY -= (int)camera.y;

        SDL_Rect drawRect = {screenX + 16 - sprite->pivotX,
                             screenY + 16 - sprite->pivotY, sprite->srcRect.w,
                             sprite->srcRect.h};

        if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w &&
            my >= drawRect.y && my <= drawRect.y + drawRect.h) {

          target = entity;

          break;
        }
      }
    }

    if (target != PixelsEngine::INVALID_ENTITY ||
        m_PendingSpellName == "Shield") {

      CastSpell(m_PendingSpellName, target);

    } else {

      // Clicked empty ground

      SpawnFloatingText(0, 0, "Select a valid target!", {255, 200, 0, 255});
    }
  }

  if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
    m_State = m_ReturnState; // Dismiss spell targeting and return to game
  }
}

void PixelsGateGame::CastSpell(const std::string &spellName,
                               PixelsEngine::Entity target) {

  auto *pStats =
      GetRegistry().GetComponent<PixelsEngine::StatsComponent>(m_Player);

  auto *pTrans =
      GetRegistry().GetComponent<PixelsEngine::TransformComponent>(m_Player);

  if (!pStats || !pTrans)
    return;

  if (m_State == GameState::Combat || m_ReturnState == GameState::Combat) {

    if (m_Combat.m_CurrentTurnIndex >= 0 && m_Combat.m_CurrentTurnIndex < m_Combat.m_TurnOrder.size()) {

      if (!m_Combat.m_TurnOrder[m_Combat.m_CurrentTurnIndex].isPlayer) {

        SpawnFloatingText(pTrans->x, pTrans->y, "Not Your Turn!",
                          {255, 0, 0, 255});

        m_State = m_ReturnState;

        return;
      }
    }

    if (m_Combat.m_ActionsLeft <= 0) {

      SpawnFloatingText(pTrans->x, pTrans->y, "No Actions!", {255, 0, 0, 255});

      m_State = m_ReturnState;

      return;
    }
  }

  bool success = false;

  if (spellName == "Heal") {

    // Can heal self or anyone

    PixelsEngine::Entity healTarget =
        (target == PixelsEngine::INVALID_ENTITY) ? m_Player : target;

    auto *tStats =
        GetRegistry().GetComponent<PixelsEngine::StatsComponent>(healTarget);

    auto *tTrans = GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
        healTarget);

    if (tStats) {

      int healing =
          PixelsEngine::Dice::Roll(4) + PixelsEngine::Dice::Roll(4) + 2;

      tStats->currentHealth += healing;

      if (tStats->currentHealth > tStats->maxHealth)
        tStats->currentHealth = tStats->maxHealth;

      if (tTrans)
        SpawnFloatingText(tTrans->x, tTrans->y, "+" + std::to_string(healing),
                          {0, 255, 0, 255});

      success = true;
    }

  } else if (spellName == "Fireball" || spellName == "Magic Missile") {

    if (target != PixelsEngine::INVALID_ENTITY) {

      auto *tStats =
          GetRegistry().GetComponent<PixelsEngine::StatsComponent>(target);

      auto *tTrans =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(target);

      if (tStats) {

        if (spellName == "Magic Missile") {

          int dmg = PixelsEngine::Dice::Roll(4) + 1;

          tStats->currentHealth -= dmg;

          if (tTrans)
            SpawnFloatingText(tTrans->x, tTrans->y, std::to_string(dmg),
                              {200, 100, 255, 255});

          if (tStats->currentHealth <= 0) {

            tStats->currentHealth = 0;

            tStats->isDead = true;

            auto *inv =
                GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
                    target);

            auto *loot =
                GetRegistry().GetComponent<PixelsEngine::LootComponent>(target);

            if (!loot)
              loot = &GetRegistry().AddComponent(target,
                                                 PixelsEngine::LootComponent{});

            if (inv) {

              for (auto &item : inv->items)
                loot->drops.push_back(item);

              inv->items.clear();
            }
          }

          success = true;

        } else if (spellName == "Fireball") {

          // Spawn Fire Surface
          auto hazard = GetRegistry().CreateEntity();
          GetRegistry().AddComponent(hazard,
                                     PixelsEngine::TransformComponent{tTrans->x, tTrans->y});
          GetRegistry().AddComponent(hazard, PixelsEngine::HazardComponent{
                                                 PixelsEngine::HazardComponent::Type::Fire, 5, 10.0f});
          auto fireTex = PixelsEngine::TextureManager::LoadTexture(
              GetRenderer(), "assets/ui/spell_fireball.png");
          GetRegistry().AddComponent(
              hazard, PixelsEngine::SpriteComponent{fireTex, {0, 0, 32, 32}, 16, 16});

          // AoE around target
          auto &allStats = GetRegistry().View<PixelsEngine::StatsComponent>();

          for (auto &[ent, s] : allStats) {

            if (ent == m_Player && target != m_Player)
              continue; // Friendly fire? usually fireball hits everyone

            auto *tr =
                GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
                    ent);

            if (tr) {

              float d = std::sqrt(std::pow(tr->x - tTrans->x, 2) +
                                  std::pow(tr->y - tTrans->y, 2));

              if (d < 3.0f) {

                int dmg = PixelsEngine::Dice::Roll(6) +
                          PixelsEngine::Dice::Roll(6) +
                          PixelsEngine::Dice::Roll(6);

                s.currentHealth -= dmg;

                SpawnFloatingText(tr->x, tr->y, std::to_string(dmg) + "!",
                                  {255, 100, 0, 255});

                if (s.currentHealth <= 0) {

                  s.currentHealth = 0;
                  s.isDead = true;

                  // Drop Loot
                  std::vector<PixelsEngine::Item> itemsToDrop;
                  auto *inv =
                      GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(ent);
                  if (inv) {
                    itemsToDrop.insert(itemsToDrop.end(), inv->items.begin(),
                                       inv->items.end());
                    inv->items.clear();
                  }
                  auto *loot =
                      GetRegistry().GetComponent<PixelsEngine::LootComponent>(ent);
                  if (loot) {
                    itemsToDrop.insert(itemsToDrop.end(), loot->drops.begin(),
                                       loot->drops.end());
                    loot->drops.clear();
                  }

                  for (const auto &item : itemsToDrop) {
                    auto dropEnt = GetRegistry().CreateEntity();
                    float offX = (PixelsEngine::Dice::Roll(10) - 5) / 10.0f;
                    float offY = (PixelsEngine::Dice::Roll(10) - 5) / 10.0f;
                    GetRegistry().AddComponent(
                        dropEnt,
                        PixelsEngine::TransformComponent{tr->x + offX, tr->y + offY});

                    std::string icon =
                        item.iconPath.empty() ? "assets/box.png" : item.iconPath;
                    auto tex =
                        PixelsEngine::TextureManager::LoadTexture(GetRenderer(), icon);
                    GetRegistry().AddComponent(
                        dropEnt, PixelsEngine::SpriteComponent{tex, {0, 0, 32, 32}, 16, 16});

                    GetRegistry().AddComponent(dropEnt, PixelsEngine::InteractionComponent{
                                                            item.name, "item_drop", false, 0.0f});
                    GetRegistry().AddComponent(
                        dropEnt, PixelsEngine::StatsComponent{5, 5, 0, false});
                    GetRegistry().AddComponent(
                        dropEnt, PixelsEngine::LootComponent{
                                     std::vector<PixelsEngine::Item>{item}});
                  }
                }
              }
            }
          }

          success = true;
        }

        if (success &&
            (m_State == GameState::Playing ||
             m_ReturnState == GameState::Playing) &&
            target != m_Player) {

          auto *ai =
              GetRegistry().GetComponent<PixelsEngine::AIComponent>(target);
          if (ai) {
            StartCombat(target);
          } else {
            StartCombat(PixelsEngine::INVALID_ENTITY);
          }
        }
      }
    }

  } else if (spellName == "Shield") {

    SpawnFloatingText(pTrans->x, pTrans->y, "Shielded!", {100, 200, 255, 255});

    success = true;
  }

  if (success) {

    if (m_State == GameState::Combat || m_ReturnState == GameState::Combat)
      m_Combat.m_ActionsLeft--;

    // Final Victory Check if in combat

    if (m_State == GameState::Combat || m_ReturnState == GameState::Combat) {

      bool enemiesAlive = false;

      auto &view = GetRegistry().View<PixelsEngine::AIComponent>();

      for (auto &[ent, ai] : view) {

        auto *s = GetRegistry().GetComponent<PixelsEngine::StatsComponent>(ent);

        if (s && !s->isDead && IsInTurnOrder(ent))
          enemiesAlive = true;
      }

      if (!enemiesAlive) {

        EndCombat();

        m_ReturnState = GameState::Playing;
      }
    }

    if (m_State != GameState::Combat) {
      if (m_ReturnState == GameState::CharacterMenu)
        m_ReturnState = GameState::Playing;
      m_State = m_ReturnState;
    }

  } else {

    // Return to spellbook if no success (clicked nothing etc)

    m_State = GameState::CharacterMenu;
    m_CharacterTab = 2; // Spellbook
  }
}

void PixelsGateGame::RenderTradeScreen() {

  SDL_Renderer *renderer = GetRenderer();

  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);

  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  // Overlay

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  SDL_SetRenderDrawColor(renderer, 20, 40, 20, 230);

  SDL_Rect screenRect = {0, 0, w, h};

  SDL_RenderFillRect(renderer, &screenRect);

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  m_TextRenderer->RenderTextCentered("TRADING", w / 2, 50,
                                     {100, 255, 100, 255});

  auto *pInv =
      GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);

  auto *nInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
      m_TradingWith);

  if (pInv) {

    m_TextRenderer->RenderText("Your Items", w / 4 - 100, 100,
                               {255, 255, 255, 255});

    int y = 140;

    for (int i = 0; i < pInv->items.size(); ++i) {

      auto &item = pInv->items[i];

      if (item.name == "Coins")
        continue;

      SDL_Rect row = {w / 4 - 130, y - 5, 300, 30};

      bool hover = (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
                    my <= row.y + row.h);

      if (hover) {

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);

        SDL_RenderFillRect(renderer, &row);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
      }

      m_TextRenderer->RenderText(item.name + " (x" +
                                     std::to_string(item.quantity) + ")",
                                 row.x + 10, y, {200, 200, 200, 255});

      m_TextRenderer->RenderText("Sell: " + std::to_string(item.value) + "g",
                                 row.x + 210, y, {255, 215, 0, 255});

      y += 35;
    }

    // Show Player Gold

    int gold = 0;

    for (auto &it : pInv->items)
      if (it.name == "Coins")
        gold = it.quantity;

    m_TextRenderer->RenderText("Your Gold: " + std::to_string(gold) + "g",
                               w / 4 - 100, h - 100, {255, 255, 0, 255});
  }

  if (nInv) {

    m_TextRenderer->RenderText("Trader Items", 3 * w / 4 - 100, 100,
                               {255, 255, 255, 255});

    int y = 140;

    for (int i = 0; i < nInv->items.size(); ++i) {

      auto &item = nInv->items[i];

      if (item.name == "Coins")
        continue;

      SDL_Rect row = {3 * w / 4 - 130, y - 5, 300, 30};

      bool hover = (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
                    my <= row.y + row.h);

      if (hover) {

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);

        SDL_RenderFillRect(renderer, &row);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
      }

      m_TextRenderer->RenderText(item.name + " (x" +
                                     std::to_string(item.quantity) + ")",
                                 row.x + 10, y, {200, 200, 200, 255});

      m_TextRenderer->RenderText("Buy: " + std::to_string(item.value) + "g",
                                 row.x + 210, y, {255, 215, 0, 255});

      y += 35;
    }

    // Show Trader Gold

    int gold = 0;

    for (auto &it : nInv->items)
      if (it.name == "Coins")
        gold = it.quantity;

    m_TextRenderer->RenderText("Trader Gold: " + std::to_string(gold) + "g",
                               3 * w / 4 - 100, h - 100, {255, 255, 0, 255});

  } else {

    m_TextRenderer->RenderTextCentered("This NPC has nothing to trade.",
                                       3 * w / 4, h / 2, {150, 150, 150, 255});
  }

  m_TextRenderer->RenderTextCentered(
      "Click item to buy/sell. Press ESC to Close.", w / 2, h - 50,
      {150, 150, 150, 255});
}

void PixelsGateGame::HandleTradeInput() {

  auto escKey =
      PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Pause);

  if (PixelsEngine::Input::IsKeyPressed(escKey)) {

    m_State = m_ReturnState;

    return;
  }

  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);

  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  bool isClick = PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT);

  if (isClick) {

    auto *pInv =
        GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);

    auto *nInv = GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(
        m_TradingWith);

    if (!pInv || !nInv)
      return;

    // Helper to find gold

    auto getGold = [](PixelsEngine::InventoryComponent *inv) -> int {
      for (auto &item : inv->items)
        if (item.name == "Coins")
          return item.quantity;

      return 0;
    };

    // 1. Check Selling (Player Inventory)

    int y = 140;

    for (size_t i = 0; i < pInv->items.size(); ++i) {

      auto &item = pInv->items[i];

      if (item.name == "Coins")
        continue;

      SDL_Rect row = {w / 4 - 130, y - 5, 300, 30};

      if (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
          my <= row.y + row.h) {

        // Sell 1

        int traderGold = getGold(nInv);

        if (traderGold >= item.value) {

          PixelsEngine::Item soldItem = item;

          soldItem.quantity = 1;

          nInv->AddItemObject(soldItem);

          nInv->AddItem("Coins", -item.value); // Remove gold from trader

          pInv->AddItem("Coins", item.value); // Add gold to player

          item.quantity--;

          if (item.quantity <= 0)
            pInv->items.erase(pInv->items.begin() + i);

          SpawnFloatingText(0, 0, "Sold " + soldItem.name, {255, 215, 0, 255});

        } else {

          SpawnFloatingText(0, 0, "Trader needs more gold!", {255, 0, 0, 255});
        }

        return;
      }

      y += 35;
    }

    // 2. Check Buying (NPC Inventory)

    y = 140;

    for (size_t i = 0; i < nInv->items.size(); ++i) {

      auto &item = nInv->items[i];

      if (item.name == "Coins")
        continue;

      SDL_Rect row = {3 * w / 4 - 130, y - 5, 300, 30};

      if (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
          my <= row.y + row.h) {

        // Buy 1

        int playerGold = getGold(pInv);

        if (playerGold >= item.value) {

          PixelsEngine::Item boughtItem = item;

          boughtItem.quantity = 1;

          pInv->AddItemObject(boughtItem);

          pInv->AddItem("Coins", -item.value); // Remove gold from player

          nInv->AddItem("Coins", item.value); // Add gold to trader

          item.quantity--;

          if (item.quantity <= 0)
            nInv->items.erase(nInv->items.begin() + i);

          SpawnFloatingText(0, 0, "Bought " + boughtItem.name,
                            {255, 215, 0, 255});

        } else {

          SpawnFloatingText(0, 0, "You need more gold!", {255, 0, 0, 255});
        }

        return;
      }

      y += 35;
    }
  }
}

void PixelsGateGame::RenderKeybindSettings() {

  SDL_Renderer *renderer = GetRenderer();

  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);

  SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);

  SDL_RenderClear(renderer);

  m_TextRenderer->RenderTextCentered("CONTROLS & KEYBINDS", w / 2, 50,
                                     {255, 255, 0, 255});

  std::vector<PixelsEngine::GameAction> actions = {

      PixelsEngine::GameAction::MoveUp,
      PixelsEngine::GameAction::MoveDown,

      PixelsEngine::GameAction::MoveLeft,
      PixelsEngine::GameAction::MoveRight,

      PixelsEngine::GameAction::AttackModifier,
      PixelsEngine::GameAction::Inventory,

      PixelsEngine::GameAction::Map,
      PixelsEngine::GameAction::Character,

      PixelsEngine::GameAction::Magic,
      PixelsEngine::GameAction::Pause,

      PixelsEngine::GameAction::EndTurn

  };

  int y = 120;

  for (int i = 0; i < actions.size(); ++i) {

    SDL_Color color = (m_MenuSelection == i) ? SDL_Color{50, 255, 50, 255}
                                             : SDL_Color{255, 255, 255, 255};

    std::string actionName = PixelsEngine::Config::GetActionName(actions[i]);

    SDL_Scancode code = PixelsEngine::Config::GetKeybind(actions[i]);

    std::string keyName = SDL_GetScancodeName(code);

    if (m_IsWaitingForKey && m_MenuSelection == i) {

      keyName = "PRESS ANY KEY...";

      color = {255, 100, 100, 255};
    }

    m_TextRenderer->RenderText(actionName, w / 2 - 200, y, color);

    m_TextRenderer->RenderText(keyName, w / 2 + 100, y, color);

    if (m_MenuSelection == i) {

      m_TextRenderer->RenderTextCentered(">", w / 2 - 220, y + 10, color);
    }

    y += 35;
  }

  m_TextRenderer->RenderTextCentered(
      "Use W/S to select, ENTER to remap, ESC to go back.", w / 2, h - 50,
      {150, 150, 150, 255});
}

void PixelsGateGame::HandleKeybindInput() {

  if (m_IsWaitingForKey) {

    // Poll for any key

    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) {

      if (PixelsEngine::Input::IsKeyPressed((SDL_Scancode)i)) {

        std::vector<PixelsEngine::GameAction> actions = {

            PixelsEngine::GameAction::MoveUp,
            PixelsEngine::GameAction::MoveDown,

            PixelsEngine::GameAction::MoveLeft,
            PixelsEngine::GameAction::MoveRight,

            PixelsEngine::GameAction::AttackModifier,
            PixelsEngine::GameAction::Inventory,

            PixelsEngine::GameAction::Map,
            PixelsEngine::GameAction::Character,

            PixelsEngine::GameAction::Magic,
            PixelsEngine::GameAction::Pause,

            PixelsEngine::GameAction::EndTurn

        };

        PixelsEngine::Config::SetKeybind(actions[m_MenuSelection],
                                         (SDL_Scancode)i);

        m_IsWaitingForKey = false;

        return;
      }
    }

    return;
  }

  int numOptions = 11;

  bool isUp = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_W);

  bool isDown = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_S);

  bool isEnter = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_RETURN);

  bool isEsc = PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE);

  if (isUp) {
    m_MenuSelection--;
    if (m_MenuSelection < 0)
      m_MenuSelection = numOptions - 1;
  }

  if (isDown) {
    m_MenuSelection++;
    if (m_MenuSelection >= numOptions)
      m_MenuSelection = 0;
  }

  if (isEnter) {
    m_IsWaitingForKey = true;
  }

  if (isEsc) {
    m_State = GameState::Paused;
    m_MenuSelection = 0;
  }
}

void PixelsGateGame::RenderLootScreen() {
  SDL_Renderer *renderer = GetRenderer();
  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);

  // Overlay
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 30, 30, 30, 230);
  SDL_Rect screenRect = {0, 0, w, h};
  SDL_RenderFillRect(renderer, &screenRect);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  int panelW = 400, panelH = 500;
  SDL_Rect panel = {(w - panelW) / 2, (h - panelH) / 2, panelW, panelH};
  SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
  SDL_RenderFillRect(renderer, &panel);
  SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
  SDL_RenderDrawRect(renderer, &panel);

  m_TextRenderer->RenderTextCentered("LOOT BODY", panel.x + panelW / 2,
                                     panel.y + 30, {255, 215, 0, 255});

  // Close Button
  SDL_Rect closeBtn = {panel.x + panelW - 30, panel.y + 10, 20, 20};
  SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
  SDL_RenderFillRect(renderer, &closeBtn);
  m_TextRenderer->RenderTextCentered("X", closeBtn.x + 10, closeBtn.y + 10,
                                     {255, 255, 255, 255});

  // Take All Button
  SDL_Rect takeAllBtn = {panel.x + 20, panel.y + panelH - 60, 120, 40};
  SDL_SetRenderDrawColor(renderer, 50, 150, 50, 255);
  SDL_RenderFillRect(renderer, &takeAllBtn);
  m_TextRenderer->RenderTextCentered("TAKE ALL", takeAllBtn.x + 60,
                                     takeAllBtn.y + 20, {255, 255, 255, 255});

  auto *loot =
      GetRegistry().GetComponent<PixelsEngine::LootComponent>(m_LootingEntity);
  if (loot) {
    int itemY = panel.y + 80;
    for (const auto &item : loot->drops) {
      SDL_Rect row = {panel.x + 20, itemY - 5, panelW - 40, 40};
      bool hover = (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
                    my <= row.y + row.h);

      if (hover) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);
        SDL_RenderFillRect(renderer, &row);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
      }

      RenderInventoryItem(item, panel.x + 30, itemY);
      itemY += 45;
    }
    if (loot->drops.empty()) {
      m_TextRenderer->RenderTextCentered("Body is empty.", panel.x + panelW / 2,
                                         panel.y + panelH / 2,
                                         {150, 150, 150, 255});
    }
  }

  m_TextRenderer->RenderTextCentered("Double-click to take item. ESC to Close.",
                                     w / 2, h - 30, {150, 150, 150, 255});
}

void PixelsGateGame::HandleLootInput() {
  auto escKey =
      PixelsEngine::Config::GetKeybind(PixelsEngine::GameAction::Pause);
  if (PixelsEngine::Input::IsKeyPressed(escKey)) {
    m_State = m_ReturnState;
    return;
  }

  int w, h;
  SDL_GetWindowSize(m_Window, &w, &h);
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);
  bool pressed = PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT);

  if (!pressed)
    return;

  int panelW = 400, panelH = 500;
  SDL_Rect panel = {(w - panelW) / 2, (h - panelH) / 2, panelW, panelH};

  // Check Close Button
  SDL_Rect closeBtn = {panel.x + panelW - 30, panel.y + 10, 20, 20};
  if (mx >= closeBtn.x && mx <= closeBtn.x + closeBtn.w && my >= closeBtn.y &&
      my <= closeBtn.y + closeBtn.h) {
    m_State = m_ReturnState;
    return;
  }

  auto *pInv =
      GetRegistry().GetComponent<PixelsEngine::InventoryComponent>(m_Player);
  auto *loot =
      GetRegistry().GetComponent<PixelsEngine::LootComponent>(m_LootingEntity);
  if (!pInv || !loot)
    return;

  // Check Take All
  SDL_Rect takeAllBtn = {panel.x + 20, panel.y + panelH - 60, 120, 40};
  if (mx >= takeAllBtn.x && mx <= takeAllBtn.x + takeAllBtn.w &&
      my >= takeAllBtn.y && my <= takeAllBtn.y + takeAllBtn.h) {
    for (const auto &item : loot->drops) {
      pInv->AddItemObject(item);
    }
    loot->drops.clear();
    SpawnFloatingText(0, 0, "Looted all items", {255, 215, 0, 255});
    return;
  }

  // Check Individual Items (Double Click)
  float currentTime = SDL_GetTicks() / 1000.0f;
  int itemY = panel.y + 80;
  for (int i = 0; i < (int)loot->drops.size(); ++i) {
    SDL_Rect row = {panel.x + 20, itemY - 5, panelW - 40, 40};
    if (mx >= row.x && mx <= row.x + row.w && my >= row.y &&
        my <= row.y + row.h) {
      if (i == m_LastClickedItemIndex &&
          (currentTime - m_LastClickTime) < 0.5f) {
        pInv->AddItemObject(loot->drops[i]);
        loot->drops.erase(loot->drops.begin() + i);
        m_LastClickedItemIndex = -1;
        return;
      }
      m_LastClickTime = currentTime;
      m_LastClickedItemIndex = i;
      return;
    }
    itemY += 45;
  }
}

bool PixelsGateGame::IsInTurnOrder(PixelsEngine::Entity entity) {
  for (const auto &turn : m_Combat.m_TurnOrder) {
    if (turn.entity == entity)
      return true;
  }
  return false;
}

void PixelsGateGame::HandleTargetingJumpInput() {
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);
  if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
    auto &camera = GetCamera();
    int worldX = mx + camera.x, worldY = my + camera.y, gridX, gridY;
    m_Level->ScreenToGrid(worldX, worldY, gridX, gridY);

    if (m_Level->IsWalkable(gridX, gridY)) {
      auto *transform =
          GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
              m_Player);
      float dist = std::sqrt(std::pow(transform->x - gridX, 2) +
                             std::pow(transform->y - gridY, 2));

      bool canJump = true;
      if (m_State == GameState::Combat || m_ReturnState == GameState::Combat) {
        if (m_Combat.m_BonusActionsLeft <= 0 || m_Combat.m_MovementLeft < 3.0f)
          canJump = false;
      }

      if (canJump && dist < 5.0f) {
        transform->x = (float)gridX;
        transform->y = (float)gridY;
        if (m_State == GameState::Combat ||
            m_ReturnState == GameState::Combat) {
          m_Combat.m_BonusActionsLeft--;
          m_Combat.m_MovementLeft -= 3.0f;
        }
        SpawnFloatingText(transform->x, transform->y, "Jump!",
                          {100, 255, 255, 255});
        m_State = m_ReturnState;
      } else {
        SpawnFloatingText(0, 0, "Can't jump there!", {255, 0, 0, 255});
      }
    }
  }
  if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE))
    m_State = m_ReturnState;
}

void PixelsGateGame::HandleTargetingShoveInput() {
  int mx, my;
  PixelsEngine::Input::GetMousePosition(mx, my);
  if (PixelsEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
    auto &camera = GetCamera();
    auto &transforms = GetRegistry().View<PixelsEngine::TransformComponent>();
    PixelsEngine::Entity target = PixelsEngine::INVALID_ENTITY;

    for (auto &[entity, transform] : transforms) {
      if (entity == m_Player)
        continue;
      auto *sprite =
          GetRegistry().GetComponent<PixelsEngine::SpriteComponent>(entity);
      if (sprite) {
        int screenX, screenY;
        m_Level->GridToScreen(transform.x, transform.y, screenX, screenY);
        screenX -= (int)camera.x;
        screenY -= (int)camera.y;
        SDL_Rect drawRect = {screenX + 16 - sprite->pivotX,
                             screenY + 16 - sprite->pivotY, sprite->srcRect.w,
                             sprite->srcRect.h};
        if (mx >= drawRect.x && mx <= drawRect.x + drawRect.w &&
            my >= drawRect.y && my <= drawRect.y + drawRect.h) {
          target = entity;
          break;
        }
      }
    }

    if (target != PixelsEngine::INVALID_ENTITY) {
      bool canShove = true;
      if (m_State == GameState::Combat || m_ReturnState == GameState::Combat) {
        if (m_Combat.m_BonusActionsLeft <= 0)
          canShove = false;
      }

      if (canShove) {
        auto *pTrans =
            GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
                m_Player);
        auto *tTrans =
            GetRegistry().GetComponent<PixelsEngine::TransformComponent>(
                target);

        float dx = tTrans->x - pTrans->x;
        float dy = tTrans->y - pTrans->y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0) {
          dx /= len;
          dy /= len;
        }

        float pushX = tTrans->x + dx * 2.0f;
        float pushY = tTrans->y + dy * 2.0f;

        if (m_Level->IsWalkable((int)pushX, (int)pushY)) {
          tTrans->x = pushX;
          tTrans->y = pushY;
        }

        if (m_State == GameState::Combat || m_ReturnState == GameState::Combat)
          m_Combat.m_BonusActionsLeft--;
        SpawnFloatingText(tTrans->x, tTrans->y, "Shove!", {255, 150, 50, 255});
        m_State = m_ReturnState;
      }
    }
  }
  if (PixelsEngine::Input::IsKeyPressed(SDL_SCANCODE_ESCAPE))
    m_State = m_ReturnState;
}

void PixelsGateGame::InitCampMap() {
  std::string isoTileset = "assets/isometric tileset/spritesheet.png";
  int campW = 15;
  int campH = 15;
  m_CampLevel = std::make_unique<PixelsEngine::Tilemap>(
      GetRenderer(), isoTileset, 32, 32, campW, campH);
  m_CampLevel->SetProjection(PixelsEngine::Projection::Isometric);

  using namespace PixelsEngine::Tiles;
  for (int y = 0; y < campH; ++y) {
    for (int x = 0; x < campW; ++x) {
      int tile = GRASS;
      if (x == 0 || x == campW - 1 || y == 0 || y == campH - 1)
        tile = ROCK;
      else if (x >= 6 && x <= 8 && y >= 6 && y <= 8) {
        if (x == 7 && y == 7)
          tile = SMOOTH_STONE; // Campfire spot
        else
          tile = DIRT;
      }
      m_CampLevel->SetTile(x, y, tile);
    }
  }
}

void PixelsGateGame::RenderTooltip(const TooltipData &data, int x, int y) {
  SDL_Renderer *renderer = GetRenderer();
  int w = 320;

  // Calculate required height
  int descHeight = m_TextRenderer->MeasureTextWrapped(data.description, w - 20);
  // Header (Name+Line): 40
  // Subheader (Cost+Range): 30
  // Description: descHeight
  // Footer (Effect+Save): 35
  // Pin hint: 25
  // Padding: 20
  int totalH = 40 + 30 + descHeight + 35 + 25 + 20;

  // Adjust if tooltip goes off screen
  int winW, winH;
  SDL_GetWindowSize(m_Window, &winW, &winH);
  if (x + w > winW)
    x = winW - w - 10;
  if (x < 10)
    x = 10;
  if (y + totalH > winH)
    y = winH - totalH - 10;
  if (y < 10)
    y = 10;

  SDL_Rect box = {x, y, w, totalH};

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 20, 20, 25, 240);
  SDL_RenderFillRect(renderer, &box);
  SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
  SDL_RenderDrawRect(renderer, &box);
  if (m_TooltipPinned) {
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    SDL_RenderDrawRect(renderer, &box);
  }

  int currentY = y + 10;
  m_TextRenderer->RenderText(data.name, x + 10, currentY, {255, 255, 255, 255});
  currentY += 30;

  SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
  SDL_RenderDrawLine(renderer, x + 10, currentY, x + w - 10, currentY);
  currentY += 10;

  SDL_Color costCol = {200, 200, 200, 255};
  if (data.cost.find("Action") != std::string::npos)
    costCol = {50, 255, 50, 255};
  else if (data.cost.find("Bonus") != std::string::npos)
    costCol = {255, 150, 50, 255};

  m_TextRenderer->RenderTextSmall("Cost: " + data.cost, x + 10, currentY,
                                  costCol);
  m_TextRenderer->RenderTextSmall("Range: " + data.range, x + 160, currentY,
                                  {200, 200, 255, 255});
  currentY += 30;

  m_TextRenderer->RenderTextWrapped(data.description, x + 10, currentY, w - 20,
                                    {180, 180, 180, 255});
  currentY += descHeight + 15;

  m_TextRenderer->RenderTextSmall("Dmg/Effect: " + data.effect, x + 10,
                                  currentY, {255, 100, 100, 255});
  if (data.save != "None") {
    m_TextRenderer->RenderTextSmall("Save: " + data.save, x + 160, currentY,
                                    {255, 255, 100, 255});
  }
  currentY += 25;

  m_TextRenderer->RenderTextSmall(m_TooltipPinned ? "[T] Unpin"
                                                  : "[T] Pin Tooltip",
                                  x + 10, currentY, {100, 100, 100, 255});

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void PixelsGateGame::SpawnWorldEntities() {
  auto playerTexture = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/knight.png");

  // Load NPC Textures
  auto innkeeperTex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/npc_innkeeper.png");
  auto guardianTex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/npc_guardian.png");
  auto companionTex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/npc_companion.png");
  auto traderTex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/npc_trader.png");

  // 3. Spawn Boars
  CreateBoar(35.0f, 35.0f);
  CreateBoar(32.0f, 5.0f);
  CreateBoar(5.0f, 32.0f);

  // 4. NPCs
  auto npc1 = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(npc1,
                             PixelsEngine::TransformComponent{19.0f, 19.0f});
  GetRegistry().AddComponent(
      npc1, PixelsEngine::SpriteComponent{innkeeperTex, {0, 0, 32, 32}, 16, 30});
  GetRegistry().AddComponent(
      npc1, PixelsEngine::InteractionComponent{"Innkeeper", "npc_innkeeper",
                                               false, 0.0f});
  GetRegistry().AddComponent(npc1,
                             PixelsEngine::StatsComponent{50, 50, 5, false});
  GetRegistry().AddComponent(
      npc1, PixelsEngine::QuestComponent{"FetchOrb", 0, "Gold Orb"});
  GetRegistry().AddComponent(
      npc1, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Quest});
  GetRegistry().AddComponent(npc1, PixelsEngine::AIComponent{8.0f, 1.5f, 2.0f,
                                                             0.0f, false, 0.0f,
                                                             90.0f, 90.0f});

  // ... Dialogue Trees ...
  PixelsEngine::DialogueTree innTree;
  innTree.currentEntityName = "Innkeeper";
  innTree.currentNodeId = "start";
  PixelsEngine::DialogueNode startNode;
  startNode.id = "start";
  startNode.npcText =
      "Welcome to the Pixel Inn! What can I do for you today, traveler?";
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "I'm looking for work. [Intelligence DC 10]", "work_check",
      "Intelligence", 10, "work_success", "work_fail",
      PixelsEngine::DialogueAction::None, "", "Inn_Work_Topic_Closed", false));
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "I found the Gold Orb.", "quest_complete", "None", 0, "", "",
      PixelsEngine::DialogueAction::CompleteQuest, "Quest_FetchOrb_Done",
      "Quest_FetchOrb_Done", true, "Quest_FetchOrb_Active", "Gold Orb"));
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "How are things?", "quest_chat", "None", 0, "", "",
      PixelsEngine::DialogueAction::None, "", "", true, "Quest_FetchOrb_Done"));
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "Can I get a discount on a room? [Charisma DC 12]", "room_check",
      "Charisma", 12, "discount_success", "discount_fail",
      PixelsEngine::DialogueAction::None, "", "discount_active", false));
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "I don't like your face. [Attack]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::StartCombat));
  startNode.options.push_back(PixelsEngine::DialogueOption(
      "Just looking around. [End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  innTree.nodes["start"] = startNode;

  PixelsEngine::DialogueNode workSuccess;
  workSuccess.id = "work_success";
  workSuccess.npcText = "Find my lucky Gold Orb in the woods. Find it for me?";
  workSuccess.options.push_back(PixelsEngine::DialogueOption(
      "I'll find it.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::StartQuest, "Quest_FetchOrb_Active"));
  innTree.nodes["work_success"] = workSuccess;

  PixelsEngine::DialogueNode workFail;
  workFail.id = "work_fail";
  workFail.npcText = "You look slow.";
  workFail.options.push_back(PixelsEngine::DialogueOption(
      "Hmph.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  innTree.nodes["work_fail"] = workFail;
  PixelsEngine::DialogueNode questComplete;
  questComplete.id = "quest_complete";
  questComplete.npcText = "My Orb! Thank you.";
  questComplete.options.push_back(PixelsEngine::DialogueOption(
      "Happy to help.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  innTree.nodes["quest_complete"] = questComplete;
  PixelsEngine::DialogueNode questChat;
  questChat.id = "quest_chat";
  questChat.npcText = "Business is booming!";
  questChat.options.push_back(PixelsEngine::DialogueOption(
      "Glad to hear it.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  innTree.nodes["quest_chat"] = questChat;
  PixelsEngine::DialogueNode discountSuccess;
  discountSuccess.id = "discount_success";
  discountSuccess.npcText = "Fine, half price.";
  discountSuccess.options.push_back(PixelsEngine::DialogueOption(
      "Much appreciated.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation, "discount_active"));
  innTree.nodes["discount_success"] = discountSuccess;
  PixelsEngine::DialogueNode discountFail;
  discountFail.id = "discount_fail";
  discountFail.npcText = "Full price.";
  discountFail.options.push_back(PixelsEngine::DialogueOption(
      "Worth a shot.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  innTree.nodes["discount_fail"] = discountFail;

  GetRegistry().AddComponent(
      npc1, PixelsEngine::DialogueComponent{
                std::make_shared<PixelsEngine::DialogueTree>(innTree)});
  auto &n1Inv =
      GetRegistry().AddComponent(npc1, PixelsEngine::InventoryComponent{});
  n1Inv.AddItem("Coins", 100);
  n1Inv.AddItem("Potion", 1, PixelsEngine::ItemType::Consumable, 0,
                "assets/ui/item_potion.png", 50);

  auto npc2 = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(npc2,
                             PixelsEngine::TransformComponent{20.0f, 25.0f});
  GetRegistry().AddComponent(npc2, PixelsEngine::SpriteComponent{
                                       guardianTex, {0, 0, 32, 32}, 16, 30});
  GetRegistry().AddComponent(
      npc2, PixelsEngine::InteractionComponent{"Guardian", "npc_guardian",
                                               false, 0.0f});
  GetRegistry().AddComponent(npc2,
                             PixelsEngine::StatsComponent{50, 50, 5, false});
  GetRegistry().AddComponent(
      npc2, PixelsEngine::QuestComponent{"HuntBoars", 0, "Boar Meat"});
  GetRegistry().AddComponent(
      npc2, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Quest});
  GetRegistry().AddComponent(npc2, PixelsEngine::AIComponent{8.0f, 1.5f, 2.0f,
                                                             0.0f, false, 0.0f,
                                                             270.0f, 90.0f});

  PixelsEngine::DialogueTree guardTree;
  guardTree.currentEntityName = "Guardian";
  guardTree.currentNodeId = "start";
  PixelsEngine::DialogueNode gStart;
  gStart.id = "start";
  gStart.npcText = "Dangerous road.";
  gStart.options.push_back(PixelsEngine::DialogueOption(
      "I can handle myself. [Charisma DC 10]", "g_check", "Charisma", 10,
      "g_pass", "g_fail"));
  gStart.options.push_back(PixelsEngine::DialogueOption(
      "What's out there?", "g_info", "None", 0, "", "",
      PixelsEngine::DialogueAction::None, "", "", false));
  gStart.options.push_back(PixelsEngine::DialogueOption(
      "I have the Boar Meat.", "g_done", "None", 0, "", "",
      PixelsEngine::DialogueAction::CompleteQuest, "Quest_HuntBoars_Done",
      "Quest_HuntBoars_Done", true, "Quest_HuntBoars_Active", "Boar Meat"));
  gStart.options.push_back(
      PixelsEngine::DialogueOption("[Attack]", "end", "None", 0, "", "",
                                   PixelsEngine::DialogueAction::StartCombat));
  gStart.options.push_back(PixelsEngine::DialogueOption(
      "[End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  guardTree.nodes["start"] = gStart;
  PixelsEngine::DialogueNode gPass;
  gPass.id = "g_pass";
  gPass.npcText = "Fine, go die.";
  gPass.options.push_back(PixelsEngine::DialogueOption("[End]", "end"));
  guardTree.nodes["g_pass"] = gPass;
  PixelsEngine::DialogueNode gFail;
  gFail.id = "g_fail";
  gFail.npcText = "Ha! Shaking.";
  gFail.options.push_back(PixelsEngine::DialogueOption("[End]", "end"));
  guardTree.nodes["g_fail"] = gFail;

  PixelsEngine::DialogueNode gInfo;
  gInfo.id = "g_info";
  gInfo.npcText = "Boars. Big ones. And worse. Stay on the path.";
  gInfo.options.push_back(PixelsEngine::DialogueOption(
      "I can help hunt them.", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::StartQuest, "Quest_HuntBoars_Active"));
  gInfo.options.push_back(PixelsEngine::DialogueOption(
      "Thanks for the warning. [End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  guardTree.nodes["g_info"] = gInfo;
  PixelsEngine::DialogueNode gDone;
  gDone.id = "g_done";
  gDone.npcText = "Not useless.";
  gDone.options.push_back(PixelsEngine::DialogueOption("[End]", "end"));
  guardTree.nodes["g_done"] = gDone;

  GetRegistry().AddComponent(
      npc2, PixelsEngine::DialogueComponent{
                std::make_shared<PixelsEngine::DialogueTree>(guardTree)});
  auto &n2Inv =
      GetRegistry().AddComponent(npc2, PixelsEngine::InventoryComponent{});
  n2Inv.AddItem("Coins", 50);
  n2Inv.AddItem("Bread", 5, PixelsEngine::ItemType::Consumable, 0,
                "assets/ui/item_bread.png", 10);

  // 5. Spawn Gold Orb
  auto orb = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(orb, PixelsEngine::TransformComponent{5.0f, 5.0f});
  m_Level->SetTile(5, 5, PixelsEngine::Tiles::GRASS);
  auto orbTexture = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/gold_orb.png");
  GetRegistry().AddComponent(
      orb, PixelsEngine::SpriteComponent{orbTexture, {0, 0, 32, 32}, 16, 16});
  GetRegistry().AddComponent(
      orb, PixelsEngine::InteractionComponent{"Gold Orb", "item_gold_orb",
                                              false, 0.0f});

  // 6. Spawn Locked Chest
  auto chest = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(chest,
                             PixelsEngine::TransformComponent{22.0f, 22.0f});
  auto chestTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(),
                                                            "assets/chest.png");
  GetRegistry().AddComponent(
      chest, PixelsEngine::SpriteComponent{chestTex, {0, 0, 32, 32}, 16, 16});
  GetRegistry().AddComponent(chest, PixelsEngine::InteractionComponent{
                                        "Old Chest", "obj_chest", false, 0.0f});
  GetRegistry().AddComponent(
      chest, PixelsEngine::LockComponent{true, "Chest Key", 15});
  std::vector<PixelsEngine::Item> chestLoot;
  chestLoot.push_back({"Rare Gem", "assets/ui/item_raregem.png", 1,
                       PixelsEngine::ItemType::Misc, 0, 500});
  chestLoot.push_back({"Potion", "assets/ui/item_potion.png", 2,
                       PixelsEngine::ItemType::Consumable, 0, 50});
  GetRegistry().AddComponent(chest, PixelsEngine::LootComponent{chestLoot});
  GetRegistry().AddComponent(chest,
                             PixelsEngine::StatsComponent{20, 20, 0, false});

  // 7. Spawn Items (Tools)
  auto toolsEnt = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(toolsEnt,
                             PixelsEngine::TransformComponent{21.0f, 25.0f});
  auto toolsTex = PixelsEngine::TextureManager::LoadTexture(
      GetRenderer(), "assets/thieves_tools.png");
  GetRegistry().AddComponent(toolsEnt, PixelsEngine::SpriteComponent{
                                           toolsTex, {0, 0, 32, 32}, 16, 16});
  GetRegistry().AddComponent(
      toolsEnt, PixelsEngine::InteractionComponent{"Thieves' Tools",
                                                   "item_tools", false, 0.0f});
  GetRegistry().AddComponent(
      toolsEnt, PixelsEngine::LootComponent{std::vector<PixelsEngine::Item>{
                    {"Thieves' Tools", "assets/thieves_tools.png", 1,
                     PixelsEngine::ItemType::Tool, 0, 25}}});

  // Add a Companion (Traveler)
  auto comp = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(
      comp, PixelsEngine::TransformComponent{21.0f, 21.0f}); // In Inn
  GetRegistry().AddComponent(comp, PixelsEngine::SpriteComponent{
                                       companionTex, {0, 0, 32, 32}, 16, 30});
  GetRegistry().AddComponent(
      comp, PixelsEngine::InteractionComponent{"Traveler", "npc_traveler",
                                               false, 0.0f});
  GetRegistry().AddComponent(
      comp, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Companion});
  GetRegistry().AddComponent(comp,
                             PixelsEngine::StatsComponent{80, 80, 8, false});
  GetRegistry().AddComponent(
      comp, PixelsEngine::AIComponent{10.0f, 1.5f, 2.0f, 0.0f, false});

  PixelsEngine::DialogueTree compTree;
  compTree.currentEntityName = "Traveler";
  compTree.currentNodeId = "start";
  PixelsEngine::DialogueNode cStart;
  cStart.id = "start";
  cStart.npcText = "Quiet night, isn't it? Thinking of heading out?";
  cStart.options.push_back(PixelsEngine::DialogueOption(
      "Who are you?", "c_info", "None", 0, "", "",
      PixelsEngine::DialogueAction::None, "", "", false));
  cStart.options.push_back(PixelsEngine::DialogueOption(
      "[End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  compTree.nodes["start"] = cStart;

  PixelsEngine::DialogueNode cInfo;
  cInfo.id = "c_info";
  cInfo.npcText =
      "Just a wanderer, like you. I've seen things you wouldn't believe.";
  cInfo.options.push_back(PixelsEngine::DialogueOption(
      "Interesting. [End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  compTree.nodes["c_info"] = cInfo;

  GetRegistry().AddComponent(
      comp, PixelsEngine::DialogueComponent{
                std::make_shared<PixelsEngine::DialogueTree>(compTree)});

  auto &cInv =
      GetRegistry().AddComponent(comp, PixelsEngine::InventoryComponent{});
  cInv.AddItem("Coins", 10);

  // Add a Trader
  auto trader = GetRegistry().CreateEntity();
  GetRegistry().AddComponent(
      trader, PixelsEngine::TransformComponent{18.0f, 21.0f}); // In Inn
  GetRegistry().AddComponent(
      trader,
      PixelsEngine::SpriteComponent{traderTex, {0, 0, 32, 32}, 16, 30});
  GetRegistry().AddComponent(trader, PixelsEngine::InteractionComponent{
                                         "Trader", "npc_trader", false, 0.0f});
  GetRegistry().AddComponent(
      trader, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Trader});
  GetRegistry().AddComponent(trader,
                             PixelsEngine::StatsComponent{100, 100, 10, false});
  GetRegistry().AddComponent(
      trader, PixelsEngine::AIComponent{8.0f, 1.5f, 2.0f, 0.0f, false, 0.0f,
                                        0.0f, 120.0f});

  PixelsEngine::DialogueTree tradeTree;
  tradeTree.currentEntityName = "Trader";
  tradeTree.currentNodeId = "start";
  PixelsEngine::DialogueNode tStart;
  tStart.id = "start";
  tStart.npcText =
      "Looking for supplies? I've got the best prices in the valley.";
  tStart.options.push_back(PixelsEngine::DialogueOption(
      "Show me your wares. [Trade]", "start", "None", 0, "", "",
      PixelsEngine::DialogueAction::GiveItem));
  tStart.options.push_back(PixelsEngine::DialogueOption(
      "Any news? [Persuasion DC 10]", "t_check", "Charisma", 10, "t_pass",
      "t_fail", PixelsEngine::DialogueAction::None, "", "", false));
  tStart.options.push_back(PixelsEngine::DialogueOption(
      "[End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  tradeTree.nodes["start"] = tStart;

  PixelsEngine::DialogueNode tPass;
  tPass.id = "t_pass";
  tPass.npcText = "Rumor has it there's a golden orb hidden in the forest. "
                  "Worth a fortune.";
  tPass.options.push_back(PixelsEngine::DialogueOption(
      "I'll keep an eye out. [End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  tradeTree.nodes["t_pass"] = tPass;

  PixelsEngine::DialogueNode tFail;
  tFail.id = "t_fail";
  tFail.npcText =
      "Nothing for free, friend. Buy something and maybe I'll talk.";
  tFail.options.push_back(PixelsEngine::DialogueOption(
      "[End]", "end", "None", 0, "", "",
      PixelsEngine::DialogueAction::EndConversation));
  tradeTree.nodes["t_fail"] = tFail;

  GetRegistry().AddComponent(
      trader, PixelsEngine::DialogueComponent{
                  std::make_shared<PixelsEngine::DialogueTree>(tradeTree)});

  auto &tInv =
      GetRegistry().AddComponent(trader, PixelsEngine::InventoryComponent{});
  tInv.AddItem("Coins", 1000);
  tInv.AddItem("Potion", 5, PixelsEngine::ItemType::Consumable, 0,
               "assets/ui/item_potion.png", 50);
  tInv.AddItem("Bread", 5, PixelsEngine::ItemType::Consumable, 0,
               "assets/ui/item_bread.png", 10);
  tInv.AddItem("Sword", 1, PixelsEngine::ItemType::WeaponMelee, 10,
               "assets/sword.png", 150);
  tInv.AddItem("Leather Armor", 1, PixelsEngine::ItemType::Armor, 5,
               "assets/armor.png", 200);
  tInv.AddItem("Shortbow", 1, PixelsEngine::ItemType::WeaponRanged, 8,
               "assets/bow.png", 120);
  tInv.AddItem("Chest Key", 1, PixelsEngine::ItemType::Misc, 0,
               "assets/key.png", 50);
}
