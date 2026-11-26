#pragma once

#include "HitBox.h"

enum class EnemyType
{
    Skull,
    Ghost
};

class Enemy
{
public:
    Enemy(EnemyType type, float worldX, float bottomY, int health);

    EnemyType type() const;
    float worldX() const;
    float bottomY() const;
    int health() const;
    bool isAlive() const;

    HitBox hitBox() const
    {
        const float height = type_ == EnemyType::Skull ? 16.0f : 10.0f;
        return HitBox::fromBottomCenter(worldX_, bottomY_, 9.0f, height);
    }

    void takeDamage(int damage);
    void stomp();

private:
    EnemyType type_;
    float worldX_;
    float bottomY_;
    int health_;
};
