#pragma once
#include "GameSprite.h"

class PreparationPhase :public GameSprite
{
public:
	PreparationPhase();

	void Update(float elapsedTime)override;

	void Render()override;

	bool GetState() { return state; }

private:
	bool state;
	Sprite* sprite;
	Sprite* sprite2;
	DirectX::XMFLOAT2 mouse_position;
	DirectX::XMFLOAT2 sprite2_pos;
	DirectX::XMFLOAT2 sprite2_size;
	DirectX::XMFLOAT4 sprite2_color;
};

