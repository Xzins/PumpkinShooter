#pragma once

#include "HitBox.h"
#include <vector>
#include "Enemy.h"
#include "Trap.h"

struct PlatformPlacement
{
    float x;
    float bottomY;
    float width;
    float height;

    HitBox hitBox(float segmentStartX) const
    {
        // Keep the solid box inside the visible grass and soil of the platform PNG.
        const float sideInset = width * 0.08f;
        const float solidBottom = bottomY + height * 0.29f;
        const float solidHeight = height * (0.63f - 0.29f);
        return {segmentStartX + x + sideInset, solidBottom,
                width - 2.0f * sideInset, solidHeight};
    }
};

struct SegmentDefinition
{
    float length;
    std::vector<PlatformPlacement> platforms;
};

struct SegmentInstance
{
    const SegmentDefinition* definition;
    float startX;
    std::vector<Enemy> enemies;
    std::vector<Trap> traps;
};