#include "TitleScene.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "GameScene.h"
#include "GameMusic.h"
USING_NS_CC;

bool TitleScene::init()
{
    if(!Scene::init())
    {
        return false;
    }
    auto* Background = Sprite::create("ui/title/cover_background.png");
    if (Background == nullptr)
    {
        CCLOG("TitleScene: failed to load ui/title/cover_background.png");
        return false;
    }
    auto* director = cocos2d::Director::getInstance();
    const auto visibleSize = director->getVisibleSize();
    const auto origin = director->getVisibleOrigin();
    const auto backgroundSize = Background->getContentSize();
    const float scaleX = visibleSize.width / backgroundSize.width;
    const float scaleY = visibleSize.height / backgroundSize.height;
    Background->setScale(std::max(scaleX, scaleY));
    Background->setPosition(
        origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height * 0.5f
    );

    addChild(Background, 0);

    auto* startButton = ui::Button::create("ui/difficulty/difficulty_button.png");
    if (startButton == nullptr) {
        CCLOG("TitleScene: failed to load ui/difficulty/difficulty_button.png");
        return false;
    }

    startButton->setTitleText("开始游戏");
    startButton->setTitleFontSize(24);
    
    if (auto* logo = Sprite::create("ui/title/title_logo.png"))
    {
        const auto size = logo->getContentSize();
        logo->setScale(std::min(visibleSize.width * 0.47f / size.width,
                                visibleSize.height * 0.25f / size.height));
        logo->setPosition(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.81f);
        addChild(logo, 1);
    }

    const float buttonWidth = visibleSize.width * 0.24f;
    const float startScale = buttonWidth / startButton->getContentSize().width;
    startButton->setScale(startScale);
    startButton->setTitleFontSize(17.0f / startScale);

    startButton->setPosition(cocos2d::Vec2(
        origin.x + visibleSize.width * 0.50f,
        origin.y + visibleSize.height * 0.57f
    ));
    startButton->addClickEventListener([this, director](cocos2d::Ref*)
    {
        GameScene* gameScene = GameScene::createForDifficulty(static_cast<Difficulty>(selectedDifficulty_));
        if (gameScene == nullptr)
        {
            CCLOG("TitleScene: failed to create GameScene");
            return;
        }
        director->replaceScene(cocos2d::TransitionFade::create(0.5f, gameScene));
    });
    addChild(startButton, 1);

    // Add the difficulty buttons
    auto* difficultyButton = ui::Button::create(
        "ui/difficulty/difficulty_button.png"
    );
    auto* bestScoreLabel = Label::createWithSystemFont("", "Arial", 16);
    if (difficultyButton == nullptr || bestScoreLabel == nullptr)
    {
        CCLOG("TitleScene: failed to create difficulty controls");
        return false;
    }

    const float centerX = origin.x + visibleSize.width * 0.5f;
    const float difficultyY = origin.y + visibleSize.height * 0.39f;
    const float difficultyScale =
        buttonWidth / difficultyButton->getContentSize().width;

    difficultyButton->setScale(difficultyScale);
    difficultyButton->setTitleFontSize(18.0f / difficultyScale);
    difficultyButton->setPosition(Vec2(centerX, difficultyY));

    bestScoreLabel->setPosition(Vec2(
        centerX,
        difficultyY - difficultyButton->getContentSize().height
            * difficultyScale * 0.5f - 14.0f
    ));

    auto showDifficulty = [difficultyButton, bestScoreLabel](int index)
    {
        static const char* const names[] = {"Easy", "Normal", "Hard"};
        static const char* const scoreKeys[] = {
            "best_score_easy", "best_score_normal", "best_score_hard"
        };

        difficultyButton->setTitleText(names[index]);

        const int best = UserDefault::getInstance()->getIntegerForKey(
            scoreKeys[index], 0
        );
        bestScoreLabel->setString("Best: " + std::to_string(best));
    };

    showDifficulty(selectedDifficulty_);

    difficultyButton->addClickEventListener(
        [this, showDifficulty](Ref*)
        {
            selectedDifficulty_ = (selectedDifficulty_ + 1) % 3;
            showDifficulty(selectedDifficulty_);
        }
    );

    addChild(difficultyButton, 1);
    addChild(bestScoreLabel, 1);

    // Add the settings button and the tutorial button
    const float iconSize = 34.0f;
    const float margin = 12.0f;
    const float gap = 8.0f;
    const float top = origin.y + visibleSize.height;

    auto addTopIcon = [&](const char* path, float left)
        -> ui::Button*
    {
        auto* button = ui::Button::create(path);
        if (button == nullptr)
        {
            CCLOG("TitleScene: failed to create icon %s", path);
            return nullptr;
        }

        const auto size = button->getContentSize();
        button->setScale(iconSize / std::max(size.width, size.height));
        button->setPosition(Vec2(
            origin.x + left + iconSize * 0.5f,
            top - margin - iconSize * 0.5f
        ));
        addChild(button, 1);
        return button;
    };

    auto* settingsButton = addTopIcon("ui/title/settings_button.png", margin);
    auto* tutorialButton = addTopIcon(
        "ui/title/tutorial_button.png",
        margin + iconSize + gap
    );
    if (settingsButton == nullptr || tutorialButton == nullptr)
    {
        return false;
    }

    settingsButton->addClickEventListener([this, origin, visibleSize](Ref*)
    {
        auto* panel = ui::Layout::create();
        panel->setContentSize(visibleSize);
        panel->setPosition(origin);
        panel->setTouchEnabled(true);
        panel->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
        panel->setBackGroundColor(Color3B(27, 52, 38));
        auto* storage = UserDefault::getInstance();
        auto* toggleText = Label::createWithSystemFont(
            storage->getBoolForKey("bgm_enabled", true) ? "BGM: ON" : "BGM: OFF", "Arial", 25);
        auto* toggle = MenuItemLabel::create(toggleText, [storage, toggleText](Ref*)
        {
            const bool enabled = !storage->getBoolForKey("bgm_enabled", true);
            storage->setBoolForKey("bgm_enabled", enabled);
            storage->flush();
            toggleText->setString(enabled ? "BGM: ON" : "BGM: OFF");
            refreshMusic("audio/title.wav");
        });
        auto* closeText = Label::createWithSystemFont("Back", "Arial", 20);
        auto* close = MenuItemLabel::create(closeText, [panel](Ref*) { panel->removeFromParent(); });
        toggle->setPosition(visibleSize.width * 0.5f, visibleSize.height * 0.60f);
        close->setPosition(visibleSize.width * 0.5f, visibleSize.height * 0.34f);
        auto* menu = Menu::create(toggle, close, nullptr);
        menu->setPosition(Vec2::ZERO);
        panel->addChild(menu, 1);
        addChild(panel, 10);
    });

    tutorialButton->addClickEventListener([this, origin, visibleSize](Ref*)
    {
        auto* panel = ui::Layout::create();
        panel->setContentSize(visibleSize);
        panel->setPosition(origin);
        panel->setTouchEnabled(true);
        panel->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
        panel->setBackGroundColor(Color3B(27, 52, 38));
        auto* card = Sprite::create("ui/title/tutorial_card.png");
        if (!card)
        {
            return;
        }
        const auto size = card->getContentSize();
        card->setScale(std::min(visibleSize.width * 0.94f / size.width,
                                visibleSize.height * 0.78f / size.height));
        card->setPosition(visibleSize.width * 0.5f, visibleSize.height * 0.56f);
        panel->addChild(card);
        auto* close = MenuItemLabel::create(Label::createWithSystemFont("Back", "Arial", 17),
            [panel](Ref*) { panel->removeFromParent(); });
        close->setPosition(visibleSize.width * 0.5f, visibleSize.height * 0.085f);
        auto* menu = Menu::create(close, nullptr);
        menu->setPosition(Vec2::ZERO);
        panel->addChild(menu, 1);
        addChild(panel, 10);
    });
    return true;
}

void TitleScene::onEnterTransitionDidFinish()
{
    cocos2d::Scene::onEnterTransitionDidFinish();
    refreshMusic("audio/title.wav");
}
