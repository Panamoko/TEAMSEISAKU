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

	void OnImGui();

	static void OBBtoAABB(
		const DirectX::XMFLOAT3& center,
		const DirectX::XMFLOAT3& half,
		const DirectX::XMFLOAT3 axis[3],
		DirectX::XMFLOAT3& outPos,
		DirectX::XMFLOAT3& outSize
	);

private:
	bool isBroken = false;//壊れたかどうか
	float hp = 2.0f;//耐久度
	DirectX::XMFLOAT3 halfSize;
	DirectX::XMFLOAT3 size;

	OBB* box;
};

