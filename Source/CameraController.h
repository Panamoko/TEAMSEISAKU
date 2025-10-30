#pragma once

#include <DirectXMath.h>

// カメラコントローラー
class CameraController
{
public:
	// 更新処理
	void Update(float elapsedTime);

	// ターゲット位置設定
	void SetTarget(const DirectX::XMFLOAT3& target) { this->target = target; }

private:
	DirectX::XMFLOAT3		target = { 0, 0, 0 };
	DirectX::XMFLOAT3		angle = { 0, 0, 0 };
	float					rollSpeed = DirectX::XMConvertToRadians(90);
	float					range = 10.0f;

	float maxAngleX = DirectX::XMConvertToRadians(60); // 視線の角度上限（上を見上げる方向）を60度に設定
	float minAngleX = DirectX::XMConvertToRadians(45); // 視線の角度下限（見下ろす方向）を30度に設定

	// オービット用の状態（度数法）
	float yawDeg = 0.0f;
	float pitchDeg = 35.264f;
	float distance = 25.0f;     // 好きな初期距離

	// ピッチ制限
	float minPitchDeg = 5.0f;   // 真上に行き過ぎない
	float maxPitchDeg = 85.0f;  // 真下に行き過ぎない
};
