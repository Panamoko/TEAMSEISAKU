#pragma once
#include <string>
#include <System/Sprite.h>
#include <System/Input.h>
#include "GameSprite.h"

class ChangeSceneSytem :public GameSprite
{
public:

	ChangeSceneSytem();

	~ChangeSceneSytem();

	// 更新処理
	void Update(float elapsedTime)override;

	// 描画処理
	void Render()override;

	// GUI描画
	void DrawGUI();

private:

	static int stage_namber;
	DirectX::XMFLOAT2 mouse_position;
	float sprite_width;
	float sprite_height;
	float sprite_left;
	float sprite_top;
	float alpha_timer = 0.0f;     // α値計算用のタイマー
	float blink_interval = 0.5f;  // 点滅間隔（例: 0.5秒ごとに切り替え）

	std::vector<Sprite*> sprite_vector = std::vector<Sprite*>(2);
	std::vector<Sprite*> sprite_stage;
};