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

	//描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer) override;

	//デバッグ描画
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

	void OnImGui();

private:
	bool isBroken = false;//壊れたかどうか
	float hp = 2.0f;//耐久度

	float maxHp = 2.0f; // リポップ時のために最大HPを記憶
	float respawnTime = 5.0f; // リポップするまでの時間 (5秒)
	float respawnTimer = 0.0f; // リポップタイマー

	DirectX::XMFLOAT3 halfSize;
	DirectX::XMFLOAT3 size;

	OBB* box;
};

