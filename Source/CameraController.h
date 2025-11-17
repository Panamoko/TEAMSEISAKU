#pragma once

#include <DirectXMath.h>

// カメラコントローラー
class CameraController
{
public:
    CameraController();

	// 更新処理
	void Update(float elapsedTime);

	// ターゲット位置設定
	void SetTarget(const DirectX::XMFLOAT3& target) { this->target = target; }

	// カメラ操作の有効/無効を切り替えるstatic関数
	static void SetEnable(bool enable) { s_enable = enable; }

private:
	// 内部計算用ヘルパー
	void SyncAngleFromCamera(); // 現在のカメラ姿勢からYaw/Pitchを逆算する

private:
    static bool s_enable;

    DirectX::XMFLOAT3 target = { 0, 0, 0 };

    // --- 慣性制御用メンバ変数 (staticにしない！) ---
    float panVelX = 0.0f;
    float panVelZ = 0.0f;
    float zoomVel = 0.0f;
    bool  isInitialized = false; // 初回同期フラグ

    // パラメータ
    float yawDeg = 0.0f;
    float pitchDeg = 35.264f;
    float distance = 0.0f;

    const float minPitchDeg = 5.0f;
    const float maxPitchDeg = 85.0f;
    const float minDistance = 15.0f; // 近すぎないように
    const float maxDistance = 75.0f;
};
