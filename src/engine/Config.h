#pragma once
#include <SDL2/SDL.h>
#include <unordered_map>
#include <string>

namespace PixelsEngine {

    enum class GameAction {
        MoveUp,
        MoveDown,
        MoveLeft,
        MoveRight,
        AttackModifier,
        Inventory,
        Map,
        Character,
        Magic,
        Pause,
        EndTurn
    };

    class Config {
    public:
        static void Init() {
            // Default Keybinds
            m_Keybinds[GameAction::MoveUp] = SDL_SCANCODE_W;
            m_Keybinds[GameAction::MoveDown] = SDL_SCANCODE_S;
            m_Keybinds[GameAction::MoveLeft] = SDL_SCANCODE_A;
            m_Keybinds[GameAction::MoveRight] = SDL_SCANCODE_D;
            m_Keybinds[GameAction::AttackModifier] = SDL_SCANCODE_LSHIFT; // Changed from CTRL to LSHIFT by default to avoid Mac conflict
            m_Keybinds[GameAction::Inventory] = SDL_SCANCODE_I;
            m_Keybinds[GameAction::Map] = SDL_SCANCODE_M;
            m_Keybinds[GameAction::Character] = SDL_SCANCODE_C;
            m_Keybinds[GameAction::Magic] = SDL_SCANCODE_K;
            m_Keybinds[GameAction::Pause] = SDL_SCANCODE_ESCAPE;
            m_Keybinds[GameAction::EndTurn] = SDL_SCANCODE_SPACE;
        }

        static SDL_Scancode GetKeybind(GameAction action) {
            return m_Keybinds[action];
        }

        static void SetKeybind(GameAction action, SDL_Scancode scancode) {
            m_Keybinds[action] = scancode;
        }

        static std::string GetActionName(GameAction action) {
            switch (action) {
                case GameAction::MoveUp: return "Move Up";
                case GameAction::MoveDown: return "Move Down";
                case GameAction::MoveLeft: return "Move Left";
                case GameAction::MoveRight: return "Move Right";
                case GameAction::AttackModifier: return "Attack (Hold + Click)";
                case GameAction::Inventory: return "Inventory";
                case GameAction::Map: return "Map";
                case GameAction::Character: return "Character";
                case GameAction::Magic: return "Magic";
                case GameAction::Pause: return "Pause/Back";
                case GameAction::EndTurn: return "End Turn";
                default: return "Unknown";
            }
        }

    private:
        static std::unordered_map<GameAction, SDL_Scancode> m_Keybinds;
    };

}
