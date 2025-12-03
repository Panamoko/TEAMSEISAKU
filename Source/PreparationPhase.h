#pragma once
#include "GameSprite.h"

class PreparationPhase :public GameSprite
{
public:
	PreparationPhase();

	void Update(float elapsedTime)override;

	void Render()override;

private:
	bool state;
	Sprite* sprite;
	Sprite* sprite2;
	DirectX::XMFLOAT2 mouse_position;
	DirectX::XMFLOAT2 sprite2_pos;
};

