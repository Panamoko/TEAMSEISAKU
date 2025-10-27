#pragma once

#include <memory>
#include "System/GamePad.h"
#include "System/Mouse.h"

// インプット
class Input
{
private:
	Input() = default;
	~Input() = default;

public:
	// インスタンス取得
	static Input& Instance()
	{
		static Input instance;
		return instance;
	}

	// 初期化
	void Initialize(HWND hWnd);

	// 更新処理
	void Update();

	// ゲームパッド取得
	GamePad& GetGamePad() { return *gamePad; }

	// マウス取得
	Mouse& GetMouse() { return *mouse; }

	// マウスのX軸移動量（前フレームとの差分）
	float GetMouseDX() const;

	// マウスのY軸移動量（前フレームとの差分）
	float GetMouseDY() const;


private:
	std::unique_ptr<GamePad>	gamePad;
	std::unique_ptr<Mouse>		mouse;
};