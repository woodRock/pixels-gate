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
        int value = 10;    // Gold value per unit
        
        bool IsEmpty() const { return name.empty() || quantity <= 0; }
    };

    struct InventoryComponent {
        std::vector<Item> items;
        int capacity = 20;
        bool isOpen = false;

        // Equipment Slots
        Item equippedMelee = {"", "", 0, ItemType::WeaponMelee, 0, 0};
        Item equippedRanged = {"", "", 0, ItemType::WeaponRanged, 0, 0};
        Item equippedArmor = {"", "", 0, ItemType::Armor, 0, 0};

        void AddItem(const std::string& name, int count = 1, ItemType type = ItemType::Misc, int bonus = 0, const std::string& iconPath = "", int val = 10) {
            for (auto it = items.begin(); it != items.end(); ++it) {
                if (it->name == name) {
                    it->quantity += count;
                    if (it->quantity <= 0) {
                        items.erase(it);
                    }
                    return;
                }
            }
            if (count > 0 && items.size() < capacity) {
                items.push_back({name, iconPath, count, type, bonus, val});
            }
        }
        
        void AddItemObject(const Item& newItem) {
            AddItem(newItem.name, newItem.quantity, newItem.type, newItem.statBonus, newItem.iconPath, newItem.value);
        }
    };

}
