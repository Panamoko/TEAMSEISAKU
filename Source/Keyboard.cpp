#include "Keyboard.h"
#include <Windows.h>

// キーが押されている状態を取得
bool Keyboard::IsKeyDown(KeyCode key) const
{
	if (key < 0 || key >= KEY_MAX) return false;
	return keyState[0][key];
}

// キーが押された瞬間を取得
bool Keyboard::IsKeyPushed(KeyCode key) const
{
	if (key < 0 || key >= KEY_MAX) return false;
	// 現在押されている かつ 前のフレームで押されていなかった
	return keyState[0][key] && !keyState[1][key];
}

// キーが離された瞬間（前フレームで押されていた）を取得
bool Keyboard::IsKeyReleased(KeyCode key) const
{
	if (key < 0 || key >= KEY_MAX) return false;
	// 現在押されていないかつ 前のフレームで押されていた
	return !keyState[0][key] && keyState[1][key];
}

// 更新 (キーボードの状態を取得)
void Keyboard::Update()
{
	// 1. 前フレームの状態をコピー (keyState[0] -> keyState[1])
	keyState[1] = keyState[0];

	// 2. 現在のフレームの状態を取得 (keyState[0])
	for (int i = 0; i < KEY_MAX; ++i)
	{
		// 戻り値の最上位ビット（0x8000）が立っている場合、キーは押されている
		keyState[0][i] = (GetKeyState(i) & 0x8000) != 0;
	}
}