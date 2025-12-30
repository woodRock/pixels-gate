#include "Input.h"
#include <cstring>

namespace PixelsEngine {

    const Uint8* Input::m_KeyboardState = nullptr;
    Uint8 Input::m_PrevKeyboardState[SDL_NUM_SCANCODES] = {0};
    Uint32 Input::m_MouseState = 0;
    int Input::m_MouseX = 0;
    int Input::m_MouseY = 0;

    void Input::Update() {
        if (!m_KeyboardState) {
            m_KeyboardState = SDL_GetKeyboardState(NULL);
        }
        
        // Copy current state (which is actually the state from the END of the previous frame due to SDL_PollEvent timing)
        // into our "Previous" buffer.
        // Then, later in the loop, SDL_PollEvent will update the internal m_KeyboardState array with new data for this frame.
        memcpy(m_PrevKeyboardState, m_KeyboardState, SDL_NUM_SCANCODES);
        
        m_MouseState = SDL_GetMouseState(&m_MouseX, &m_MouseY);
    }

    bool Input::IsKeyDown(SDL_Scancode key) {
        if (!m_KeyboardState) return false;
        return m_KeyboardState[key];
    }

    bool Input::IsKeyPressed(SDL_Scancode key) {
        if (!m_KeyboardState) return false;
        return m_KeyboardState[key] && !m_PrevKeyboardState[key];
    }

    bool Input::IsKeyReleased(SDL_Scancode key) {
        if (!m_KeyboardState) return false;
        return !m_KeyboardState[key] && m_PrevKeyboardState[key];
    }

    bool Input::IsMouseButtonDown(int button) {
        return (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(button));
    }

    void Input::GetMousePosition(int& x, int& y) {
        SDL_GetMouseState(&x, &y);
    }

}