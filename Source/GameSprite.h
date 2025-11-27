#pragma once

#include <string>
#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "System/Sprite.h"

class GameSprite
{
public:

	GameSprite();

	virtual void Update(float elapsedTime) {};

	void Render();

public:
	std::string name;
	std::string texture;
	DirectX::XMFLOAT2 position;
	DirectX::XMFLOAT2 size;
	DirectX::XMFLOAT2 uv_min;
	DirectX::XMFLOAT2 uv_max;
	float rotation;
	DirectX::XMFLOAT4 color;
	int sprite_index;

	Sprite sprite = nullptr;
};