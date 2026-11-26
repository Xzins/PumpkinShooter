#pragma once

#include "HitBox.h"

class Trap
{
public:
    Trap(float leftX, float width, float height = 3.0f, float elevation = 0.0f, bool rope = false)
        : leftX_(leftX), width_(width), height_(height), elevation_(elevation), rope_(rope) {}
    bool isRope() const { return rope_; }

    float leftX() const { return leftX_; }
    float width() const { return width_; }

    HitBox hitBox(float groundY) const { return {leftX_, groundY + elevation_, width_, height_}; }

    bool crosses(float previousFootX, float footX) const
    {
        return footX >= leftX_ && previousFootX <= leftX_ + width_;
    }

private:
    float leftX_;
    float width_;
    float height_;
    float elevation_;
    bool rope_;
};
