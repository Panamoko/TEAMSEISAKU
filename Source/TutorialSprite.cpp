#include "TutorialSprite.h"
#include "Collision.h"
#include <System/Graphics.h>
#include <System/Mouse.h>
#include <System/Input.h>

TutorialSprite::TutorialSprite()
{
	click_pos = { 0.0f,0.0f };
	click_size = { 1280.0f,720.0f };
	position = { 0.0f,0.0f };
}

void TutorialSprite::AddSprite(const Sprite& sprite_date)
{
	sprite.push_back(sprite_date);
}

void TutorialSprite::Update()
{
	Mouse& mouse = Input::Instance().GetMouse();

	click_pos.x = static_cast<float>(mouse.GetPositionX());
	click_pos.y = static_cast<float>(mouse.GetPositionY());

	bool is_mouse_over_sprite = Collision::IntersectPosSquare(
		click_pos,
		{ position.x,position.y },
		{ click_size.x ,click_size.y });



}

void TutorialSprite::Render()
{
}
