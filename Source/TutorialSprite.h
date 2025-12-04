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
    bool LoadSprite(const std::vector<std::string>& file_paths);

private:
    int current_index;//Œ»İ•\¦‚µ‚Ä‚¢‚é‰æ‘œ‚Ì”Ô†
    DirectX::XMFLOAT2 click_pos;
    DirectX::XMFLOAT2 click_size;
    DirectX::XMFLOAT2 mouse_pos;

    std::vector<Sprite> sprite;
    std::vector<std::string> sprite_paths;
};