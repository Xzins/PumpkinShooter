#include <iostream>
#include "cocos2d.h"

class TitleScene : public cocos2d::Scene
{
    public:
    bool init() override;
    void onEnterTransitionDidFinish() override;
private:
    int selectedDifficulty_ = 0;
public:
    CREATE_FUNC(TitleScene);

};
