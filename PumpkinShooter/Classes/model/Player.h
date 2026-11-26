#pragma once

#include "HitBox.h"

class Player
{
public:
    bool tryJump();
    void update(float dt);
    void landAt(float surfaceY);

    HitBox hitBox(float worldX) const
    {
        return HitBox::fromBottomCenter(worldX, footY_, 11.0f, 16.0f);
    }

    void stopAtCeiling(float footY);
    int health() const { return health_; }
    void takeDamage();

    float footY() const;
    float velocityY() const;

private:
    float footY_ = 0.0f;
    float velocityY_ = 0.0f;
    int health_ = 3;
    float invulnerableTime_ = 0.0f;
    int jumpsRemaining_ = 2;
};