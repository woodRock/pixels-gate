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
    int mapW = 100, mapH = 100;
    m_Level = std::make_unique<PixelsEngine::Tilemap>(GetRenderer(), isoTileset, 32, 32, mapW, mapH);
    m_Level->SetProjection(PixelsEngine::Projection::Isometric);

    using namespace PixelsEngine::Tiles;
    
    // Perlin-ish noise for terrain
    auto Noise = [](float x, float y) {
        return std::sin(x * 0.1f) + std::cos(y * 0.1f) + std::sin(x * 0.3f + y * 0.2f) * 0.5f;
    };

    // POIs
    int innX = 20, innY = 20;
    int caveX = 85, caveY = 15;
    int deadManX = 50, deadManY = 60;

    for (int y = 0; y < mapH; ++y) {
        for (int x = 0; x < mapW; ++x) {
            int tile = GRASS;
            float n = Noise((float)x, (float)y);

            // Biomes
            if (n > 1.2f) tile = GRASS_WITH_BUSH;
            else if (n < -1.0f) tile = DIRT_VARIANT_19;
            
            // Forest patches
            if (n > 0.8f && (x*y)%7 == 0) tile = BUSH;
            if (n > 0.5f && (x*y)%13 == 0) tile = GRASS_VARIANT_01;

            // River (Diagonal)
            if (std::abs((x - 50) - (y - 50)) < 4) tile = WATER;
            
            // Roads (Simple A* or distance based interpolation for now)
            // Road 1: Inn to Dead Man
            float d1 = std::abs((x - innX) * (deadManY - innY) - (deadManX - innX) * (y - innY)) / std::sqrt(std::pow(deadManX - innX, 2) + std::pow(deadManY - innY, 2));
            if (d1 < 1.5f && x >= std::min(innX, deadManX) && x <= std::max(innX, deadManX) && y >= std::min(innY, deadManY) && y <= std::max(innY, deadManY)) tile = DIRT;

            // Road 2: Dead Man to Cave
            float d2 = std::abs((x - deadManX) * (caveY - deadManY) - (caveX - deadManX) * (y - deadManY)) / std::sqrt(std::pow(caveX - deadManX, 2) + std::pow(caveY - deadManY, 2));
            if (d2 < 1.5f && x >= std::min(deadManX, caveX) && x <= std::max(deadManX, caveX) && y >= std::min(caveY, deadManY) && y <= std::max(caveY, deadManY)) tile = DIRT;

            // Inn Area
            if (x >= 17 && x <= 23 && y >= 17 && y <= 23) {
                if (x == 17 || x == 23 || y == 17 || y == 23) {
                    tile = LOGS;
                    if (x == 20 && y == 23) tile = DIRT;
                } else tile = SMOOTH_STONE;
            }

            // Cave Area
            if (std::sqrt(std::pow(x - caveX, 2) + std::pow(y - caveY, 2)) < 8.0f) {
                tile = ROCK;
                if (std::sqrt(std::pow(x - caveX, 2) + std::pow(y - caveY, 2)) < 5.0f) tile = DIRT_WITH_PARTIAL_GRASS;
                
                // Cave Entrance (West side)
                if (x < caveX && std::abs(y - caveY) < 3) tile = DIRT;
            }

            if (x == 0 || x == mapW - 1 || y == 0 || y == mapH - 1) tile = ROCK;
            m_Level->SetTile(x, y, tile);
        }
    }
}

void PixelsGateGame::CreateWolf(float x, float y) {
    auto wolf = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(wolf, PixelsEngine::TransformComponent{x, y});
    GetRegistry().AddComponent(wolf, PixelsEngine::StatsComponent{40, 40, 6, false});
    GetRegistry().AddComponent(wolf, PixelsEngine::InteractionComponent{"Wolf", "npc_wolf", false, 0.0f});
    GetRegistry().AddComponent(wolf, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Hostile});
    GetRegistry().AddComponent(wolf, PixelsEngine::AIComponent{10.0f, 1.5f, 2.0f, 0.0f, true});
    auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/critters/wolf/wolf-run.png");
    GetRegistry().AddComponent(wolf, PixelsEngine::SpriteComponent{tex, {0, 0, 64, 32}, 32, 24});
    
    std::vector<PixelsEngine::Item> drops;
    drops.push_back({"Wolf Pelt", "assets/wolf_pelt.png", 1, PixelsEngine::ItemType::Misc, 0, 50});
    GetRegistry().AddComponent(wolf, PixelsEngine::LootComponent{drops});
}

void PixelsGateGame::CreateStag(float x, float y) {
    auto stag = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(stag, PixelsEngine::TransformComponent{x, y});
    GetRegistry().AddComponent(stag, PixelsEngine::StatsComponent{30, 30, 2, false});
    GetRegistry().AddComponent(stag, PixelsEngine::TagComponent{PixelsEngine::EntityTag::None}); // Passive
    GetRegistry().AddComponent(stag, PixelsEngine::AIComponent{8.0f, 1.5f, 2.0f, 0.0f, false}); // Not aggressive
    auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/critters/stag/critter_stag_SE_idle.png");
    GetRegistry().AddComponent(stag, PixelsEngine::SpriteComponent{tex, {0, 0, 41, 43}, 20, 32});

    std::vector<PixelsEngine::Item> drops;
    drops.push_back({"Stag Meat", "assets/stag_meat.png", 1, PixelsEngine::ItemType::Consumable, 0, 30});
    GetRegistry().AddComponent(stag, PixelsEngine::LootComponent{drops});
}

void PixelsGateGame::CreateBadger(float x, float y) {
    auto badger = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(badger, PixelsEngine::TransformComponent{x, y});
    GetRegistry().AddComponent(badger, PixelsEngine::StatsComponent{20, 20, 4, false});
    GetRegistry().AddComponent(badger, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Hostile});
    GetRegistry().AddComponent(badger, PixelsEngine::AIComponent{6.0f, 1.2f, 2.0f, 0.0f, true});
    auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/critters/badger/critter_badger_SE_idle.png");
    GetRegistry().AddComponent(badger, PixelsEngine::SpriteComponent{tex, {0, 0, 32, 22}, 16, 16});

    std::vector<PixelsEngine::Item> drops;
    drops.push_back({"Badger Pelt", "assets/badger_pelt.png", 1, PixelsEngine::ItemType::Misc, 0, 40});
    GetRegistry().AddComponent(badger, PixelsEngine::LootComponent{drops});
}

void PixelsGateGame::CreateWolfBoss(float x, float y) {
    auto boss = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(boss, PixelsEngine::TransformComponent{x, y});
    GetRegistry().AddComponent(boss, PixelsEngine::StatsComponent{150, 150, 12, false});
    GetRegistry().AddComponent(boss, PixelsEngine::InteractionComponent{"Dire Wolf", "boss_wolf", false, 0.0f});
    GetRegistry().AddComponent(boss, PixelsEngine::TagComponent{PixelsEngine::EntityTag::Hostile});
    GetRegistry().AddComponent(boss, PixelsEngine::AIComponent{12.0f, 2.0f, 2.0f, 0.0f, true});
    auto tex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/critters/wolf/wolf-howl.png");
    // Scale 2.0f
    GetRegistry().AddComponent(boss, PixelsEngine::SpriteComponent{tex, {0, 0, 64, 46}, 32, 32, SDL_FLIP_NONE, 2.0f});
    
    std::vector<PixelsEngine::Item> drops;
    drops.push_back({"Wolf Pelt", "assets/wolf_pelt.png", 1, PixelsEngine::ItemType::Misc, 0, 200});
    drops.push_back({"Magic Ring", "assets/magic_ring.png", 1, PixelsEngine::ItemType::Misc, 0, 500});
    GetRegistry().AddComponent(boss, PixelsEngine::LootComponent{drops});
}

void PixelsGateGame::CreateDeadManAndSon(float x, float y) {
    // Dead Father
    auto father = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(father, PixelsEngine::TransformComponent{x, y});
    auto fTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/npc_guardian.png"); // Reusing for body
    GetRegistry().AddComponent(father, PixelsEngine::SpriteComponent{fTex, {0, 0, 32, 32}, 16, 32}); // Should rotate 90 deg ideally
    GetRegistry().AddComponent(father, PixelsEngine::StatsComponent{0, 0, 0, true}); // Dead
    GetRegistry().AddComponent(father, PixelsEngine::InteractionComponent{"Dead Body", "dead_body", false, 0.0f});
    std::vector<PixelsEngine::Item> loot;
    loot.push_back({"Letter to Son", "assets/letter.png", 1, PixelsEngine::ItemType::Misc, 0, 0});
    GetRegistry().AddComponent(father, PixelsEngine::LootComponent{loot});

    // Son
    auto son = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(son, PixelsEngine::TransformComponent{x + 1.0f, y});
    auto sTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/npc_trader.png"); // Reusing
    GetRegistry().AddComponent(son, PixelsEngine::SpriteComponent{sTex, {0, 0, 32, 32}, 16, 32});
    GetRegistry().AddComponent(son, PixelsEngine::InteractionComponent{"Grieving Son", "npc_son", false, 0.0f});
    GetRegistry().AddComponent(son, PixelsEngine::TagComponent{PixelsEngine::EntityTag::NPC});
    GetRegistry().AddComponent(son, PixelsEngine::StatsComponent{30, 30, 3, false});
    GetRegistry().AddComponent(son, PixelsEngine::AIComponent{10.0f, 1.5f, 2.0f, 0.0f, false});

    PixelsEngine::DialogueTree tree;
    tree.currentEntityName = "Grieving Son";
    tree.currentNodeId = "start";
    
    PixelsEngine::DialogueNode start; start.id = "start"; start.npcText = "He's gone... The beast came out of nowhere. It... it tore him apart.";
    start.options.push_back(PixelsEngine::DialogueOption("You should seek revenge. Hunt it down! [Persuasion DC 12]", "rev_check", "Charisma", 12, "rev_success", "rev_fail", PixelsEngine::DialogueAction::None, "", "Quest_KillWolfBoss_Done", false, "")); // Hide if already done
    start.options.push_back(PixelsEngine::DialogueOption("It's too dangerous. Go home and live. [Persuasion DC 10]", "home_check", "Charisma", 10, "home_success", "home_fail", PixelsEngine::DialogueAction::None, "", "Quest_KillWolfBoss_Done", false, ""));
    start.options.push_back(PixelsEngine::DialogueOption("I will kill the wolf for you.", "hero", "None", 0, "", "", PixelsEngine::DialogueAction::StartQuest, "Quest_KillWolfBoss", "Quest_KillWolfBoss_Active", false, ""));
    start.options.push_back(PixelsEngine::DialogueOption("The wolf is dead. Justice is served.", "wolf_dead", "None", 0, "", "", PixelsEngine::DialogueAction::CompleteQuest, "Quest_KillWolfBoss_Done", "Quest_KillWolfBoss_Done", true, "WolfBoss_Dead"));
    start.options.push_back(PixelsEngine::DialogueOption("The deed is done. The wolf is dead.", "wolf_dead", "None", 0, "", "", PixelsEngine::DialogueAction::None, "", "Quest_KillWolfBoss_Done", true, "WolfBoss_Dead")); 
    
    // Recruit (Initial) - Show if Quest Done, NOT At Camp, NOT In Party
    start.options.push_back(PixelsEngine::DialogueOption("Come with me. We'll face the dangers together.", "join_party", "None", 0, "", "", PixelsEngine::DialogueAction::JoinParty, "", "Grieving Son_InParty", true, "Quest_KillWolfBoss_Done")); 
    
    // Recruit (Rejoin) - Show ONLY if At Camp
    start.options.push_back(PixelsEngine::DialogueOption("Join me. I have need of your skills.", "join_party_camp", "None", 0, "", "", PixelsEngine::DialogueAction::JoinParty, "camp", "", true, "camp")); 
    
    // Dismiss - Show ONLY if In Party
    start.options.push_back(PixelsEngine::DialogueOption("Wait at camp.", "dismiss", "None", 0, "", "", PixelsEngine::DialogueAction::Dismiss, "camp", "", true, "Grieving Son_InParty"));

    start.options.push_back(PixelsEngine::DialogueOption("Leave.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    tree.nodes["start"] = start;

    PixelsEngine::DialogueNode rs; rs.id = "rev_success"; rs.npcText = "You're right. I can't let this stand. I'll join you. For father!"; 
    rs.options.push_back(PixelsEngine::DialogueOption("Let's go.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::JoinParty));
    tree.nodes["rev_success"] = rs;

    PixelsEngine::DialogueNode rf; rf.id = "rev_fail"; rf.npcText = "I... I can't. I'm too scared."; 
    rf.options.push_back(PixelsEngine::DialogueOption("Pathetic.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    tree.nodes["rev_fail"] = rf;

    PixelsEngine::DialogueNode hs; hs.id = "home_success"; hs.npcText = "Maybe you're right. Mother needs me."; 
    hs.options.push_back(PixelsEngine::DialogueOption("Go.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    tree.nodes["home_success"] = hs;

    PixelsEngine::DialogueNode hf; hf.id = "home_fail"; hf.npcText = "No! I won't leave him here!"; 
    hf.options.push_back(PixelsEngine::DialogueOption("Suit yourself.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    tree.nodes["home_fail"] = hf;

    PixelsEngine::DialogueNode hero; hero.id = "hero"; hero.npcText = "You would do that? Thank you! It lives in the cave to the east."; 
    hero.options.push_back(PixelsEngine::DialogueOption("Consider it dead.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    tree.nodes["hero"] = hero;

    PixelsEngine::DialogueNode wd; wd.id = "wolf_dead"; wd.npcText = "You... you did it? Thank you. I found this coin purse on him. Please, take it.";
    wd.options.push_back(PixelsEngine::DialogueOption("Thank you.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    tree.nodes["wolf_dead"] = wd;

    PixelsEngine::DialogueNode jp; jp.id = "join_party"; jp.npcText = "I will follow your lead.";
    jp.options.push_back(PixelsEngine::DialogueOption("Let's move.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    tree.nodes["join_party"] = jp;

    PixelsEngine::DialogueNode jpc; jpc.id = "join_party_camp"; jpc.npcText = "I'm ready to fight again.";
    jpc.options.push_back(PixelsEngine::DialogueOption("Let's go.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    tree.nodes["join_party_camp"] = jpc;

    PixelsEngine::DialogueNode dis; dis.id = "dismiss"; dis.npcText = "I'll head to the camp then.";
    dis.options.push_back(PixelsEngine::DialogueOption("See you there.", "end", "None", 0, "", "", PixelsEngine::DialogueAction::EndConversation));
    tree.nodes["dismiss"] = dis;

    GetRegistry().AddComponent(son, PixelsEngine::DialogueComponent{std::make_shared<PixelsEngine::DialogueTree>(tree)});
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
    inv.AddItem("Camp Supplies", 3, PixelsEngine::ItemType::Misc, 0, "assets/ui/item_bread.png", 40, 40); // 3 Packs = 120 supplies
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

    // Helper to check for valid spawn location (Not on Road/Dirt)
    auto IsValidSpawn = [&](float x, float y) {
        int gx = (int)x; int gy = (int)y;
        if (m_Level) {
            int tile = m_Level->GetTile(gx, gy);
            // Assuming DIRT is the road tile (and DIRT_VARIANT_19 is road/biome)
            // We want to avoid spawning ON the road.
            if (tile == PixelsEngine::Tiles::DIRT || tile == PixelsEngine::Tiles::DIRT_VARIANT_19) return false;
            // Also check neighbors for "Close to road"
            for(int dy=-2; dy<=2; ++dy) {
                for(int dx=-2; dx<=2; ++dx) {
                    int t = m_Level->GetTile(gx+dx, gy+dy);
                    if (t == PixelsEngine::Tiles::DIRT) return false;
                }
            }
        }
        return true;
    };

    // Scatter Critters
    for(int i=0; i<8; ++i) {
        float x = 30.0f + (i*10); float y = 30.0f + (i*5);
        if(IsValidSpawn(x, y)) CreateWolf(x, y);
    }
    for(int i=0; i<10; ++i) {
        float x = 10.0f + (i*8); float y = 50.0f + (i*2);
        if(IsValidSpawn(x, y)) CreateStag(x, y);
    }
    for(int i=0; i<6; ++i) {
        float x = 60.0f + (i*5); float y = 20.0f + (i*6);
        if(IsValidSpawn(x, y)) CreateBadger(x, y);
    }
    
    // Scenario & Boss
    CreateDeadManAndSon(50.0f, 60.0f);
    CreateWolfBoss(85.0f, 15.0f); // In Cave Area

    // Boars: One is quest boar (allowed on/near road), others avoided
    CreateBoar(35.0f, 35.0f); // Quest Boar (likely near road)
    
    if(IsValidSpawn(32.0f, 5.0f)) CreateBoar(32.0f, 5.0f);
    if(IsValidSpawn(5.0f, 32.0f)) CreateBoar(5.0f, 32.0f);

    // Innkeeper
    auto npc1 = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(npc1, PixelsEngine::TransformComponent{19.0f, 19.0f});
    GetRegistry().AddComponent(npc1, PixelsEngine::SpriteComponent{innkeeperTex, {0, 0, 32, 32}, 16, 32});
    GetRegistry().AddComponent(npc1, PixelsEngine::InteractionComponent{"Innkeeper", "npc_innkeeper", false, 0.0f});
    GetRegistry().AddComponent(npc1, PixelsEngine::StatsComponent{50, 50, 5, false});
    GetRegistry().AddComponent(npc1, PixelsEngine::QuestComponent{"FetchOrb", "The innkeeper lost his lucky Gold Orb. He believes it's somewhere near the rocky cliffs.", 0, "Gold Orb"});
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
    GetRegistry().AddComponent(npc2, PixelsEngine::QuestComponent{"HuntBoars", "The road is dangerous. The guardian needs you to thin out the boar population and bring back some meat.", 0, "Boar Meat"});
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
    GetRegistry().AddComponent(comp, PixelsEngine::TagComponent{PixelsEngine::EntityTag::NPC});
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

    // Tent (Camp)
    auto tent = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(tent, PixelsEngine::TransformComponent{7.0f, 5.0f});
    auto tentTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/camp_tent.png");
    GetRegistry().AddComponent(tent, PixelsEngine::SpriteComponent{tentTex, {0, 0, 64, 64}, 32, 48});
    GetRegistry().AddComponent(tent, PixelsEngine::TagComponent{PixelsEngine::EntityTag::CampProp});

    // Bedroll (Camp)
    auto bedroll = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(bedroll, PixelsEngine::TransformComponent{8.0f, 6.0f});
    auto bedrollTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/camp_bedroll.png");
    GetRegistry().AddComponent(bedroll, PixelsEngine::SpriteComponent{bedrollTex, {0, 0, 32, 32}, 16, 16});
    GetRegistry().AddComponent(bedroll, PixelsEngine::TagComponent{PixelsEngine::EntityTag::CampProp});
    GetRegistry().AddComponent(bedroll, PixelsEngine::InteractionComponent{"Rest", "camp_bedroll", false, 0.0f});

    // Fire (Camp)
    auto fire = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(fire, PixelsEngine::TransformComponent{7.5f, 7.5f});
    auto fireTex = PixelsEngine::TextureManager::LoadTexture(GetRenderer(), "assets/camp_fire_sheet.png");
    GetRegistry().AddComponent(fire, PixelsEngine::SpriteComponent{fireTex, {0, 0, 32, 32}, 16, 24});
    auto &fireAnim = GetRegistry().AddComponent(fire, PixelsEngine::AnimationComponent{});
    fireAnim.AddAnimation("Burn", 0, 0, 32, 32, 4, 0.15f);
    fireAnim.Play("Burn");
    GetRegistry().AddComponent(fire, PixelsEngine::LightComponent{5.0f, {255, 200, 100, 255}, true});
    GetRegistry().AddComponent(fire, PixelsEngine::TagComponent{PixelsEngine::EntityTag::CampProp});
    GetRegistry().AddComponent(fire, PixelsEngine::InteractionComponent{"Rest", "camp_fire", false, 0.0f});

    // Son's Camp Spot
    auto sTent = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(sTent, PixelsEngine::TransformComponent{11.0f, 4.0f});
    GetRegistry().AddComponent(sTent, PixelsEngine::SpriteComponent{tentTex, {0, 0, 64, 64}, 32, 48});
    GetRegistry().AddComponent(sTent, PixelsEngine::TagComponent{PixelsEngine::EntityTag::CampProp});

    auto sBed = GetRegistry().CreateEntity();
    GetRegistry().AddComponent(sBed, PixelsEngine::TransformComponent{11.0f, 5.0f});
    GetRegistry().AddComponent(sBed, PixelsEngine::SpriteComponent{bedrollTex, {0, 0, 32, 32}, 16, 16});
    GetRegistry().AddComponent(sBed, PixelsEngine::TagComponent{PixelsEngine::EntityTag::CampProp});
}