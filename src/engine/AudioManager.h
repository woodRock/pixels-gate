#pragma once
#include <SDL2/SDL_mixer.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace PixelsEngine {

class AudioManager {
public:
    static bool Init();
    static void Quit();

    static void PlaySound(const std::string& path);
    static void PlayMusic(const std::string& path, int loops = -1);
    static void StopMusic();
    static void SetMusicVolume(int volume); // 0 to 128
    static void SetSoundVolume(int volume); // 0 to 128

private:
    static Mix_Chunk* LoadSound(const std::string& path);
    static Mix_Music* LoadMusic(const std::string& path);

    static std::unordered_map<std::string, Mix_Chunk*> m_Sounds;
    static std::unordered_map<std::string, Mix_Music*> m_Music;
    static int m_SoundVolume;
};

} // namespace PixelsEngine
