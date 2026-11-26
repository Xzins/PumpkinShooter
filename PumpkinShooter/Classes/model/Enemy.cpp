#include "Enemy.h"

#include <algorithm>

Enemy::Enemy(EnemyType type, float worldX, float bottomY, int health)
    : type_(type), worldX_(worldX), bottomY_(bottomY), health_(std::max(0, health))
{
}

EnemyType Enemy::type() const
{
    return type_;
}

float Enemy::worldX() const
{
    return worldX_;
}

float Enemy::bottomY() const
{
    return bottomY_;
}

int Enemy::health() const
{
    return health_;
}

bool Enemy::isAlive() const
{
    return health_ > 0;
}

void Enemy::takeDamage(int damage)
{
    if (damage > 0)
    {
        health_ = std::max(0, health_ - damage);
    }
}

void Enemy::stomp()
{
    health_ = 0;
}
