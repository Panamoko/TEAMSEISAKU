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
    int current_index;//åªç›ï\é¶ÇµÇƒÇ¢ÇÈâÊëúÇÃî‘çÜ
    DirectX::XMFLOAT2 click_pos;
    DirectX::XMFLOAT2 click_size;
    DirectX::XMFLOAT2 mouse_pos;

    std::vector<Sprite> sprite;
    std::vector<std::string> sprite_paths;

    bool end_sprite_namber;
    std::vector<Sprite*> click_sprite;

    float BUTTON_WIDTH;
    float BUTTON_HEIGHT;
    float BUTTON_Y;
    float COLOR_R, COLOR_G, COLOR_B, COLOR_A;
    float BACK_BUTTON_X;
    float NEXT_BUTTON_X;
    float next_image_coror_a;
    float back_image_coror_a;
};