#include "AudioManager.h"
#include <iostream>

namespace PixelsEngine {

std::unordered_map<std::string, Mix_Chunk*> AudioManager::m_Sounds;
std::unordered_map<std::string, Mix_Music*> AudioManager::m_Music;
int AudioManager::m_SoundVolume = MIX_MAX_VOLUME;

bool AudioManager::Init() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }
    return true;
}

void AudioManager::Quit() {
    for (auto& pair : m_Sounds) {
        Mix_FreeChunk(pair.second);
    }
    m_Sounds.clear();

    for (auto& pair : m_Music) {
        Mix_FreeMusic(pair.second);
    }
    m_Music.clear();

    Mix_Quit();
    Mix_CloseAudio();
}

Mix_Chunk* AudioManager::LoadSound(const std::string& path) {
    if (m_Sounds.find(path) != m_Sounds.end()) {
        return m_Sounds[path];
    }

    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        std::cerr << "Failed to load sound effect! SDL_mixer Error: " << Mix_GetError() << " at path: " << path << std::endl;
        return nullptr;
    }

    m_Sounds[path] = chunk;
    return chunk;
}

Mix_Music* AudioManager::LoadMusic(const std::string& path) {
    if (m_Music.find(path) != m_Music.end()) {
        return m_Music[path];
    }

    Mix_Music* music = Mix_LoadMUS(path.c_str());
    if (!music) {
        std::cerr << "Failed to load music! SDL_mixer Error: " << Mix_GetError() << " at path: " << path << std::endl;
        return nullptr;
    }

    m_Music[path] = music;
    return music;
}

void AudioManager::PlaySound(const std::string& path) {
    Mix_Chunk* chunk = LoadSound(path);
    if (chunk) {
        Mix_VolumeChunk(chunk, m_SoundVolume);
        Mix_PlayChannel(-1, chunk, 0);
    }
}

void AudioManager::PlayMusic(const std::string& path, int loops) {
    Mix_Music* music = LoadMusic(path);
    if (music) {
        Mix_PlayMusic(music, loops);
    }
}

void AudioManager::StopMusic() {
    Mix_HaltMusic();
}

void AudioManager::SetMusicVolume(int volume) {
    Mix_VolumeMusic(volume);
}

void AudioManager::SetSoundVolume(int volume) {
    m_SoundVolume = volume;
}

} // namespace PixelsEngine
