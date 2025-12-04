#pragma once
#include "GameSprite.h"

class TutorialSprite :
    public GameSprite
{
public:
    TutorialSprite();
    void AddSprite(const Sprite& sprite_date);
    void Update();
    void Render();

private:
    int current_index;//Œ»İ•\¦‚µ‚Ä‚¢‚é‰æ‘œ‚Ì”Ô†
    DirectX::XMFLOAT2 click_pos;
    DirectX::XMFLOAT2 click_size;

    std::vector<Sprite> sprite;
};