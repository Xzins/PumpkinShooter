#pragma once

#include "../model/Gameworld.h"

class GameController
{
public:
    void setDifficulty(Difficulty difficulty) { world_.setDifficulty(difficulty); }

    void onJumpInput()
    {
        world_.tryPlayerJump();
    }

    void update(float dt, float visibleWidth)
    {
        world_.update(dt, visibleWidth);
    }

    void setMuzzleOffset(float x, float y)
    {
        world_.setMuzzleOffset(x, y);
    }

    const Gameworld& world() const
    {
        return world_;
    }

    float playerFootY() const
    {
        return world_.player().footY();
    }

private:
    Gameworld world_;
};