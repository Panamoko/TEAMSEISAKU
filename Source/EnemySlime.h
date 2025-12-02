#pragma once

#include "System/Model.h"
#include "Enemy.h"
#include "ProjectileManager.h"
#include "Editor.h"
#include "Animator.h"
#include "AStar.h"
#include "GridMap.h"

class Player;

// スライム（敵基底クラスとしても機能）
class EnemySlime : public Enemy
{
public:
	// --- コンストラクタ / デストラクタ ---
	EnemySlime(const char* modelPath = "Data/Model/chara/nico.mdl");
	~EnemySlime() override;

	// --- 基本更新・描画 ---
	void Update(float elapsedTime) override;
	void Render(const RenderContext& rc, ModelRenderer* renderer) override;
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

	// --- 外部設定 ---
	// 縄張りの設定
	void SetTerritory(const DirectX::XMFLOAT3& origin, float range);
	// マップ情報のセット
	void SetGridMap(const GridMap* map) { gridMap = map; }

	// --- イベントハンドラ ---
	void OnDead() override;                 // 死亡時
	void OnCollision(GameObject* object) override; // 衝突時

protected:
	// ==========================================
	// AI & 戦略ロジック
	// ==========================================

	// プレイヤーと味方が多いエリアを計算し、戦略的な目標地点を取得
	// 戻り値: true=戦略的移動が必要 / false=ランダム移動でOK
	bool GetStrategicTarget(DirectX::XMFLOAT3& outPos);

	// 経路探索を用いて目標地点へ移動
	void MoveToTarget(float elapsedTime, float moveSpeedRate, float turnSpeedRate);

	// 味方同士の分離行動（重なり防止）
	void ApplySeparationForce(float elapsedTime);

	// ターゲット（プレイヤーや味方）の索敵
	Character* SearchTarget();

	// ==========================================
	// ステートマシン (状態遷移)
	// ==========================================
	enum class State
	{
		Wander, // 徘徊
		Idle,   // 待機
		Attack  // 攻撃
	};

	// 徘徊ステート
	virtual void SetWanderState();
	void UpdateWanderState(float elapsedTime);

	// 待機ステート
	void SetIdleState();
	virtual void UpdateIdleState(float elapsedTime);

	// 攻撃ステート
	virtual void SetAttackState(Character* target);
	virtual void UpdateAttackState(float elapsedTime);

protected:
	// --- パラメータ ---
	State state = State::Wander;

	// 移動・探索関連
	DirectX::XMFLOAT3 targetPosition = { 0,0,0 }; // 現在の目的地
	DirectX::XMFLOAT3 territoryOrigin = { 0,0,0 };// 縄張りの中心
	float territoryRange = 10.0f;                 // 縄張りの半径
	float searchRange = 5.0f;                     // 索敵半径

	// 速度設定
	float moveSpeed = 2.0f;
	float turnSpeed = DirectX::XMConvertToRadians(360);

	// タイマー
	float stateTimer = 0.0f;        // ステート維持用
	float targetUpdateTimer = 0.0f; // 索敵・思考の間隔用

	// --- コンポーネント・オブジェクト ---
	ProjectileManager projectileManager;
	Animator animator;
	AStar aStar;
	const GridMap* gridMap = nullptr;

	// 経路探索用
	std::vector<std::pair<int, int>> currentPath;
	int pathIndex = 0;
	float pathRecalcTimer = 0.0f;

	// --- 攻撃制御 ---
	Character* targetCharacter = nullptr; // ロックオン中の相手
	bool isAttackFired = false;           // 攻撃モーション中に発射したか

private:
	// 内部リソース
	editor game_editor;
	std::vector<std::unique_ptr<GameSprite>> sprites2d;
	CylinderCollider* cylinder = nullptr;
	std::unique_ptr<Model> uniqueModel;
	// 親クラスの model ポインタは uniqueModel.get() を指す
};