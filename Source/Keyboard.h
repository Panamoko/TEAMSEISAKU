#pragma once

#include <array>
#include <Windows.h> // 仮想キーコード VK_XX を使用するために必要

// 仮想キーコード
using KeyCode = int;

// キーボード
class Keyboard
{
public:
	// Windowsの仮想キーコード（VK_XX）をカバーするために256キーを想定
	static constexpr int KEY_MAX = 256;

public:
	Keyboard() = default;
	~Keyboard() = default;

	// 初期化
	void Initialize() {}

	// 更新
	void Update();

	// キーが押されている状態を取得
	bool IsKeyDown(KeyCode key) const;

	// キーが押された瞬間を取得
	bool IsKeyPushed(KeyCode key) const;

	// キーが離された瞬間を取得
	bool IsKeyReleased(KeyCode key) const;

private:
	// キーの状態を保持します
	// [0]: 現在のフレームの状態, [1]: 前のフレームの状態
	std::array<bool, KEY_MAX> keyState[2] = {};
};