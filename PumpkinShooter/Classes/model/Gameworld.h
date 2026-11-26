#pragma once

#include <deque>
#include <random>
#include <vector>

#include "Bullet.h"
#include "Player.h"
#include "Segment.h"

class Gameworld
{
public:
    void setDifficulty(Difficulty difficulty) { difficulty_ = difficulty; }
    Difficulty difficulty() const { return difficulty_; }
    int score() const { return score_; }
    const std::vector<Impact>& impacts() const { return impacts_; }
    Weapon weapon() const { return weapon_; }
    float weaponTime() const { return weaponTime_; }
    const std::deque<Pickup>& pickups() const { return pickups_; }
    bool tryPlayerJump();
    void update(float dt, float visibleWidth);
    float cameraX() const;
    void setMuzzleOffset(float x, float y);
    const std::deque<Bullet>& bullets() const { return bullets_; }
    static float playerViewOffset(float visibleWidth) { return visibleWidth * 0.22f; }
    float groundY() const { return groundY_; }

    HitBox playerHitBox(float visibleWidth) const
    {
        return player_.hitBox(cameraX_ + playerViewOffset(visibleWidth));
    }

    bool playerDefeated() const { return playerDefeated_; }
    const std::deque<SegmentInstance>& segments() const;
    const Player& player() const;

private:
    void updateSegments(float visibleWidth);
    void appendSegment(const SegmentDefinition& definition, bool spawnEnemy);

    void updateStep(float dt, float visibleWidth);
    void resolvePlatforms(const HitBox& previous, float visibleWidth);
    void resolveEnemies(const HitBox& previous, float visibleWidth);
    bool hitEnemy(Bullet& bullet, float endX);
    void updateShooting(float dt, float visibleWidth);

    void updatePickups(float dt, float visibleWidth);

    Player player_;
    Difficulty difficulty_ = Difficulty::Easy;
    Weapon weapon_ = Weapon::Pea;
    float weaponTime_ = 0.0f;
    float pickupCountdown_ = 65.0f;
    std::deque<Pickup> pickups_;
    int score_ = 0;
    std::vector<Impact> impacts_;
    unsigned int combatSegments_ = 0;
    std::deque<Bullet> bullets_;
    std::uint64_t nextBulletId_ = 0;
    float shotCooldown_ = 0.0f;
    float muzzleOffsetX_ = 0.0f;
    float muzzleOffsetY_ = 0.0f;
    std::deque<SegmentInstance> segments_;
    float cameraX_ = 0.0f;
    float nextSegmentX_ = 0.0f;
    std::mt19937 random_{std::random_device{}()};

    float groundY_ = 0.0f;
    bool playerDefeated_ = false;
};