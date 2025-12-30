#pragma once
#include <string>
#include <vector>

namespace PixelsEngine {

    enum class ItemType {
        Misc,
        Consumable,
        WeaponMelee,
        WeaponRanged,
        Armor
    };

    struct Item {
        std::string name;
        std::string iconPath; // For now we'll just list names
        int quantity = 1;
        ItemType type = ItemType::Misc;
        int statBonus = 0; // Damage for weapons, Defense for armor
        
        bool IsEmpty() const { return name.empty() || quantity <= 0; }
    };

    struct InventoryComponent {
        std::vector<Item> items;
        int capacity = 20;
        bool isOpen = false;

        // Equipment Slots
        Item equippedMelee = {"", "", 0, ItemType::WeaponMelee, 0};
        Item equippedRanged = {"", "", 0, ItemType::WeaponRanged, 0};
        Item equippedArmor = {"", "", 0, ItemType::Armor, 0};

        void AddItem(const std::string& name, int count = 1, ItemType type = ItemType::Misc, int bonus = 0, const std::string& iconPath = "") {
            for (auto& item : items) {
                if (item.name == name) {
                    item.quantity += count;
                    return;
                }
            }
            if (items.size() < capacity) {
                items.push_back({name, iconPath, count, type, bonus});
            }
        }
        
        void AddItemObject(const Item& newItem) {
            AddItem(newItem.name, newItem.quantity, newItem.type, newItem.statBonus, newItem.iconPath);
        }
    };

}
