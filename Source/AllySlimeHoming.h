#pragma once
// =============================================
// AllySlimeHoming.h
// AllySlime と“同じ設計”で作った追尾弾スライム。
// ・隊列アンカーの計算(UpdateAnchor)
// ・追従は Character::Move/Turn（moveSpeed/turnSpeed 使用）
// ・自動攻撃はタイマー＋最寄り敵探索→ProjectileHoming を発射
// ・弾は自前の ProjectileManager で管理（更新・描画・当たり）
// ・Render/RenderDebugPrimitive のシグネチャも AllySlime と一致
// モデルは暫定で AllySlime と同じ（後で差し替え）
// =============================================

#include "System/Model.h"
#include "System/ModelRenderer.h"
#include "Character.h"
#include "ProjectileManager.h"

class Player;
class Enemy;
struct RenderContext;
class ShapeRenderer;

class AllySlimeHoming : public Character
{
public:
	AllySlimeHoming(int formationIndex);
	~AllySlimeHoming() override {}

	// AllySlime と揃える公開 API
	void SetIndex(int idx) { index = idx; }
	void SetLeader(Player* p) { leader = p; }
	Player* GetLeader() const { return leader; }

	// 更新／描画（シグネチャを AllySlime と合わせる）
	void Update(float elapsedTime);
	void Render(const RenderContext& rc, ModelRenderer* renderer);
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

	// 必要に応じて外から切り替えられるよう公開（AllySlime と同様運用）
	void AutoAttackUpdate(float elapsedTime);

private:
	void UpdateAnchor();
	void CollisionProjectilesVsEnemies();

private:
	// ===== 編隊・追従パラメータ（AllySlime と同値） =====
	int   index = 0;               // 編隊内スロット番号
	int   rowWidth = 4;            // 1列あたり
	float followDistance = 1.2f;   // 前後間隔
	float lateralSpacing = 0.9f;   // 左右間隔
	float moveSpeed = 3.0f;        // 追従移動速度（お好みで）
	float turnSpeed = 6.0f;        // 追従回頭速度（お好みで）

	// ===== 自動攻撃 =====
	bool  autoAttackEnabled = true;
	float autoAttackRange = 8.0f;
	float autoAttackInterval = 1.5f;
	float autoAttackTimer = 0.0f;

	// ===== 見た目・弾管理 =====
	DirectX::XMFLOAT3 anchor = { 0,0,0 };
	Model* slimeModel = nullptr;           // 暫定：AllySlime と同モデル
	ProjectileManager projectileManager;   // 自前で所有

	// ===== 編隊リーダー =====
	Player* leader = nullptr; // null の場合は Player::Instance() を参照
};