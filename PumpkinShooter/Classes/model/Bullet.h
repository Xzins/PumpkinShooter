#pragma once

#include "HitBox.h"
#include "RunRules.h"

#include <cstdint>

struct Bullet
{
    std::uint64_t id;
    float startX;
    float worldX;
    float worldY;
    float range;
    Weapon weapon;

    HitBox hitBox() const
    {
        const float halfSize = 2.0f;
        return {worldX - halfSize, worldY - halfSize, 2.0f * halfSize, 2.0f * halfSize};
    }
};
