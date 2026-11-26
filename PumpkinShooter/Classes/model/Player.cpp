#include "Player.h"

#include <algorithm>

bool Player::tryJump()
{
    if (jumpsRemaining_ == 0)
    {
        return false;
    }

    --jumpsRemaining_;
    velocityY_ = 420.0f;
    return true;
}

void Player::update(float dt)
{
    invulnerableTime_ = std::max(0.0f, invulnerableTime_ - dt);
    velocityY_ -= 1800.0f * dt;
    velocityY_ = std::max(velocityY_, -600.0f);
    footY_ += velocityY_ * dt;
}

void Player::landAt(float surfaceY)
{
    footY_ = surfaceY;
    velocityY_ = 0.0f;
    jumpsRemaining_ = 2;
}

float Player::footY() const
{
    return footY_;
}   

float Player::velocityY() const
{
    return velocityY_;
}
void Player::stopAtCeiling(float footY)
{
    footY_ = footY;
    velocityY_ = 0.0f;
}

void Player::takeDamage()
{
    if (invulnerableTime_ == 0.0f && health_ > 0)
    {
        --health_;
        invulnerableTime_ = 0.8f;
    }
}
