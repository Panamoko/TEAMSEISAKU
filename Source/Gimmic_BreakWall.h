#pragma once
#include "GimmicBase.h"
#include "GimmicManager.h"
#include "Collider.h"
#include "System/ShapeRenderer.h"

class Gimmic_BreakWall : public GimmicBase
{
public:
	Gimmic_BreakWall();

	//衝突結果
	void OnCollision(GameObject* objects) override;

	//ギミック更新処理
	void Update(float elapsedTime)override;

	//デバッグ描画
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

private:
	bool isBroken = false;//壊れたかどうか
	float hp = 2.0f;//耐久度
	DirectX::XMFLOAT3 halfSize;
	BoxCollider* box = static_cast<BoxCollider*>(collider);
};

