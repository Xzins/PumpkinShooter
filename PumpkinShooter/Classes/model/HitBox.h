#pragma once

struct HitBox
{
    float left;
    float bottom;
    float width;
    float height;

    float right() const { return left + width; }
    float top() const { return bottom + height; }

    static HitBox fromBottomCenter(float x, float y, float width, float height)
    {
        return {x - width * 0.5f, y, width, height};
    }

    bool overlaps(const HitBox& other) const
    {
        return left < other.right() && right() > other.left
            && bottom < other.top() && top() > other.bottom;
    }
};
