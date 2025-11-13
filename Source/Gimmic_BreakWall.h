#pragma once
#include "GimmicBase.h"
#include "GimmicManager.h"
#include "Collider.h"
#include "System/ShapeRenderer.h"

class Gimmic_BreakWall : public GimmicBase
{
public:
	Gimmic_BreakWall();

	// 衝突処理
	void OnCollision(GameObject* objects) override;

	// ギミック更新処理
	void Update(float elapsedTime)override;

	// 描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer) override;

	// デバッグ描画
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

	void OnImGui();

	bool IsBroken() const { return isBroken; } // 攻撃対象判定用

private:
	bool isBroken = false; // 壊れているかどうか
	float hp = 2.0f; // 耐久値

	float maxHp = 2.0f; // ���|�b�v���̂��߂ɍő�HP���L��
	float respawnTime = 5.0f; // リスポーンするまでの時間 (5秒)
	float respawnTimer = 0.0f; // リスポーンタイマー

	DirectX::XMFLOAT3 halfSize;
	DirectX::XMFLOAT3 size;

	OBB* box;
};

