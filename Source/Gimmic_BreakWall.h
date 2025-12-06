#pragma once
#include "GimmicBase.h"
#include "GimmicManager.h"
#include "Collider.h"
#include "System/ShapeRenderer.h"

class Gimmic_BreakWall : public GimmicBase
{
public:
	Gimmic_BreakWall();

	void OnCollision(GameObject* objects) override;

	void Update(float elapsedTime)override;

	void Render(const RenderContext& rc, ModelRenderer* renderer) override;

	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

	bool OnImGui();

	bool IsBroken() const { return isBroken; }

protected:
	bool isBroken = false;//壁が壊れたかどうか

	float maxHp = 2.0f; //最大耐久力
	float respawnTime = 5.0f;//リスポーンを開始するまでの時間
	float respawnTimer = 0.0f;//リスポーンまでの残り時間

	bool isRespawning = false;//壁がリスポーン処理中かどうかを示すフラグ
	float fadeInDuration = 1.5f;//リスポーン時のフェードインにかかる時間
	float fadeInTimer = 0.0f;//フェードインの経過時間をカウントするタイマー

	DirectX::XMFLOAT3 halfSize;//壁の半分のサイズ
	DirectX::XMFLOAT3 size;//壁の全体のサイズ

	OBB* box;
};

