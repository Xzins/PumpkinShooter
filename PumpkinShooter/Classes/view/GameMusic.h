#pragma once
#include "cocos2d.h"
#include "audio/include/AudioEngine.h"

inline void refreshMusic(const char* path)
{
    static int track = -1;
    if (track != -1)
    {
        cocos2d::AudioEngine::stop(track);
        track = -1;
    }
    if (cocos2d::UserDefault::getInstance()->getBoolForKey("bgm_enabled", true))
    {
        track = cocos2d::AudioEngine::play2d(path, true, 0.55f);
    }
}
