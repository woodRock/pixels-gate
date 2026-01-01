#include "../src/game/GameComponents.h"
#include <cassert>
#include <iostream>
#include <cmath>

void TestTimeManager() {
    TimeManager tm;
    tm.m_TimeOfDay = 10.0f;
    tm.Update(1.0f); // Speed is 0.1, so +0.1
    if (std::abs(tm.m_TimeOfDay - 10.1f) > 0.0001f) {
        std::cerr << "TimeManager Update failed: Expected 10.1, got " << tm.m_TimeOfDay << std::endl;
        exit(1);
    }
    
    tm.m_TimeOfDay = 23.95f;
    tm.Update(1.0f); // +0.1 -> 24.05 -> wrapped to 0.05
    if (std::abs(tm.m_TimeOfDay - 0.05f) > 0.0001f) {
        std::cerr << "TimeManager Wrap failed: Expected 0.05, got " << tm.m_TimeOfDay << std::endl;
        exit(1);
    }
    std::cout << "TimeManager Test Passed" << std::endl;
}

void TestFloatingTextManager() {
    FloatingTextManager ftm;
    ftm.Spawn(10, 10, "Test", {255, 255, 255, 255});
    if (ftm.m_Texts.size() != 1) {
        std::cerr << "FloatingText Spawn failed" << std::endl;
        exit(1);
    }
    
    ftm.Update(0.5f);
    if (ftm.m_Texts[0].life >= 1.0f) {
         std::cerr << "FloatingText Life update failed" << std::endl;
         exit(1);
    }
    
    ftm.Update(2.0f); // Should expire
    if (!ftm.m_Texts.empty()) {
        std::cerr << "FloatingText Expiration failed" << std::endl;
        exit(1);
    }
    std::cout << "FloatingTextManager Test Passed" << std::endl;
}

void TestCombatManager() {
    CombatManager cm;
    if (cm.IsCombatActive()) {
        std::cerr << "CombatManager initially active?" << std::endl;
        exit(1);
    }
    
    // Create Dummy Entity
    PixelsEngine::Entity p = 1;
    PixelsEngine::Entity e = 2;
    
    cm.m_TurnOrder.push_back({p, 20, true});
    cm.m_TurnOrder.push_back({e, 10, false});
    
    if (!cm.IsCombatActive()) {
        std::cerr << "CombatManager not active after adding turns" << std::endl;
        exit(1);
    }
    
    cm.m_CurrentTurnIndex = 0;
    if (!cm.IsPlayerTurn()) {
        std::cerr << "CombatManager Player Turn check failed" << std::endl;
        exit(1);
    }
    
    cm.m_CurrentTurnIndex = 1;
    if (cm.IsPlayerTurn()) {
        std::cerr << "CombatManager Enemy Turn check failed" << std::endl;
        exit(1);
    }
    
    cm.Reset();
    if (cm.IsCombatActive()) {
        std::cerr << "CombatManager Reset failed" << std::endl;
        exit(1);
    }
    std::cout << "CombatManager Test Passed" << std::endl;
}

int main() {
    TestTimeManager();
    TestFloatingTextManager();
    TestCombatManager();
    return 0;
}
