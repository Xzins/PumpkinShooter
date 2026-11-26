#pragma once

#include <array>
#include <map>

#include "cocos2d.h"
#include "../controller/GameController.h"

class GameScene : public cocos2d::Scene
{
public:
    bool init() override;
    void update(float dt) override;
    CREATE_FUNC(GameScene);
    static GameScene* createForDifficulty(Difficulty difficulty);
    void onEnterTransitionDidFinish() override;

private:
    void syncPlatforms(float visibleLeft);
    void syncEnemies(float visibleLeft);
    void syncTraps(float visibleLeft);
    void syncBullets(float visibleLeft);
    void animateShot();
    void syncPickups(float visibleLeft);
    void updateHud();
    void showPause();
    void saveBestScore() const;

    bool paused_ = false;
    Weapon displayedWeapon_ = Weapon::Pea;
    int displayedScore_ = -1;
    cocos2d::Sprite* pauseIcon_ = nullptr;
    cocos2d::DrawNode* hearts_ = nullptr;
    cocos2d::Label* scoreLabel_ = nullptr;
    cocos2d::Vector<cocos2d::SpriteFrame*> cornFrames_;
    std::map<float, cocos2d::Sprite*> pickupSprites_;
#ifndef NDEBUG
    void drawHitBoxes(float visibleLeft, float visibleWidth);
    cocos2d::DrawNode* hitBoxOverlay_ = nullptr;
#endif

    cocos2d::Sprite* peaMouth_ = nullptr;
    cocos2d::Vec2 mouthRestPosition_;
    cocos2d::Vector<cocos2d::SpriteFrame*> shotFrames_;
    std::map<std::uint64_t, cocos2d::Sprite*> bulletSprites_;
    bool createClouds(const cocos2d::Size& visibleSize, float visibleBottom);
    void syncClouds(float visibleLeft, float visibleWidth, float cameraX);

    std::array<cocos2d::Sprite*, 4> cloudSprites_{};

    struct GroundTile
    {
        cocos2d::Sprite* grass = nullptr;
        cocos2d::Sprite* dirt = nullptr;
    };

    bool createGround(float visibleWidth, float visibleBottom);
    void syncGround(float visibleLeft, float cameraX);

    std::array<GroundTile, 2> groundTiles_;
    float groundTileWidth_ = 0.0f;

    GameController controller_;
    std::map<float, std::vector<cocos2d::Sprite*>> platformSprites_;
    std::map<float, cocos2d::Sprite*> enemySprites_;
    std::map<float, cocos2d::Sprite*> trapSprites_;

    int displayedHealth_ = -1;
    cocos2d::Sprite* hero_ = nullptr;
    float groundViewY_ = 0.0f;
};