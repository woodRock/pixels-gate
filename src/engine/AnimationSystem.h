#pragma once
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include "Components.h"

namespace PixelsEngine {

    struct Animation {
        std::string name;
        std::vector<SDL_Rect> frames;
        float frameDuration = 0.1f; // Seconds per frame
        bool loop = true;
    };

    struct AnimationComponent {
        std::vector<Animation> animations;
        int currentAnimationIndex = 0;
        int currentFrameIndex = 0;
        float timer = 0.0f;
        bool isPlaying = true;

        void AddAnimation(const std::string& name, int startX, int startY, int width, int height, int frameCount, float duration = 0.1f) {
            Animation anim;
            anim.name = name;
            anim.frameDuration = duration;
            for (int i = 0; i < frameCount; ++i) {
                anim.frames.push_back({ startX + (i * width), startY, width, height });
            }
            animations.push_back(anim);
        }

        void Play(const std::string& name) {
            for (size_t i = 0; i < animations.size(); ++i) {
                if (animations[i].name == name) {
                    if (currentAnimationIndex != i) {
                        currentAnimationIndex = i;
                        currentFrameIndex = 0;
                        timer = 0.0f;
                        isPlaying = true;
                    }
                    return;
                }
            }
        }
    };

}
