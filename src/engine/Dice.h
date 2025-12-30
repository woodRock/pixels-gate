#pragma once
#include <cstdlib>
#include <ctime>
#include <iostream>

namespace PixelsEngine {

    class Dice {
    public:
        static void Init() {
            std::srand(static_cast<unsigned int>(std::time(nullptr)));
        }

        static int Roll(int sides, int count = 1) {
            int total = 0;
            for (int i = 0; i < count; ++i) {
                total += (std::rand() % sides) + 1;
            }
            return total;
        }

        static bool Check(int modifier, int dc, bool& criticalSuccess, bool& criticalFail) {
            int roll = Roll(20);
            criticalSuccess = (roll == 20);
            criticalFail = (roll == 1);
            
            std::cout << "Rolled d20: " << roll << " + Mod: " << modifier << " vs DC: " << dc << std::endl;
            
            if (criticalSuccess) return true;
            if (criticalFail) return false;
            
            return (roll + modifier) >= dc;
        }
    };

}
