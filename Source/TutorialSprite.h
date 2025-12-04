#pragma once
#include "GameSprite.h"
class TutorialSprite :
    public GameSprite
{
public:

private:
    int current_index;//Œ»İ•\¦‚µ‚Ä‚¢‚é‰æ‘œ‚Ì”Ô†
    std::vector<Sprite> sprite;
};