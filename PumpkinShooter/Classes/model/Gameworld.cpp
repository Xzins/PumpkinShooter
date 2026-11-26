#include "Gameworld.h"
#include "Segment.h"
#include <algorithm>
#include <utility>

//segement 
namespace
{
    // Intersect the time intervals in which the moving box overlaps each axis.
    bool clipContactAxis(float low, float high, float delta, float targetLow,
                         float targetHigh, float& enter, float& leave)
    {
        if (delta == 0.0f)
        {
            return low < targetHigh && high > targetLow;
        }

        float first = (targetLow - high) / delta;
        float last = (targetHigh - low) / delta;
        if (first > last)
        {
            std::swap(first, last);
        }
        enter = std::max(enter, first);
        leave = std::min(leave, last);
        return enter < leave;
    }

    bool crossesEnemy(const HitBox& from, const HitBox& to, const HitBox& enemy)
    {
        float enter = 0.0f;
        float leave = 1.0f;
        return clipContactAxis(from.left, from.right(), to.left - from.left,
                   enemy.left, enemy.right(), enter, leave)
            && clipContactAxis(from.bottom, from.top(), to.bottom - from.bottom,
                   enemy.bottom, enemy.top(), enter, leave);
    }

    const SegmentDefinition openingSegment{600.0f, {}};

    const SegmentDefinition lowPlatformSegment{
        600.0f,
        {{140.0f, 24.0f, 100.0f, 10.0f}}
    };
    const SegmentDefinition staggeredPlatforms{
        600.0f, {{120, 24, 90, 12}, {315, 43, 105, 14}}
    };
    const SegmentDefinition descendingSteps{
        600.0f, {{120, 48, 85, 13}, {265, 35, 95, 14}, {425, 23, 105, 15}}
    };
    const SegmentDefinition shortCanopies{
        600.0f, {{115, 22, 155, 14}, {360, 24, 160, 14}}
    };
    const SegmentDefinition risingSteps{
        600.0f, {{100, 22, 95, 13}, {235, 40, 100, 14}, {385, 58, 130, 14}}
    };
    const SegmentDefinition longCanopy{
        600.0f, {{125, 23, 385, 16}}
    };
    const SegmentDefinition splitDeck{
        600.0f, {{110, 23, 200, 14}, {190, 59, 155, 14}, {390, 32, 120, 14}}
    };
    const SegmentDefinition alternatingHeights{
        600.0f, {{100, 23, 80, 12}, {215, 54, 95, 14},
                 {345, 23, 85, 12}, {450, 49, 85, 14}}
    };
    const SegmentDefinition bridgeAndSteps{
        600.0f, {{95, 22, 74, 12}, {210, 42, 206, 14}, {445, 23, 90, 13}}
    };

    template <std::size_t N>
    const SegmentDefinition& chooseSegment(
        const SegmentDefinition* const (&pool)[N], std::mt19937& random)
    {
        return *pool[std::uniform_int_distribution<std::size_t>(0, N - 1)(random)];
    }
}

void Gameworld::appendSegment(const SegmentDefinition& definition, bool spawnEnemy)
{
    segments_.push_back(SegmentInstance{&definition, nextSegmentX_, {}, {}});
    if (spawnEnemy)
    {
        const int level = static_cast<int>(difficulty_);
        const int count = level + 1;
        const int health = level + 1;
        auto& segment = segments_.back();
        std::uniform_int_distribution<int> typeRoll(0, 1);
        std::uniform_real_distribution<float> height(35.0f, 54.0f);
        std::uniform_real_distribution<float> jitter(-18.0f, 18.0f);
        for (int i = 0; i < count; ++i)
        {
            const auto type = typeRoll(random_) == 0 ? EnemyType::Skull : EnemyType::Ghost;
            const float x = 135.0f + (i + 1) * (420.0f / (count + 1)) + jitter(random_);
            float y = type == EnemyType::Skull ? groundY_ : height(random_);
            for (const auto& platform : definition.platforms)
            {
                const auto box = platform.hitBox(0.0f);
                if (x + 5.0f > box.left && x - 5.0f < box.right())
                {
                    if (type == EnemyType::Ghost)
                    {
                        y = std::max(y, box.top() + 12.0f);
                    }
                    else if (&definition != &longCanopy && (i + combatSegments_) % 2 == 0)
                    {
                        y = box.top();
                    }
                }
            }
            segment.enemies.emplace_back(type, nextSegmentX_ + x, y, health);
        }

        if (definition.platforms.empty())
        {
            segment.traps.emplace_back(nextSegmentX_ + 180.0f, 25.0f);
        }
        if (level > 0 && combatSegments_ % 3 != 1)
        {
            const bool airborne = combatSegments_ % 3 == 0;
            segment.traps.emplace_back(nextSegmentX_ + 100.0f, 13.0f,
                15.0f, airborne ? 39.0f : 0.0f, true);
        }
        ++combatSegments_;
    }
    nextSegmentX_ += definition.length;
}

void Gameworld::updateSegments(float visibleWidth)
{
    // put two segment instances first frame
    if (nextSegmentX_ == 0.0f)
    {
        appendSegment(openingSegment, false);
        appendSegment(lowPlatformSegment, false);
    }

    const float lookAhead = 160.0f;
    const float requiredEndX = cameraX_ + visibleWidth + lookAhead;

    while (nextSegmentX_ < requiredEndX)
    {
        const SegmentDefinition* easy[] = {&openingSegment, &lowPlatformSegment};
        const SegmentDefinition* normal[] = {
            &lowPlatformSegment, &staggeredPlatforms, &descendingSteps, &shortCanopies
        };
        const SegmentDefinition* hard[] = {
            &risingSteps, &longCanopy, &splitDeck, &alternatingHeights, &bridgeAndSteps
        };
        const auto& next = difficulty_ == Difficulty::Easy ? chooseSegment(easy, random_)
            : (difficulty_ == Difficulty::Normal ? chooseSegment(normal, random_)
                                                 : chooseSegment(hard, random_));
        appendSegment(next, true);
    }

    while (!segments_.empty())
    {
        const auto& first = segments_.front();
        const float firstEndX = first.startX + first.definition->length;

        if (firstEndX > cameraX_)
        {
            break;
        }

        segments_.pop_front();
    }
}

float Gameworld::cameraX() const
{
    return cameraX_;
}

const std::deque<SegmentInstance>& Gameworld::segments() const
{
    return segments_;
}


// palyer
bool Gameworld::tryPlayerJump()
{
    return !playerDefeated_ && player_.tryJump();
}

const Player& Gameworld::player() const
{
    return player_;
}


// update

void Gameworld::update(float dt, float visibleWidth)
{
    impacts_.clear();
    // Keep body movement small enough to resolve contacts at platform corners.
    while (dt > 0.0f && !playerDefeated_)
    {
        const float step = std::min(dt, 1.0f / 120.0f);
        updateStep(step, visibleWidth);
        dt -= step;
    }
}

void Gameworld::updateStep(float dt, float visibleWidth)
{
    const HitBox previous = playerHitBox(visibleWidth);

    player_.update(dt);
    cameraX_ += (120.0f + 15.0f * static_cast<int>(difficulty_)) * dt;
    updateSegments(visibleWidth);

    resolvePlatforms(previous, visibleWidth);
    if (player_.velocityY() <= 0.0f && player_.footY() <= groundY_)
    {
        player_.landAt(groundY_);
    }

    const HitBox current = playerHitBox(visibleWidth);
    for (const auto& segment : segments_)
    {
        for (const auto& trap : segment.traps)
        {
            const auto bounds = trap.hitBox(groundY_);
            if (trap.isRope() ? crossesEnemy(previous, current, bounds) : (current.bottom < bounds.top() && current.top() > bounds.bottom
                && current.right() > bounds.left
                && (previous.bottom < bounds.top() ? previous.left : current.left) < bounds.right()))
            {
                playerDefeated_ = true;
                return;
            }
        }
    }

    updatePickups(dt, visibleWidth);
    updateShooting(dt, visibleWidth);
    resolveEnemies(previous, visibleWidth);
    playerDefeated_ = player_.health() == 0;
}

void Gameworld::resolvePlatforms(const HitBox& previous, float visibleWidth)
{
    for (const auto& segment : segments_)
    {
        for (const auto& placement : segment.definition->platforms)
        {
            const auto platform = placement.hitBox(segment.startX);
            const auto current = playerHitBox(visibleWidth);
            if (current.right() <= platform.left || current.left >= platform.right())
            {
                continue;
            }

            if (player_.velocityY() <= 0.0f
                && previous.bottom >= platform.top() && current.bottom <= platform.top())
            {
                player_.landAt(platform.top());
            }
            else if (player_.velocityY() > 0.0f
                && previous.top() <= platform.bottom && current.top() >= platform.bottom
                && previous.right() > platform.left)
            {
                player_.stopAtCeiling(platform.bottom - current.height);
            }
            else if (previous.right() <= platform.left && current.overlaps(platform))
            {
                cameraX_ -= current.right() - platform.left;
            }
        }
    }
}

void Gameworld::resolveEnemies(const HitBox& previous, float visibleWidth)
{
    for (auto& segment : segments_)
    {
        for (auto& enemy : segment.enemies)
        {
            if (!enemy.isAlive())
            {
                continue;
            }

            const auto current = playerHitBox(visibleWidth);
            const auto target = enemy.hitBox();
            const float fall = previous.bottom - current.bottom;
            if (fall > 0.0f && previous.bottom >= target.top()
                && current.bottom <= target.top())
            {
                const float contactTime = (previous.bottom - target.top()) / fall;
                const float contactLeft = previous.left
                    + (current.left - previous.left) * contactTime;
                if (contactLeft < target.right() && contactLeft + current.width > target.left)
                {
                    impacts_.push_back({enemy.worldX(), target.top(), false});
                    enemy.stomp();
                    ++score_;
                    continue;
                }
            }

            if (current.overlaps(target) || crossesEnemy(previous, current, target))
            {
                player_.takeDamage();
            }
        }
    }
}

bool Gameworld::hitEnemy(Bullet& bullet, float endX)
{
    HitBox sweep = bullet.hitBox();
    sweep.width += endX - bullet.worldX;
    Enemy* nearest = nullptr;
    for (auto& segment : segments_)
    {
        for (auto& enemy : segment.enemies)
        {
            if (enemy.isAlive() && sweep.overlaps(enemy.hitBox())
                && (!nearest || enemy.hitBox().left < nearest->hitBox().left))
            {
                nearest = &enemy;
            }
        }
    }

    if (!nearest)
    {
        return false;
    }

    impacts_.push_back({nearest->worldX(), bullet.worldY, bullet.weapon == Weapon::Corn});
    if (bullet.weapon == Weapon::Corn)
    {
        const float blastX = nearest->worldX();
        const float blastY = nearest->bottomY();
        for (auto& segment : segments_)
        {
            for (auto& enemy : segment.enemies)
            {
                const float dx = enemy.worldX() - blastX;
                const float dy = enemy.bottomY() - blastY;
                if (enemy.isAlive() && dx * dx + dy * dy <= 42.0f * 42.0f)
                {
                    enemy.takeDamage(3);
                    if (!enemy.isAlive()) { ++score_; }
                }
            }
        }
    }
    else
    {
        nearest->takeDamage(1);
        if (!nearest->isAlive()) { ++score_; }
    }
    return true;
}

void Gameworld::setMuzzleOffset(float x, float y)
{
    muzzleOffsetX_ = x;
    muzzleOffsetY_ = y;
}

void Gameworld::updateShooting(float dt, float visibleWidth)
{
    const float bulletSpeed = 480.0f;
    const float peaInterval = 0.72f;
    const float shotInterval = weapon_ == Weapon::MachineGun ? peaInterval * 0.5f : peaInterval;

    for (auto it = bullets_.begin(); it != bullets_.end();)
    {
        const float rangeEnd = it->startX + it->range;
        const float endX = std::min(rangeEnd, it->worldX + bulletSpeed * dt);
        if (hitEnemy(*it, endX) || endX >= rangeEnd)
        {
            it = bullets_.erase(it);
        }
        else
        {
            it->worldX = endX;
            ++it;
        }
    }

    shotCooldown_ -= dt;
    if (shotCooldown_ <= 0.0f)
    {
        const float muzzleX = cameraX_ + playerViewOffset(visibleWidth) + muzzleOffsetX_;
        bullets_.push_back(Bullet{
            nextBulletId_++, muzzleX, muzzleX,
            player_.footY() + muzzleOffsetY_, visibleWidth * 0.5f, weapon_
        });
        shotCooldown_ += shotInterval;
    }
}

void Gameworld::updatePickups(float dt, float visibleWidth)
{
    weaponTime_ = std::max(0.0f, weaponTime_ - dt);
    if (weaponTime_ == 0.0f)
    {
        weapon_ = Weapon::Pea;
    }


    pickupCountdown_ -= dt;
    if (pickupCountdown_ <= 0.0f)
    {
        std::uniform_real_distribution<float> interval(65.0f, 85.0f);
        std::uniform_int_distribution<int> type(0, 1);
        pickups_.push_back({type(random_) == 0 ? Weapon::MachineGun : Weapon::Corn,
                            cameraX_ + visibleWidth * 0.80f, 23.0f});
        auto& pickup = pickups_.back();
        for (const auto& segment : segments_)
        {
            for (const auto& platform : segment.definition->platforms)
            {
                const auto box = platform.hitBox(segment.startX);
                if (HitBox::fromBottomCenter(pickup.x, pickup.y, 14.0f, 14.0f).overlaps(box))
                {
                    pickup.y = std::max(pickup.y, box.top() + 4.0f);
                }
            }
        }
        pickupCountdown_ = interval(random_);
    }

    const auto player = playerHitBox(visibleWidth);
    for (auto it = pickups_.begin(); it != pickups_.end();)
    {
        const HitBox area = HitBox::fromBottomCenter(it->x, it->y, 14.0f, 14.0f);
        if (player.overlaps(area))
        {
            weapon_ = it->weapon;
            weaponTime_ = 10.0f;
            shotCooldown_ = 0.0f;
            it = pickups_.erase(it);
        }
        else if (area.right() < cameraX_)
        {
            it = pickups_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
