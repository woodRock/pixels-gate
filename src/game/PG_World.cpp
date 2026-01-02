#include "PixelsGateGame.h"
#include "../engine/Tilemap.h"
#include "../engine/AnimationSystem.h"
#include "../engine/TextureManager.h"
#include "../engine/Tiles.h"
#include <cmath>

void PixelsGateGame::InitCampMap() {
    std::string isoTileset = "assets/isometric tileset/spritesheet.png";
    m_CampLevel = std::make_unique<PixelsEngine::Tilemap>(GetRenderer(), isoTileset, 32, 32, 15, 15);
    m_CampLevel->SetProjection(PixelsEngine::Projection::Isometric);
    using namespace PixelsEngine::Tiles;
    for (int y = 0; y < 15; ++y) {
        for (int x = 0; x < 15; ++x) {
            int tile = GRASS;
            if (x == 0 || x == 14 || y == 0 || y == 14) tile = ROCK;
            else if (x >= 6 && x <= 8 && y >= 6 && y <= 8) {
                tile = (x == 7 && y == 7) ? SMOOTH_STONE : DIRT;
            }
            m_CampLevel->SetTile(x, y, tile);
        }
    }
}

void PixelsGateGame::GenerateMainLevelTerrain() {
    std::string isoTileset = "assets/isometric tileset/spritesheet.png";
    int mapW = 40, mapH = 40;
    m_Level = std::make_unique<PixelsEngine::Tilemap>(GetRenderer(), isoTileset, 32, 32, mapW, mapH);
    m_Level->SetProjection(PixelsEngine::Projection::Isometric);

    using namespace PixelsEngine::Tiles;
    for (int y = 0; y < mapH; ++y) {
        for (int x = 0; x < mapW; ++x) {
            int tile = GRASS;
            // The Inn
            if (x >= 17 && x <= 23 && y >= 17 && y <= 23) {
                if (x == 17 || x == 23 || y == 17 || y == 23) {
                    tile = LOGS;
                    if (x == 20 && y == 23) tile = DIRT;
                } else tile = SMOOTH_STONE;
            }
            // Paths
            else if ((x == 20) || (y == 20) || (x == y) || (x + y == 40) ||
                     (std::sqrt(std::pow(x - 20, 2) + std::pow(y - 20, 2)) < 12.0f &&
                      std::sqrt(std::pow(x - 20, 2) + std::pow(y - 20, 2)) > 10.5f)) {
                tile = DIRT;
            }
            // River
            else if (std::abs(x - y) < 3) {
                tile = (std::abs(x - y) == 0) ? DEEP_WATER : WATER;
                if ((x + y) % 7 == 0) tile = ROCK_ON_WATER;
            }
            // Terrain
            else {
                float noise = std::sin(x * 0.2f) + std::cos(y * 0.2f);
                if (noise > 1.0f) tile = GRASS_WITH_BUSH;
                else if (noise < -1.2f) tile = DIRT_VARIANT_19;
                if (tile == GRASS && (x * y) % 13 == 0) tile = GRASS_VARIANT_01;
                if (tile == GRASS && (x * y) % 17 == 0) tile = FLOWER;
                if (tile == GRASS_WITH_BUSH && (x * y) % 11 == 0) tile = BUSH;
            }
            if (x == 0 || x == mapW - 1 || y == 0 || y == mapH - 1) tile = ROCK;
            m_Level->SetTile(x, y, tile);
        }
    }
}

void PixelsGateGame::CreateBoar(float x, float y) {
    auto boar = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(boar, PixelsEngine::TransformComponent{x, y});
    GetRegistry().AddComponent(boar, PixelsEngine::StatsComponent{30, 30, 2, false});
    GetRegistry().AddComponent(boar, PixelsEngine::InteractionComponent{"Boar", "npc_boar_" + std::to_string((int)x), false, 0.0f});
    GetRegistry().AddComponent(boar, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Hostile});
    GetRegistry().AddComponent(boar, PixelsEngine::AIComponent{8.0f, 1.2f, 2.0f, 0.0f, true});
    std::vector<PixelsEngine::Item> drops;
    drops.push_back({"Boar Meat", "assets/ui/item_boarmeat.png", 1, PixelsEngine::ItemType::Consumable, 0, 25});
    GetRegistry().AddComponent(boar, PixelsEngine::LootComponent{drops});
    auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/critters/boar/boar_SE_run_strip.png");
    GetRegistry().AddComponent(boar, PixelsEngine::SpriteComponent{tex, {0, 0, 41, 25}, 20, 20});
    auto &anim = GetRegistry().AddComponent(boar, PixelsEngine::AnimationComponent{});
    anim.AddAnimation("Idle", 0, 0, 41, 25, 1);
    anim.AddAnimation("Run", 0, 0, 41, 25, 4);
    anim.Play("Idle");
}

void PixelsGateGame::SpawnWorldEntities() {
    // Player
    m_Player = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(m_Player, PixelsEngine::TransformComponent{20.0f, 20.0f});
    GetRegistry().AddComponent(m_Player, PixelsEngine::PlayerComponent{5.0f});
    GetRegistry().AddComponent(m_Player, PixelsEngine::PathMovementComponent{});
    
    PixelsEngine::StatsComponent pStats{100, 100, 15, false, 0, 1, 1, false, false, 2, 2};
    pStats.strength = 10; pStats.dexterity = 10; pStats.constitution = 10;
    pStats.intelligence = 10; pStats.wisdom = 10; pStats.charisma = 10;
    GetRegistry().AddComponent(m_Player, pStats);
    auto &inv = GetRegistry().AddComponent(m_Player, PixelsEngine::InventoryComponent{});
    inv.AddItem("Potion", 3, PixelsEngine::ItemType::Consumable, 0, "assets/ui/item_potion.png", 50);
    inv.AddItem("Thieves' Tools", 1, PixelsEngine::ItemType::Tool, 0, "assets/thieves_tools.png", 25);
    auto playerTexture = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/knight.png");
    GetRegistry().AddComponent(m_Player, PixelsEngine::SpriteComponent{playerTexture, {0, 0, 32, 32}, 16, 32});
    auto &anim = GetRegistry().AddComponent(m_Player, PixelsEngine::AnimationComponent{});
    anim.AddAnimation("Idle", 0, 0, 32, 32, 1);
    anim.AddAnimation("WalkDown", 0, 0, 32, 32, 1);
    anim.AddAnimation("WalkRight", 0, 0, 32, 32, 1);
    anim.AddAnimation("WalkUp", 0, 0, 32, 32, 1);
    anim.AddAnimation("WalkLeft", 0, 0, 32, 32, 1);

    auto innkeeperTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/npc_innkeeper.png");
    auto guardianTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/npc_guardian.png");
    auto companionTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/npc_companion.png");
    auto traderTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/npc_trader.png");

    CreateBoar(35.0f, 35.0f);
    CreateBoar(32.0f, 5.0f);
    CreateBoar(5.0f, 32.0f);

    // Innkeeper
    auto npc1 = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(npc1, PixelsEngine::TransformComponent{19.0f, 19.0f});
    GetRegistry().AddComponent(npc1, PixelsEngine::SpriteComponent{innkeeperTex, {0, 0, 32, 32}, 16, 32});
    GetRegistry().AddComponent(npc1, PixelsEngine::InteractionComponent{"Innkeeper", "npc_innkeeper", false, 0.0f});
    GetRegistry().AddComponent(npc1, PixelsEngine::StatsComponent{50, 50, 5, false});
    GetRegistry().AddComponent(npc1, PixelsEngine::QuestComponent{"FetchOrb", 0, "Gold Orb"});
    GetRegistry().AddComponent(npc1, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Quest});
    GetRegistry().AddComponent(npc1, PixelsEngine::AIComponent{8.0f, 1.5f, 2.0f, 0.0f, false, 0.0f, 90.0f, 90.0f});

    PixelsEngine::DialogueTree innTree;
    innTree.currentEntityName = "Innkeeper";
    innTree.currentNodeId = "start";
    PixelsEngine::DialogueNode startNode;
    startNode.id = "start";
    startNode.npcText = "Welcome to the Pixel Inn! What can I do for you today, traveler?";
    startNode.options.push_back(PixelsEngine::DialogueOption("I'm looking for work. [Intelligence DC 10]", "work_check", "Intelligence", 10, "work_success", "work_fail", PixelsEngine::DialogueAction::None, "", "Quest_FetchOrb_Active", false));
    startNode.options.push_back(PixelsEngine::DialogueOption("I found the Gold Orb.", "quest_complete", "None", 0, "", "", PixelsEngine::DialogueAction::CompleteQuest, "Quest_FetchOrb_Done", "Quest_FetchOrb_Done", true, "Quest_FetchOrb_Found", "Gold Orb"));
    startNode.options.push_back(PixelsEngine::DialogueOption("How are things?", "quest_chat", "None", 0, "", "", PixelsEngine::DialogueAction::None, "", "", true, "Quest_FetchOrb_Done"));
    startNode.options.push_back(PixelsEngine::DialogueOption("Can I get a discount on a room? [Charisma DC 12]", "room_check", "Charisma", 12, "discount_success", "discount_fail", PixelsEngine::DialogueAction::None, "", "discount_active", false));
    startNode.options.push_back(PixelsEngine::DialogueOption("I don't like your face. [Attack]", "end", "None", 0, "", "", PixelsEngine::DialogueAction::StartCombat));
    startNode.options.push_back(PixelsEngine::DialogueOption("Just looking around. [End]", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    innTree.nodes["start"] = startNode;

    PixelsEngine::DialogueNode workSuccess; workSuccess.id = "work_success"; workSuccess.npcText = "Indeed, I lost my lucky Gold Orb. Find it?";
    workSuccess.options.push_back(PixelsEngine::DialogueOption("I'll find it.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::StartQuest, "Quest_FetchOrb_Active"));
    innTree.nodes["work_success"] = workSuccess;
    
    PixelsEngine::DialogueNode workFail; workFail.id = "work_fail"; workFail.npcText = "You look a bit... slow.";
    workFail.options.push_back(PixelsEngine::DialogueOption("Hmph.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    innTree.nodes["work_fail"] = workFail;

    PixelsEngine::DialogueNode qc; qc.id = "quest_complete"; qc.npcText = "My Orb! Thank you!";
    qc.options.push_back(PixelsEngine::DialogueOption("Happy to help.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    innTree.nodes["quest_complete"] = qc;

    PixelsEngine::DialogueNode qch; qch.id = "quest_chat"; qch.npcText = "Business is booming!";
    qch.options.push_back(PixelsEngine::DialogueOption("Glad to hear.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    innTree.nodes["quest_chat"] = qch;

    PixelsEngine::DialogueNode ds; ds.id = "discount_success"; ds.npcText = "Fine, half price.";
    ds.options.push_back(PixelsEngine::DialogueOption("Thanks.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation, "discount_active"));
    innTree.nodes["discount_success"] = ds;

    PixelsEngine::DialogueNode df; df.id = "discount_fail"; df.npcText = "Full price.";
    df.options.push_back(PixelsEngine::DialogueOption("Worth a shot.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    innTree.nodes["discount_fail"] = df;

    GetRegistry().AddComponent(npc1, PixelsEngine::DialogueComponent{std::make_shared<PixelsEngine::DialogueTree>(innTree)});
    auto &n1Inv = GetRegistry().AddComponent(npc1, PixelsEngine::InventoryComponent{});
    n1Inv.AddItem("Coins", 100);
    n1Inv.AddItem("Potion", 1, PixelsEngine::ItemType::Consumable, 0, "assets/ui/item_potion.png", 50);

    // Guardian
    auto npc2 = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(npc2, PixelsEngine::TransformComponent{20.0f, 25.0f});
    GetRegistry().AddComponent(npc2, PixelsEngine::SpriteComponent{guardianTex, {0, 0, 32, 32}, 16, 32});
    GetRegistry().AddComponent(npc2, PixelsEngine::InteractionComponent{"Guardian", "npc_guardian", false, 0.0f});
    GetRegistry().AddComponent(npc2, PixelsEngine::StatsComponent{50, 50, 5, false});
    GetRegistry().AddComponent(npc2, PixelsEngine::QuestComponent{"HuntBoars", 0, "Boar Meat"});
    GetRegistry().AddComponent(npc2, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Quest});
    GetRegistry().AddComponent(npc2, PixelsEngine::AIComponent{8.0f, 1.5f, 2.0f, 0.0f, false, 0.0f, 270.0f, 90.0f});

    PixelsEngine::DialogueTree guardTree; guardTree.currentEntityName = "Guardian"; guardTree.currentNodeId = "start";
    PixelsEngine::DialogueNode gStart; gStart.id = "start"; gStart.npcText = "Dangerous road ahead.";
    gStart.options.push_back(PixelsEngine::DialogueOption("I can handle myself. [Intimidate DC 10]", "g_check", "Charisma", 10, "g_pass", "g_fail", PixelsEngine::DialogueAction::None, "", "Quest_HuntBoars_Active", false));
    gStart.options.push_back(PixelsEngine::DialogueOption("What's out there?", "g_info", "None", 0, "", "", PixelsEngine::DialogueAction::None, "", "Quest_HuntBoars_Active"));
    gStart.options.push_back(PixelsEngine::DialogueOption("I have the Boar Meat.", "g_done", "None", 0, "", "", PixelsEngine::DialogueAction::CompleteQuest, "Quest_HuntBoars_Done", "Quest_HuntBoars_Done", true, "Quest_HuntBoars_Active", "Boar Meat"));
    gStart.options.push_back(PixelsEngine::DialogueOption("[Attack]", "end", "None", 0, "", "", PixelsEngine::DialogueAction::StartCombat));
    gStart.options.push_back(PixelsEngine::DialogueOption("Bye.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    guardTree.nodes["start"] = gStart;

    PixelsEngine::DialogueNode gp; gp.id = "g_pass"; gp.npcText = "Fine, go die."; gp.options.push_back(PixelsEngine::DialogueOption("[End]", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    guardTree.nodes["g_pass"] = gp;
    PixelsEngine::DialogueNode gf; gf.id = "g_fail"; gf.npcText = "Ha! You're shaking."; gf.options.push_back(PixelsEngine::DialogueOption("[End]", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    guardTree.nodes["g_fail"] = gf;
    PixelsEngine::DialogueNode gi; gi.id = "g_info"; gi.npcText = "Boars. Big ones."; gi.options.push_back(PixelsEngine::DialogueOption("I can help hunt.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::StartQuest, "Quest_HuntBoars_Active"));
    gi.options.push_back(PixelsEngine::DialogueOption("Thanks.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    guardTree.nodes["g_info"] = gi;
    PixelsEngine::DialogueNode gd; gd.id = "g_done"; gd.npcText = "Not useless after all."; gd.options.push_back(PixelsEngine::DialogueOption("Told you.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    guardTree.nodes["g_done"] = gd;

    GetRegistry().AddComponent(npc2, PixelsEngine::DialogueComponent{std::make_shared<PixelsEngine::DialogueTree>(guardTree)});
    auto &n2Inv = GetRegistry().AddComponent(npc2, PixelsEngine::InventoryComponent{});
    n2Inv.AddItem("Coins", 50);
    n2Inv.AddItem("Bread", 5, PixelsEngine::ItemType::Consumable, 0, "", 10);

    // Companion
    auto comp = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(comp, PixelsEngine::TransformComponent{21.0f, 21.0f});
    GetRegistry().AddComponent(comp, PixelsEngine::SpriteComponent{companionTex, {0, 0, 32, 32}, 16, 32});
    GetRegistry().AddComponent(comp, PixelsEngine::InteractionComponent{"Traveler", "npc_traveler", false, 0.0f});
    GetRegistry().AddComponent(comp, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Companion});
    GetRegistry().AddComponent(comp, PixelsEngine::StatsComponent{80, 80, 8, false});
    GetRegistry().AddComponent(comp, PixelsEngine::AIComponent{10.0f, 1.5f, 2.0f, 0.0f, false});
    PixelsEngine::DialogueTree compTree; compTree.currentEntityName = "Traveler"; compTree.currentNodeId = "start";
    PixelsEngine::DialogueNode cStart; cStart.id = "start"; cStart.npcText = "Quiet night.";
    cStart.options.push_back(PixelsEngine::DialogueOption("Who are you?", "c_info"));
    cStart.options.push_back(PixelsEngine::DialogueOption("[End]", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    compTree.nodes["start"] = cStart;
    PixelsEngine::DialogueNode ci; ci.id = "c_info"; ci.npcText = "Just a wanderer."; ci.options.push_back(PixelsEngine::DialogueOption("[End]", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    compTree.nodes["c_info"] = ci;
    GetRegistry().AddComponent(comp, PixelsEngine::DialogueComponent{std::make_shared<PixelsEngine::DialogueTree>(compTree)});
    auto &cInv = GetRegistry().AddComponent(comp, PixelsEngine::InventoryComponent{});
    cInv.AddItem("Coins", 10);

    // Trader
    auto trader = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(trader, PixelsEngine::TransformComponent{18.0f, 21.0f});
    GetRegistry().AddComponent(trader, PixelsEngine::SpriteComponent{traderTex, {0, 0, 32, 32}, 16, 32});
    GetRegistry().AddComponent(trader, PixelsEngine::InteractionComponent{"Trader", "npc_trader", false, 0.0f});
    GetRegistry().AddComponent(trader, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Trader});
    GetRegistry().AddComponent(trader, PixelsEngine::StatsComponent{100, 100, 10, false});
    GetRegistry().AddComponent(trader, PixelsEngine::AIComponent{8.0f, 1.5f, 2.0f, 0.0f, false, 0.0f, 0.0f, 120.0f});
    
    PixelsEngine::DialogueTree tradeTree; tradeTree.currentEntityName = "Trader"; tradeTree.currentNodeId = "start";
    PixelsEngine::DialogueNode tStart; tStart.id = "start"; tStart.npcText = "Looking for supplies?";
    tStart.options.push_back(PixelsEngine::DialogueOption("Show me your wares.", "start", "None", 0, "", "", PixelsEngine::DialogueAction::GiveItem));
    tStart.options.push_back(PixelsEngine::DialogueOption("[End]", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    tradeTree.nodes["start"] = tStart;
    GetRegistry().AddComponent(trader, PixelsEngine::DialogueComponent{std::make_shared<PixelsEngine::DialogueTree>(tradeTree)});
    auto &tInv = GetRegistry().AddComponent(trader, PixelsEngine::InventoryComponent{});
    tInv.AddItem("Sword", 1, PixelsEngine::ItemType::WeaponMelee, 5, "assets/sword.png", 100);
    tInv.AddItem("Bow", 1, PixelsEngine::ItemType::WeaponRanged, 3, "assets/bow.png", 120);
    tInv.AddItem("Armor", 1, PixelsEngine::ItemType::Armor, 2, "assets/armor.png", 150);
    tInv.AddItem("Potion", 5, PixelsEngine::ItemType::Consumable, 0, "assets/ui/item_potion.png", 50);
    tInv.AddItem("Chest Key", 1, PixelsEngine::ItemType::Misc, 0, "assets/key.png", 50);
    
    // Orb
    auto orb = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(orb, PixelsEngine::TransformComponent{5.0f, 5.0f});
    auto orbTexture = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/gold_orb.png");
    GetRegistry().AddComponent(orb, PixelsEngine::SpriteComponent{orbTexture, {0, 0, 32, 32}, 16, 16});
    GetRegistry().AddComponent(orb, PixelsEngine::InteractionComponent{"Gold Orb", "item_gold_orb", false, 0.0f});

    // Chest
    auto chest = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(chest, PixelsEngine::TransformComponent{22.0f, 22.0f});
    auto chestTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/chest.png");
    GetRegistry().AddComponent(chest, PixelsEngine::SpriteComponent{chestTex, {0, 0, 32, 32}, 16, 16});
    GetRegistry().AddComponent(chest, PixelsEngine::InteractionComponent{"Old Chest", "obj_chest", false, 0.0f});
    GetRegistry().AddComponent(chest, PixelsEngine::LockComponent{true, "Chest Key", 15});
    std::vector<PixelsEngine::Item> chestLoot;
    chestLoot.push_back({"Rare Gem", "assets/ui/item_raregem.png", 1, PixelsEngine::ItemType::Misc, 0, 500});
    chestLoot.push_back({"Potion", "assets/ui/item_potion.png", 2, PixelsEngine::ItemType::Consumable, 0, 50});
    GetRegistry().AddComponent(chest, PixelsEngine::LootComponent{chestLoot});
    GetRegistry().AddComponent(chest, PixelsEngine::StatsComponent{20, 20, 0, false});

    // Tools
    auto toolsEnt = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(toolsEnt, PixelsEngine::TransformComponent{21.0f, 25.0f});
    auto toolsTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/thieves_tools.png");
    GetRegistry().AddComponent(toolsEnt, PixelsEngine::SpriteComponent{toolsTex, {0, 0, 32, 32}, 16, 16});
    GetRegistry().AddComponent(toolsEnt, PixelsEngine::InteractionComponent{"Thieves' Tools", "item_tools", false, 0.0f});
    GetRegistry().AddComponent(toolsEnt, PixelsEngine::LootComponent{std::vector<PixelsEngine::Item>{{"Thieves' Tools", "assets/thieves_tools.png", 1, PixelsEngine::ItemType::Tool, 0, 25}}});
}