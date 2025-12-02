#pragma once
#include "GimmicBase.h"
#include "Collider.h"
#include "CollisionManager.h"
#include "GimmicManager.h"
#include <DirectXMath.h>
#include <Animator.h>

class Core : public GimmicBase
{
public:
	Core();
	~Core();
	void init();
	void Update(float elapsedTime)override;
	void Render(const RenderContext& rc, ModelRenderer* renderer)override;
	//デバッグ描画
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;
	void OnCollision(GameObject* object)override;
	bool OnImGui()override;
	float GetHP() { return hp; };

	static Core* Instance();
private:
	static Core* sInstance;

	Animator animator;
	CylinderCollider* cylinder;

	// 死亡演出用メンバ変数
	bool isDying = false;           // 死亡演出中フラグ
	float dyingTimer = 0.0f;        // 演出経過時間
	const float dyingDuration = 5.0f; // 演出の長さ（秒）
	float currentSlowScale = 1.0f;  // 演出開始時のスロー倍率を保存

	// 演出用カメラパラメータ
	DirectX::XMFLOAT3 startFocus; // 演出開始時の注視点
	float startYawDeg = 0.0f;     // 演出開始時のYaw
	float startPitchDeg = 0.0f;   // 演出開始時のPitch
	float startDistance = 0.0f;   // 演出開始時の距離

	// 演出目標値
	float endPitchDeg = 45.0f;    // 最終的なカメラの俯瞰角度 
	float endDistance = 15.0f;     // 最終的なカメラ距離 
	float orbitSpeed = 45.0f;     // 旋回速度 (度/秒) 

	float shakeMagnitude = 0.0f; // 現在の揺れの強さ
};

