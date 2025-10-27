#include "System/Input.h"

// 初期化
void Input::Initialize(HWND hWnd)
{
	gamePad = std::make_unique<GamePad>();
	mouse = std::make_unique<Mouse>(hWnd);
}

// 更新処理
void Input::Update()
{
	gamePad->Update();
	mouse->Update();
}

// ★追加: マウスのX軸移動量
float Input::GetMouseDX() const
{
	// MouseクラスにGetDX()がある前提
	return mouse->GetDX();
}

// ★追加: マウスのY軸移動量
float Input::GetMouseDY() const
{
	// MouseクラスにGetDY()がある前提
	return mouse->GetDY();
}