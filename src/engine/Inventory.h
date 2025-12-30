#pragma once
#include <string>
#include <vector>

namespace PixelsEngine {

    struct Item {
        std::string name;
        std::string iconPath; // For now we'll just list names
        int quantity = 1;
    };

    struct InventoryComponent {
        std::vector<Item> items;
        int capacity = 20;
        bool isOpen = false;

        void AddItem(const std::string& name, int count = 1) {
            for (auto& item : items) {
                if (item.name == name) {
                    item.quantity += count;
                    return;
                }
            }
            if (items.size() < capacity) {
                items.push_back({name, "", count});
            }
        }
    };

}
