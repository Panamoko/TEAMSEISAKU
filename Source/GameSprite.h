#pragma once

#include <string>
#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "System/Sprite.h"
#include "SpriteManager.h"

class GameSprite
{
public:

	GameSprite();

	virtual void Update(float elapsedTime) {};

	void Render();

	void SetupSprite(const std::string& texture_path);

public:
	std::string name;
	std::string texture_name;
	DirectX::XMFLOAT2 position;
	DirectX::XMFLOAT2 size;
	DirectX::XMFLOAT2 uv_min;
	DirectX::XMFLOAT2 uv_max;
	float rotation;
	DirectX::XMFLOAT4 color;
	int sprite_index;

	Sprite* sprite_ptr = nullptr;
};