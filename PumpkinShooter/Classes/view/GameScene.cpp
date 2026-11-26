#include "GameScene.h"
#include "TitleScene.h"
#include "GameMusic.h"
#include "ui/CocosGUI.h"

#include <algorithm>
#include <cmath>

bool GameScene::init()
{
    if (!cocos2d::Scene::init())
    {
        return false;
    }

    auto* background = cocos2d::Sprite::create("gameplay/field_sky.png");
    hero_ = cocos2d::Sprite::create("gameplay/hero_body.png");
    auto* peaMouth = cocos2d::Sprite::create("gameplay/hero_mouth_pea.png");

    if (!background || !hero_ || !peaMouth)
    {
        CCLOG("GameScene: failed to load gameplay sprites");
        return false;
    }

    auto* director = cocos2d::Director::getInstance();
    const auto visibleSize = director->getVisibleSize();
    const auto origin = director->getVisibleOrigin();

    const auto backgroundSize = background->getContentSize();
    const float backgroundScale = std::max(
        visibleSize.width / backgroundSize.width,
        visibleSize.height / backgroundSize.height
    );

    background->setScale(backgroundScale);
    background->setPosition(
        origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height * 0.5f
    );
    addChild(background, -2);

    if (!createClouds(visibleSize, origin.y))
    {
        return false;
    }
    syncClouds(origin.x, visibleSize.width, controller_.world().cameraX());

    const float grassHeightRatio = 0.33f;
    groundViewY_ = background->getPositionY()
        + (grassHeightRatio - 0.5f) * backgroundSize.height * backgroundScale;

    if (!createGround(visibleSize.width, origin.y))
    {
        return false;
    }
    syncGround(origin.x, controller_.world().cameraX());

    hero_->setAnchorPoint(cocos2d::Vec2(0.5f, 0.0f));
    hero_->setScale(0.65f);
    hero_->setPosition(
        origin.x + Gameworld::playerViewOffset(visibleSize.width),
        groundViewY_ + controller_.playerFootY()
    );

    peaMouth->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
    const auto heroSize = hero_->getContentSize();
    // The mouth mount is at (52, 43) in the 85 x 116 body artwork.
    peaMouth->setPosition(
        heroSize.width * (52.0f / 85.0f),
        heroSize.height * (43.0f / 116.0f)
    );
    peaMouth_ = peaMouth;
    mouthRestPosition_ = peaMouth->getPosition();
    const auto mouthSize = peaMouth->getContentSize();
    controller_.setMuzzleOffset(
        (mouthRestPosition_.x + mouthSize.width - heroSize.width * 0.5f) * hero_->getScale(),
        mouthRestPosition_.y * hero_->getScale()
    );
    hero_->addChild(peaMouth, 1);
    addChild(hero_, 1);

    for (const char* path : {"gameplay/pea_shot_1.png", "gameplay/pea_shot_2.png", "gameplay/pea_shot_3.png"})
    {
        auto* frameSprite = cocos2d::Sprite::create(path);
        if (!frameSprite)
        {
            CCLOG("GameScene: failed to load %s", path);
            return false;
        }
        shotFrames_.pushBack(frameSprite->getSpriteFrame());
    }

    for (const char* path : {"gameplay/corn_shot_1.png", "gameplay/corn_shot_2.png", "gameplay/corn_shot_3.png"})
    {
        auto* frameSprite = cocos2d::Sprite::create(path);
        if (!frameSprite) { return false; }
        cornFrames_.pushBack(frameSprite->getSpriteFrame());
    }

    pauseIcon_ = cocos2d::Sprite::create("gameplay/pause.png");
    hearts_ = cocos2d::DrawNode::create();
    scoreLabel_ = cocos2d::Label::createWithSystemFont("Score: 0", "Arial", 16);
    if (!pauseIcon_ || !hearts_ || !scoreLabel_) { return false; }
    pauseIcon_->setScale(26.0f / pauseIcon_->getContentSize().width);
    pauseIcon_->setPosition(origin.x + visibleSize.width - 22.0f, origin.y + visibleSize.height - 22.0f);
    scoreLabel_->setPosition(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 17.0f);
    hearts_->setPosition(origin.x + 12.0f, origin.y + visibleSize.height - 19.0f);
    addChild(pauseIcon_, 4);
    addChild(scoreLabel_, 4);
    addChild(hearts_, 4);
    updateHud();

    auto* touchListener = cocos2d::EventListenerTouchOneByOne::create();
    touchListener->onTouchBegan = [this](cocos2d::Touch* touch, cocos2d::Event*)
    {
        if (controller_.world().playerDefeated())
        {
            auto* scene = TitleScene::create();
            if (scene)
            {
                cocos2d::Director::getInstance()->replaceScene(scene);
            }
            return true;
        }

        if (paused_) { return false; }
        if (pauseIcon_->getBoundingBox().containsPoint(touch->getLocation()))
        {
            showPause();
            return true;
        }

        controller_.onJumpInput();
        return true;
    };

    getEventDispatcher()->addEventListenerWithSceneGraphPriority(
        touchListener, this
    );

#ifndef NDEBUG
    hitBoxOverlay_ = cocos2d::DrawNode::create();
    if (hitBoxOverlay_)
    {
        addChild(hitBoxOverlay_, 3);
        hitBoxOverlay_->setVisible(false);
        drawHitBoxes(origin.x, visibleSize.width);
    }
#endif
    scheduleUpdate();
    return true;
}

void GameScene::update(float dt)
{
    auto* director = cocos2d::Director::getInstance();
    if (paused_) { return; }
    controller_.update(dt, director->getVisibleSize().width);
    const auto& world = controller_.world();
    if (displayedWeapon_ != world.weapon())
    {
        displayedWeapon_ = world.weapon();
        const char* mouth = displayedWeapon_ == Weapon::Corn ? "gameplay/hero_mouth_corn.png"
            : (displayedWeapon_ == Weapon::MachineGun ? "gameplay/hero_mouth_machinegun.png"
                                                     : "gameplay/hero_mouth_pea.png");
        peaMouth_->setTexture(mouth);
        controller_.setMuzzleOffset(
            (mouthRestPosition_.x + peaMouth_->getContentSize().width
                - hero_->getContentSize().width * 0.5f) * hero_->getScale(),
            mouthRestPosition_.y * hero_->getScale()
        );
    }
    peaMouth_->setOpacity(world.weaponTime() > 0.0f && world.weaponTime() < 2.0f
        && static_cast<int>(world.weaponTime() * 8.0f) % 2 == 0 ? 90 : 255);
    const float visibleLeft = director->getVisibleOrigin().x;
    for (const auto& impact : world.impacts())
    {
        auto* spark = cocos2d::DrawNode::create();
        if (!spark) { continue; }
        const float radius = impact.blast ? 20.0f : 3.0f;
        const auto color = impact.blast ? cocos2d::Color4F(1.0f, 0.69f, 0.12f, 1.0f)
                                        : cocos2d::Color4F(0.87f, 1.0f, 0.62f, 1.0f);
        spark->drawCircle(cocos2d::Vec2::ZERO, radius, 0.0f, 18, false, color);
        spark->setPosition(visibleLeft + impact.x - world.cameraX(), groundViewY_ + impact.y);
        addChild(spark, 2);
        spark->runAction(cocos2d::Sequence::create(
            cocos2d::ScaleTo::create(0.16f, 2.0f), cocos2d::RemoveSelf::create(), nullptr
        ));
    }
    syncPickups(visibleLeft);
    syncClouds(
        director->getVisibleOrigin().x,
        director->getVisibleSize().width,
        controller_.world().cameraX()
    );
    syncGround(director->getVisibleOrigin().x, controller_.world().cameraX());
    syncPlatforms(director->getVisibleOrigin().x);
    updateHud();
    syncEnemies(director->getVisibleOrigin().x);
    syncTraps(director->getVisibleOrigin().x);
    syncBullets(director->getVisibleOrigin().x);
    hero_->setPositionY(groundViewY_ + controller_.playerFootY());
#ifndef NDEBUG
    drawHitBoxes(director->getVisibleOrigin().x, director->getVisibleSize().width);
#endif
    if (controller_.world().playerDefeated())
    {
        saveBestScore();
        unscheduleUpdate();
        hero_->setColor(cocos2d::Color3B(135, 135, 135));
        auto* label = cocos2d::Label::createWithSystemFont(
            "Game Over  |  Score: " + std::to_string(world.score()) + "\nTap to return", "Arial", 22
        );
        if (label)
        {
            const auto origin = director->getVisibleOrigin();
            const auto size = director->getVisibleSize();
            label->setPosition(origin.x + size.width * 0.5f, origin.y + size.height * 0.55f);
            addChild(label, 2);
        }
    }
}
void GameScene::syncPlatforms(float visibleLeft)
{
    const auto& world = controller_.world();
    const auto& segments = world.segments();
    const float cameraX = world.cameraX();

    while (!platformSprites_.empty()
        && (segments.empty() || platformSprites_.begin()->first < segments.front().startX))
    {
        auto oldest = platformSprites_.begin();
        for (auto* sprite : oldest->second)
        {
            if (sprite)
            {
                sprite->removeFromParent();
            }
        }
        platformSprites_.erase(oldest);
    }

    for (const auto& segment : segments)
    {
        const auto& placements = segment.definition->platforms;
        if (placements.empty())
        {
            continue;
        }

        auto& sprites = platformSprites_[segment.startX];
        if (sprites.empty())
        {
            for (const auto& placement : placements)
            {
                auto* sprite = cocos2d::Sprite::create("gameplay/grass_platform.png");
                sprites.push_back(sprite);
                if (!sprite)
                {
                    CCLOG("GameScene: failed to load grass_platform.png");
                    continue;
                }

                const auto size = sprite->getContentSize();
                sprite->setAnchorPoint(cocos2d::Vec2::ZERO);
                sprite->setScaleX(placement.width / size.width);
                sprite->setScaleY(placement.height / size.height);
                addChild(sprite, 1);
            }
        }

        for (std::size_t i = 0; i < sprites.size(); ++i)
        {
            if (sprites[i])
            {
                const auto& placement = placements[i];
                sprites[i]->setPosition(
                    visibleLeft + segment.startX + placement.x - cameraX,
                    groundViewY_ + placement.bottomY
                );
            }
        }
    }
}

bool GameScene::createGround(float visibleWidth, float visibleBottom)
{
    for (auto& tile : groundTiles_)
    {
        tile.grass = cocos2d::Sprite::create("gameplay/field_grass.png");
        tile.dirt = cocos2d::Sprite::create("gameplay/field_dirt.png");
        if (!tile.grass || !tile.dirt)
        {
            CCLOG("GameScene: failed to load scrolling ground");
            return false;
        }
    }

    const auto grassSize = groundTiles_[0].grass->getContentSize();
    const auto dirtSize = groundTiles_[0].dirt->getContentSize();
    const float overlap = 14.0f;
    const float grassTopY = groundViewY_ + 5.0f;
    // Cover the screen both across and below the grass line.
    const float scale = std::max(
        visibleWidth / grassSize.width,
        (grassTopY - visibleBottom + overlap) / (grassSize.height + dirtSize.height)
    );

    groundTileWidth_ = grassSize.width * scale;
    const float dirtTopY = grassTopY - grassSize.height * scale + overlap;

    for (auto& tile : groundTiles_)
    {
        // Use the top-left corner to stack the layers and join adjacent tiles.
        tile.grass->setAnchorPoint(cocos2d::Vec2(0.0f, 1.0f));
        tile.dirt->setAnchorPoint(cocos2d::Vec2(0.0f, 1.0f));
        tile.grass->setScale(scale);
        tile.dirt->setScale(scale);
        tile.grass->setPositionY(grassTopY);
        tile.dirt->setPositionY(dirtTopY);
        addChild(tile.dirt, 0);
        addChild(tile.grass, 0);
    }

    return true;
}

void GameScene::syncGround(float visibleLeft, float cameraX)
{
    // Keep only the position within a two-tile cycle.
    const float offset = std::fmod(cameraX, 2.0f * groundTileWidth_);
    const bool firstTileFlipped = offset >= groundTileWidth_;
    const float offsetInTile = firstTileFlipped ? offset - groundTileWidth_ : offset;

    for (std::size_t i = 0; i < groundTiles_.size(); ++i)
    {
        auto& tile = groundTiles_[i];
        const float x = visibleLeft - offsetInTile + static_cast<float>(i) * groundTileWidth_;
        const bool flipped = i == 0 ? firstTileFlipped : !firstTileFlipped;

        // Adjacent tiles meet at the same edge of the source image.
        tile.grass->setPositionX(x);
        tile.dirt->setPositionX(x);
        tile.grass->setFlippedX(flipped);
        tile.dirt->setFlippedX(flipped);
    }
}

bool GameScene::createClouds(const cocos2d::Size& visibleSize, float visibleBottom)
{
    const char* paths[] = {
        "gameplay/cloud_1.png", "gameplay/cloud_2.png",
        "gameplay/cloud_3.png", "gameplay/cloud_4.png"
    };
    const float heights[] = {0.87f, 0.69f, 0.82f, 0.73f};
    const float widths[] = {0.14f, 0.18f, 0.23f, 0.13f};

    for (std::size_t i = 0; i < cloudSprites_.size(); ++i)
    {
        auto* cloud = cocos2d::Sprite::create(paths[i]);
        if (!cloud)
        {
            CCLOG("GameScene: failed to load %s", paths[i]);
            return false;
        }

        cloud->setScale(visibleSize.width * widths[i] / cloud->getContentSize().width);
        cloud->setPositionY(visibleBottom + visibleSize.height * heights[i]);
        addChild(cloud, -1);
        cloudSprites_[i] = cloud;
    }

    return true;
}

void GameScene::syncClouds(float visibleLeft, float visibleWidth, float cameraX)
{
    const float parallax = 0.72f;
    const float margin = visibleWidth * 0.13f;
    const float span = visibleWidth + 2.0f * margin;
    const float travel = std::fmod(cameraX * parallax, span);

    for (std::size_t i = 0; i < cloudSprites_.size(); ++i)
    {
        const float initialX = visibleWidth * (0.12f + 0.24f * static_cast<float>(i));
        // Wrap with both endpoints outside the visible area.
        const float offset = std::fmod(initialX + margin + span - travel, span);
        cloudSprites_[i]->setPositionX(visibleLeft - margin + offset);
    }
}

void GameScene::syncEnemies(float visibleLeft)
{
    const auto& world = controller_.world();
    const auto& segments = world.segments();

    while (!enemySprites_.empty()
        && (segments.empty() || enemySprites_.begin()->first < segments.front().startX))
    {
        auto oldest = enemySprites_.begin();
        oldest->second->removeFromParent();
        enemySprites_.erase(oldest);
    }

    for (const auto& segment : segments)
    {
        for (const auto& enemy : segment.enemies)
        {
            const auto existing = enemySprites_.find(enemy.worldX());
            if (!enemy.isAlive())
            {
                if (existing != enemySprites_.end())
                {
                    existing->second->removeFromParent();
                    enemySprites_.erase(existing);
                }
                continue;
            }

            auto* sprite = existing == enemySprites_.end() ? nullptr : existing->second;
            if (!sprite)
            {
                const char* path = enemy.type() == EnemyType::Skull
                    ? "gameplay/enemy_skull.png" : "gameplay/enemy_ghost.png";
                sprite = cocos2d::Sprite::create(path);
                if (!sprite) { continue; }
                sprite->setAnchorPoint(cocos2d::Vec2(0.5f, 0.0f));
                sprite->setScale(0.65f);
                addChild(sprite, 1);
                enemySprites_.emplace(enemy.worldX(), sprite);
            }

            sprite->setPosition(visibleLeft + enemy.worldX() - world.cameraX(),
                                groundViewY_ + enemy.bottomY());
        }
    }
}

void GameScene::syncTraps(float visibleLeft)
{
    const auto& world = controller_.world();
    const auto& segments = world.segments();
    while (!trapSprites_.empty()
        && (segments.empty() || trapSprites_.begin()->first < segments.front().startX))
    {
        auto oldest = trapSprites_.begin();
        oldest->second->removeFromParent();
        trapSprites_.erase(oldest);
    }

    for (const auto& segment : segments)
    {
        for (const auto& trap : segment.traps)
        {
            auto found = trapSprites_.find(trap.leftX());
            if (found == trapSprites_.end())
            {
                auto* sprite = cocos2d::Sprite::create(trap.isRope() ? "gameplay/trap_rope.png" : "gameplay/trap_rake.png");
                if (!sprite)
                {
                    CCLOG("GameScene: failed to load gameplay/trap_rake.png");
                    continue;
                }

                sprite->setAnchorPoint(cocos2d::Vec2::ZERO);
                sprite->setScale(trap.width() / sprite->getContentSize().width);
                addChild(sprite, 1);
                found = trapSprites_.emplace(trap.leftX(), sprite).first;
            }

            found->second->setPosition(visibleLeft + trap.leftX() - world.cameraX(), groundViewY_ + trap.hitBox(world.groundY()).bottom);
        }
    }
}

void GameScene::syncBullets(float visibleLeft)
{
    const auto& world = controller_.world();
    const auto& bullets = world.bullets();
    for (auto sprite = bulletSprites_.begin(); sprite != bulletSprites_.end();)
    {
        const auto bullet = std::find_if(bullets.begin(), bullets.end(),
            [&](const Bullet& candidate) { return candidate.id == sprite->first; });
        if (bullet == bullets.end())
        {
            sprite->second->removeFromParent();
            sprite = bulletSprites_.erase(sprite);
        }
        else
        {
            ++sprite;
        }
    }

    for (const auto& bullet : bullets)
    {
        auto found = bulletSprites_.find(bullet.id);
        if (found == bulletSprites_.end())
        {
            const auto& frames = bullet.weapon == Weapon::Corn ? cornFrames_ : shotFrames_;
            auto* sprite = cocos2d::Sprite::createWithSpriteFrame(frames.front());
            if (bullet.weapon == Weapon::MachineGun)
            {
                sprite->setColor(cocos2d::Color3B(237, 212, 116));
                sprite->setScale(0.78f);
            }
            addChild(sprite, 2);
            found = bulletSprites_.emplace(bullet.id, sprite).first;
            auto* animation = cocos2d::Animation::createWithSpriteFrames(frames, 0.035f);
            sprite->runAction(cocos2d::Animate::create(animation));
            animateShot();
        }

        found->second->setPosition(
            visibleLeft + bullet.worldX - world.cameraX(),
            groundViewY_ + bullet.worldY
        );
    }
}

void GameScene::animateShot()
{
    peaMouth_->stopAllActions();
    peaMouth_->setPosition(mouthRestPosition_);
    peaMouth_->runAction(cocos2d::Sequence::create(
        cocos2d::MoveTo::create(0.035f, mouthRestPosition_ - cocos2d::Vec2(1.0f, 0.0f)),
        cocos2d::MoveTo::create(0.09f, mouthRestPosition_),
        nullptr
    ));
}

#ifndef NDEBUG
void GameScene::drawHitBoxes(float visibleLeft, float visibleWidth)
{
    if (!hitBoxOverlay_)
    {
        return;
    }
    hitBoxOverlay_->clear();

    const auto& world = controller_.world();
    const float xOffset = visibleLeft - world.cameraX();
    const auto drawBox = [&](const HitBox& box, const cocos2d::Color4F& color)
    {
        hitBoxOverlay_->drawRect(
            cocos2d::Vec2(xOffset + box.left, groundViewY_ + box.bottom),
            cocos2d::Vec2(xOffset + box.right(), groundViewY_ + box.top()),
            color
        );
    };

    const float groundY = groundViewY_ + world.groundY();
    hitBoxOverlay_->drawLine(
        cocos2d::Vec2(visibleLeft, groundY),
        cocos2d::Vec2(visibleLeft + visibleWidth, groundY),
        cocos2d::Color4F::WHITE
    );

    drawBox(world.playerHitBox(visibleWidth), cocos2d::Color4F::GREEN);
    for (const auto& segment : world.segments())
    {
        for (const auto& platform : segment.definition->platforms)
        {
            drawBox(platform.hitBox(segment.startX), cocos2d::Color4F(0.0f, 1.0f, 1.0f, 1.0f));
        }
        for (const auto& enemy : segment.enemies)
        {
            if (enemy.isAlive())
            {
                drawBox(enemy.hitBox(), cocos2d::Color4F::RED);
            }
        }
        for (const auto& trap : segment.traps)
        {
            drawBox(trap.hitBox(world.groundY()), cocos2d::Color4F::MAGENTA);
        }
    }

    for (const auto& bullet : world.bullets())
    {
        drawBox(bullet.hitBox(), cocos2d::Color4F::YELLOW);
    }
}
#endif

GameScene* GameScene::createForDifficulty(Difficulty difficulty)
{
    auto* scene = GameScene::create();
    if (scene)
    {
        scene->controller_.setDifficulty(difficulty);
    }
    return scene;
}

void GameScene::onEnterTransitionDidFinish()
{
    cocos2d::Scene::onEnterTransitionDidFinish();
    refreshMusic("audio/run.wav");
}

void GameScene::saveBestScore() const
{
    const auto& world = controller_.world();
    auto* storage = cocos2d::UserDefault::getInstance();
    const char* key = bestScoreKey(world.difficulty());
    if (world.score() > storage->getIntegerForKey(key))
    {
        storage->setIntegerForKey(key, world.score());
        storage->flush();
    }
}

void GameScene::updateHud()
{
    const auto& world = controller_.world();
    const int health = world.playerDefeated() ? 0 : world.player().health();
    if (health != displayedHealth_)
    {
        displayedHealth_ = health;
        hearts_->clear();
        for (int i = 0; i < 3; ++i)
        {
            const float x = 17.0f * i + 6.0f;
            const auto color = i < health ? cocos2d::Color4F(1.0f, 0.15f, 0.2f, 1.0f)
                                          : cocos2d::Color4F(0.33f, 0.23f, 0.23f, 1.0f);
            hearts_->drawSolidCircle({x - 3.0f, 3.0f}, 3.5f, 0.0f, 16, color);
            hearts_->drawSolidCircle({x + 3.0f, 3.0f}, 3.5f, 0.0f, 16, color);
            hearts_->drawTriangle({x - 6.0f, 3.0f}, {x + 6.0f, 3.0f}, {x, -7.0f}, color);
        }
    }
    if (world.score() != displayedScore_)
    {
        displayedScore_ = world.score();
        scoreLabel_->setString("Score: " + std::to_string(displayedScore_));
    }
}

void GameScene::showPause()
{
    paused_ = true;
    using namespace cocos2d;
    auto* director = Director::getInstance();
    const auto size = director->getVisibleSize();
    const auto origin = director->getVisibleOrigin();
    auto* panel = ui::Layout::create();
    panel->setContentSize(size);
    panel->setPosition(origin);
    panel->setTouchEnabled(true);
    panel->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
    panel->setBackGroundColor(Color3B(26, 51, 38));
    panel->setBackGroundColorOpacity(230);
    auto* title = Label::createWithSystemFont("Paused", "Arial", 28);
    title->setPosition(size.width * 0.5f, size.height * 0.72f);
    panel->addChild(title);
    auto* resume = MenuItemLabel::create(Label::createWithSystemFont("Continue", "Arial", 22),
        [this, panel](Ref*) { paused_ = false; panel->removeFromParent(); });
    auto* quit = MenuItemLabel::create(Label::createWithSystemFont("End Run", "Arial", 22),
        [this](Ref*) {
            saveBestScore();
            auto* titleScene = TitleScene::create();
            if (titleScene) { Director::getInstance()->replaceScene(titleScene); }
        });
    resume->setPosition(size.width * 0.5f, size.height * 0.5f);
    quit->setPosition(size.width * 0.5f, size.height * 0.31f);
    auto* menu = Menu::create(resume, quit, nullptr);
    menu->setPosition(Vec2::ZERO);
    panel->addChild(menu, 1);
    addChild(panel, 10);
}

void GameScene::syncPickups(float visibleLeft)
{
    const auto& world = controller_.world();
    const auto& pickups = world.pickups();
    for (auto it = pickupSprites_.begin(); it != pickupSprites_.end();)
    {
        const auto found = std::find_if(pickups.begin(), pickups.end(),
            [&](const Pickup& pickup) { return pickup.x == it->first; });
        if (found == pickups.end())
        {
            it->second->removeFromParent();
            it = pickupSprites_.erase(it);
        }
        else { ++it; }
    }

    for (const auto& pickup : pickups)
    {
        auto found = pickupSprites_.find(pickup.x);
        if (found == pickupSprites_.end())
        {
            auto* sprite = cocos2d::Sprite::create(pickup.weapon == Weapon::Corn
                ? "gameplay/pickup_corn.png" : "gameplay/pickup_machinegun.png");
            if (!sprite) { continue; }
            sprite->setAnchorPoint(cocos2d::Vec2(0.5f, 0.0f));
            const auto size = sprite->getContentSize();
            sprite->setScale(14.0f / std::max(size.width, size.height));
            addChild(sprite, 2);
            found = pickupSprites_.emplace(pickup.x, sprite).first;
        }
        found->second->setPosition(visibleLeft + pickup.x - world.cameraX(),
                                   groundViewY_ + pickup.y);
    }
}
